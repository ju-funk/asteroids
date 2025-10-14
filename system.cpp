#include "main.h"

// ------------------------------------------------------------------
// free function: userNotice - displays standard windows dialog
// ------------------------------------------------------------------
void sys::userNotice( const TCHAR *szMessage, bool isFatal )
{
    unsigned long int iType = MB_OK|MB_TOPMOST;
    const TCHAR *szCaption = output.GetTitle();

    if ( isFatal )
    {
        szCaption = _T("Fatal Error");
        iType |= MB_ICONERROR;
    }
    else
    {
        szCaption = _T("Information");
        iType |= MB_ICONINFORMATION;
    }

    MessageBox( 0, szMessage, szCaption, iType );
}

// ------------------------------------------------------------------
// free function: getSeed - uses native api's to get random seed
// ------------------------------------------------------------------
unsigned long sys::getSeed( void )
{
    // get clock ticks, remove higher bytes
    unsigned long iLow = GetTickCount64() & 0xFFFF;

    // retrieve system clock
    SYSTEMTIME lpTime = { 0 };
    GetLocalTime( &lpTime );

    // get clock milloseconds
    unsigned long iHigh = lpTime.wMilliseconds;

#ifndef _WIN64
    // xor processor tick count with seeds
    __asm
    {
        push eax
        push ebx
        rdtsc
        xor eax, ebx
        xor iHigh, eax
        xor ebx, iHigh
        xor iLow, ebx
        pop ebx
        pop eax
    }
#endif
    // combine and return
    return (iHigh<<16) | iLow;
}


#ifdef _DEBUG
void sys::DebugOut(const TCHAR* pszFmt, ...)
{
    va_list ptr;
    va_start(ptr, pszFmt);
    TCHAR buf[300];
    buf[0] = 0;
    _vstprintf_s(buf, 300, pszFmt, ptr);
    OutputDebugString(buf);
    va_end(ptr);
}
#endif // DEBUG




/////////////////////////////////////////////////////////////
///// class screen
////////////////////////////////////////////////////////////


bool sys::screen::Create(int width, int height, const TCHAR* szCaption)
{
    pcTitle = szCaption;
    iWidth = width;
    iHeight = height;

    // create the window, verify success
    bool ret = create();

    if(ret && (hWnd != nullptr))
        SetWindowText(hWnd, szCaption);
    else
        sys::userNotice(_T("Can not init Window"), true);
   
    setVisible(true);
    clearBuffer();
    flipBuffers();

    return ret;
}


// ------------------------------------------------------------------
// screen object: winDlgProc - static event handling proceedure
// ------------------------------------------------------------------
LRESULT CALLBACK sys::screen::winDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static sys::screen* This = nullptr;

    switch( uMsg )
    {
    case WM_NULL:
        if(hWnd == 0 && wParam == 0)
            This = reinterpret_cast<sys::screen*>(lParam);
        break;

    case WM_MOUSEWHEEL:
        This->SetInputState(WM_MOUSEWHEEL, wParam, lParam);
        break;
   
    case WM_USER + 1:
        output.SetFull(true);
        break;

    case WM_KEYDOWN:
        if ( wParam != VK_ESCAPE )
            break;
        // handle escape key as close event

    case WM_CLOSE:
        // if threads are running, signal quit
        if ( !bHasTermSignal )
            bHasTermSignal = true;
        else
            This->cleanup();  // if threads have finished, kill the dialog
        break;

    case WM_DESTROY:
        // send WM_QUIT
        PostQuitMessage( 0 );
        break;

    default:
        // all other messages are handled by windows
        return DefWindowProc( hWnd, uMsg, wParam, lParam );
    }

    return 0;
}

void sys::screen::SetInputState(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_MOUSEWHEEL:
        int delta = GET_WHEEL_DELTA_WPARAM(wParam); // Wheel
        if (delta > 0)
            StWheel = 1; // up
        else if (delta < 0)
            StWheel = 2; // down
        break;
    }
}

// input handling
bool sys::screen::GetInputState(POINT& mousePos, int &wheel)
{
    bool ret = EnaMouse;

    GetCursorPos(&mousePos);
    ScreenToClient(hWnd, &mousePos);

    if (mousePos.x < 0 || mousePos.y < 0 ||
        mousePos.x > iWidth || mousePos.y > iHeight)
        ret = false;

    wheel = StWheel;
    StWheel = 0;

    return ret;
}


// ------------------------------------------------------------------
// screen object: cleanup - deallocate desktop window resources
// ------------------------------------------------------------------
void sys::screen::cleanup( void )
{
    // reset display mode
    if ( hasFullScreen )
        toggleFullScreen();

    // cleanup gdi buffer
    SelectObject( hVideoDC, hOldFontObj);
    SelectObject( hVideoDC, hOldObject );
    DeleteObject( hBitmap );
    DeleteObject(hOldFontObj);
    DeleteDC( hVideoDC );

    CleanWave();

    // cleanup window
    ReleaseDC( hWnd, hDC );
    DestroyWindow( hWnd );
    UnregisterClass( szClass, hInstance );
}

// ------------------------------------------------------------------
// screen object: doEvents - process window messages
// ------------------------------------------------------------------
bool sys::screen::doEvents( void )
{
    if (!IsSetFull())
    {
        BOOL iResult = GetMessage(&wMsg, 0, 0, 0);

        // handle errors, according to msdn
        if (iResult == -1)
        {
            userNotice(_T("Window message pump failure."), true);
            return false;
        }

        // recieved quit message
        if (!iResult)
        {
            // must return exit code, as described by MSDN
            iExitCode = (int)wMsg.wParam;

            // signal quit event
            bHasTermSignal = true;
            return false;
        }

        // let windows handle the message
        TranslateMessage(&wMsg);
        DispatchMessage(&wMsg);
    }
    else
    {
        toggleFullScreen();
        SetFull(false);
    }

    return true;
}


// ------------------------------------------------------------------
// screen object: create - create window and video buffer
// ------------------------------------------------------------------
bool sys::screen::create()
{
    winDlgProc(0, WM_NULL, 0, reinterpret_cast<LPARAM>(this));  //init Win Dlg function with this pointer

    // fill class struct
    WNDCLASSEX wClass = { 0 };
    wClass.cbSize = sizeof(WNDCLASSEX);
    wClass.lpfnWndProc = winDlgProc;
    wClass.hInstance = hInstance = GetModuleHandle(0);
    wClass.lpszClassName = szClass = (TCHAR *) _T("dilaDemo");
    wClass.hCursor = LoadCursor( 0, IDC_ARROW );
    wClass.hIcon   = LoadIcon(hInstance, (LPCTSTR)IPP_ICON);

    // attempt create window class
    if ( !RegisterClassEx(&wClass) )
    {
        userNotice( _T("Unable to register window class."), true );
        return false;
    }

    // set dimensions
    rSize.right = iWidth;
    rSize.bottom = iHeight;
    rSize.left = rSize.top = 0;

    // determine window style
    DWORD dwStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    // get window to caluclate exact size of window based on style
    if ( !AdjustWindowRectEx(&rSize, dwStyle, 0, 0) )
    {
        UnregisterClass( szClass, hInstance );
        userNotice( _T("Could not set correct window dimensions."), true );
        return false;
    }

    // set true height and width of window area
    rSize.right -= rSize.left;
    rSize.bottom -= rSize.top;

    int cx = GetSystemMetrics( SM_CXSCREEN );
    int cy = GetSystemMetrics( SM_CYSCREEN );

    // center window area
    rSize.left = cx/2 - iWidth/2;
    rSize.top = cy/2 - iHeight/2;

    // create the window with desired attributes
    hWnd = CreateWindowEx( 0, szClass, 0, dwStyle, rSize.left,
                rSize.top, rSize.right, rSize.bottom, 0, 0, hInstance, 0 );

    if ( !hWnd )
    {
        UnregisterClass( szClass, hInstance );
        userNotice( _T("Could not create desktop window."), true );
        return false;
    }

    // set visible state flag
    bVisibleState = false;

    // retrieve window device context
    hDC = ::GetDC( hWnd );

    if ( !hDC )
    {
        DestroyWindow( hWnd );
        UnregisterClass( szClass, hInstance );
        userNotice( _T("Could not obtain window device handle."), true );
        return false;
    }

    // create new device handle from window context
    hVideoDC = CreateCompatibleDC( hDC );

    // verify device context was created
    if ( !hVideoDC )
    {
        ReleaseDC( hWnd, hDC );
        DestroyWindow( hWnd );
        UnregisterClass( szClass, hInstance );
        userNotice( _T("Unable to create new device context."), true );
        return false;
    }

    // fill bitmap info struct
    BITMAPINFO lpBmpInfo = { 0 };
    lpBmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    lpBmpInfo.bmiHeader.biWidth = iWidth;
    lpBmpInfo.bmiHeader.biHeight = -iHeight;
    lpBmpInfo.bmiHeader.biPlanes = 1;
    lpBmpInfo.bmiHeader.biBitCount = 32;
    lpBmpInfo.bmiHeader.biCompression = BI_RGB;

    // create GDI bitmap object
    hBitmap = CreateDIBSection( hVideoDC,
        &lpBmpInfo, DIB_RGB_COLORS, (void**)&pBitmap, 0, 0 );

    // verify object was created
    if ( !hBitmap )
    {
        DeleteDC( hVideoDC );
        ReleaseDC( hWnd, hDC );
        DestroyWindow( hWnd );
        UnregisterClass( szClass, hInstance );
        userNotice( _T("Could not create bitmap buffer."), true );
        return false;
    }

    // store address of buffer end and set size
    iBmpSize = iWidth*iHeight * sizeof(unsigned long);

    // select GDI object into new device context
    hOldObject = SelectObject( hVideoDC, hBitmap );

    // verify select was sucessfull
    if ( !hOldObject )
    {
        DeleteObject( hBitmap );
        DeleteDC( hVideoDC );
        ReleaseDC( hWnd, hDC );
        DestroyWindow( hWnd );
        UnregisterClass( szClass, hInstance );
        userNotice( _T("Could not select bitmap into device context."), true );
        return false;
    }

    return LoadWaves();
}

void sys::screen::SetNewFont(const TCHAR *Fn, int Size, int Ta, int Bm, int cF, COLORREF ClTx, COLORREF ClBa)
{
    if (hOldFontObj != nullptr)
    {
        hOldFontObj = SelectObject(hVideoDC, hOldFontObj);
        DeleteObject(hOldFontObj);
    }

    HFONT hFont = CreateFont(Size, 0, 0, 0, (cF & 1) == 1 ? FW_BOLD : FW_NORMAL, (cF & 2) == 2 ? TRUE : FALSE, (cF & 4) == 4 ? TRUE : FALSE, FALSE,
        ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, Fn);
    hOldFontObj = SelectObject(hVideoDC, hFont);

    SetTextColor(hVideoDC, ClTx);
    SetBkColor(hVideoDC, ClBa);
    SetBkMode(hVideoDC, Bm);
    SetTextAlign(hVideoDC, Ta);
}

SIZE sys::screen::GetTextSize(const TCHAR* text)
{
    SIZE size;
    TEXTMETRIC tm;
    GetTextMetrics(hVideoDC, &tm);
    GetTextExtentPoint32(hVideoDC, text, static_cast<int>(_tcslen(text)), &size);
    size.cy = tm.tmAscent;
    return size;
}

// ------------------------------------------------------------------
// screen object: toggleFullScreen - change display resolution
// ------------------------------------------------------------------
bool sys::screen::toggleFullScreen( void )
{
    // set fullscreen mode
    if ( !hasFullScreen )
    {
        windowStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
        SetWindowLongPtr(hWnd, GWL_STYLE, windowStyle & ~(WS_CAPTION | WS_THICKFRAME));
        SetWindowPos(hWnd, nullptr, 0, 0, iWidth, iHeight, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);

       // initialize device configuration
        dmOriginalConfig.dmSize = sizeof(DEVMODE);
        dmOriginalConfig.dmDriverExtra = 0;

        // retrieve current configuration, so that it may be restored
        BOOL bResult = EnumDisplaySettings( 0, ENUM_CURRENT_SETTINGS, &dmOriginalConfig );

        // verify success
        if ( !bResult )
            return false;

        // duplicate current config, and adjust resolution
        DEVMODE dmNewConfig = dmOriginalConfig;
        dmNewConfig.dmPelsWidth = iWidth;
        dmNewConfig.dmPelsHeight = iHeight;

        // test if the display mode is valid
        LONG iResult = ChangeDisplaySettings( &dmNewConfig, CDS_TEST );
        if ( iResult != DISP_CHANGE_SUCCESSFUL )
            return false;

        // if the display test succeeded, change the resolution
        iResult = ChangeDisplaySettings( &dmNewConfig, CDS_FULLSCREEN );

        // verify success
        if ( iResult != DISP_CHANGE_SUCCESSFUL )
            return false;

        // hide system cursor
        ShowCursor(EnaMouse);

        // change success, set status flag
        hasFullScreen = true;
    }
    else  // restore original configuration
    {
        // attempt to restore previous configuration
        LONG iResult = ChangeDisplaySettings( &dmOriginalConfig, CDS_RESET );
        // verify display mode was restored
        if ( iResult != DISP_CHANGE_SUCCESSFUL )
            ChangeDisplaySettings( nullptr, 0 );// restore display with registry settings

        SetWindowLongPtr(hWnd, GWL_STYLE, windowStyle);
        SetWindowPos(hWnd, nullptr, rSize.left, rSize.top, rSize.right, rSize.bottom, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);

        // display cursor, set status flag
        ShowCursor( true );
        hasFullScreen = false;
        EnaMouse = true;
    }

    // return success
    return true;
}


// ------------------------------------------------------------------
// screen object: setVisible - show/hide desktop window
// ------------------------------------------------------------------
void sys::screen::setVisible( bool state )
{
    // show/hide window
    if ( bVisibleState = state )
    {
        ShowWindow( hWnd, SW_SHOWNORMAL );
        SetForegroundWindow( hWnd );
    }
    else
        ShowWindow( hWnd, SW_HIDE );
}

// ------------------------------------------------------------------
// screen object: redrawWindow - redraw entire window area
// ------------------------------------------------------------------
void sys::screen::flipBuffers( void )
{
    BitBlt( hDC, 0, 0, iWidth, iHeight, hVideoDC, 0, 0, SRCCOPY );
}

// ------------------------------------------------------------------
// screen object: clearBuffer - initialize video buffer
// ------------------------------------------------------------------
void sys::screen::clearBuffer( void )
{
    ZeroMemory( pBitmap, iBmpSize );
}


/////////////////////////////////////////////////////////////////////
//// Key Handling
/////////////////////////////////////////////////////////////////////


bool KeyMan::GetKeyState(int Key, int Todo, int extkey)
{
    bool ret = false, down = false;

    if(!output.IsActiv())
        return false;

    short ekey = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? eKeyShift : 0;
    ekey |= (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? eKeyCtrl : 0;
    ekey |= (GetAsyncKeyState(VK_MENU) & 0x8000) ? eKeyAlt : 0;

    if (ekey != extkey)
        return false;


    short state = GetAsyncKeyState(Key);

    switch (Todo)
    {
    case IsDown:
        return ((state & 0x8000) == 0x8000);
    case MustToggle:
    {
        kdat& dat = GetKDat(Key, extkey);
        if (!dat.isPress)
        {
            dat.isPress = ((state & 0x8000) == 0x8000);
            return dat.isPress;
        }
        else
        {
            dat.isPress = ((state & 0x8001) != 0);
            return false;
        }
    }
    default:  // timer
    {
        kdat& dat = GetKDat(Key, extkey);

        if ((state & 0x8000) == 0x8000)
        {
            auto now = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - dat.last).count();
            if (ms > Todo)
            {
                dat.last = now;
                return true;
            }
        }

        return false;
    }
    }

    return ret;
}


KeyMan::kdat& KeyMan::GetKDat(int Key, int extkey)
{
    auto it = std::find_if(vkDat.begin(), vkDat.end(), [Key, extkey](kdat& v) { return v.Key == Key && v.extKeys == extkey; });
    if (it != vkDat.end())
        return *it;

    kdat dat(Key, extkey);
    vkDat.push_back(dat);

    return *std::prev(vkDat.end());
}





/////////////////////////////////////////////////////////////////////
//// Wave Sound Handling
/////////////////////////////////////////////////////////////////////


sys::screen::mpbtSnd sys::screen::OpenWave(stSound* Snd)
{
    size_t i = Snd->key++;

    mpbtSnd itsnd = Snd->vSound.insert({ i, vtSound(i) });

    //DebugOut(_T("Open               stSP : %p, vtSP : %p, Key : %03i, SizeMap : %03i\n"), Snd, &itsnd.first->second, i, Snd->vSound.size());

    MMRESULT res = waveOutOpen(&itsnd.first->second.hWaveOut, WAVE_MAPPER, &Snd->wfx,
        (DWORD_PTR)WaveOutProc, (DWORD_PTR)&itsnd.first->second, CALLBACK_FUNCTION);

    if (res != MMSYSERR_NOERROR)
    {
        userNotice(_T("Unable to register sound."), true);
        return mpbtSnd(Snd->vSound.end(), false);
    }
    else
    {
        itsnd.first->second.waveHdr.lpData = reinterpret_cast<LPSTR>(Snd->buffer);
        itsnd.first->second.waveHdr.dwBufferLength = Snd->size;
        itsnd.first->second.waveHdr.dwFlags = 0;
        itsnd.first->second.waveHdr.dwLoops = 0;
        itsnd.first->second.waveHdr.dwFlags = 0;
        waveOutPrepareHeader(itsnd.first->second.hWaveOut, &itsnd.first->second.waveHdr, sizeof(WAVEHDR));
    }

    return itsnd;
}


void sys::screen::Sound(WORD id)
{
    stSound* Snd = &pWaves[id - IDW_OFFSET];

    mpbtSnd it(std::find_if(Snd->vSound.begin(), Snd->vSound.end(), [](auto& snd) {return (snd.second.prepared & vtSound::Mask) == vtSound::Use; }), true);
    it.second = it.first != Snd->vSound.end();

    if (it.second && (Snd->key < Snd->max_key))
        it = OpenWave(Snd);

    if (it.second)
    {
        // DebugOut(_T("Sound              stSP : %p, vtSP : %p, Key : %03i, Id  : %03i\n"), Snd, it.first->second, it.first->second.Idx, id - IDW_OFFSET);
        it.first->second |= vtSound::FreeInUse;
        waveOutWrite(it.first->second.hWaveOut, &it.first->second.waveHdr, sizeof(WAVEHDR));
    }
    //else
       // DebugOut(_T("NO Sound\n"));

    // close too many sound handles
    for (mtSndIt its = Snd->vSound.begin(); its != Snd->vSound.end(); )
    {
        if ((its->second.prepared & vtSound::Use) == vtSound::Del)
        {
            CloseWave(its->second);
            its = Snd->vSound.erase(its);
        }
        else
            ++its;
    }

}


void CALLBACK sys::screen::WaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    //    DebugOut(_T("OutProc Msg : %u, stSP : %p, vtSP : %p, Key : %03i, Handle : %p = %p\n"), uMsg, sound->bSound, sound, sound->Idx, hwo, sound->hWaveOut);

    switch (uMsg)
    {
    case WOM_OPEN:
    case WOM_CLOSE:
        break;
    case WOM_DONE:
        vtSound* sound = reinterpret_cast<vtSound*>(dwInstance);
        if (sound->Idx > 5)   // close sound handles when over
            sound->prepared = vtSound::Del;
        else
            *sound &= vtSound::FreeInUse;
    }
}


bool sys::screen::LoadWaves()
{
    const char dataId[] = "data";

    auto LoadWave = [&](WORD id, stSound* snd) -> bool
    {
        HRSRC hRes = FindResource(hInstance, MAKEINTRESOURCE(id), _T("Wave"));
        if (hRes)
        {
            HGLOBAL hData = LoadResource(hInstance, hRes);
            snd->size = SizeofResource(hInstance, hRes);

            if (hData)
            {
                snd->buffer = reinterpret_cast<BYTE*>(LockResource(hData));
                memcpy(reinterpret_cast<BYTE*>(&snd->wfx), snd->buffer + StartHeader, sizeof(WAVEFORMATEX));

                auto it = std::search(snd->buffer, snd->buffer + snd->size, dataId, dataId + 4);
                if (it != snd->buffer + snd->size)
                {
                    DWORD* size = reinterpret_cast<DWORD*>(it + 4);
                    snd->buffer = it + 8;
                    snd->size = *size;

                    return true;
                }

                return false;
            }
        }

        return false;
    };

    for (WORD i = IDW_START; i < IDW_ENDWAV; ++i)
    {
        stSound* Snd = &pWaves[i - IDW_OFFSET];
        switch (i)
        {
        case IDW_START:
        case IDW_LEVEL:
        case IDW_SHIPEX:
        case IDW_FIRWAR:
            Snd->max_key = 1;
            break;
        case IDW_COLAST:
        case IDW_ASTHIT:
        case IDW_ASTEXP:
            Snd->max_key = 5;
            break;

        case IDW_FIRESH:
        case IDW_ASTSHL:
        case IDW_GETITE:
        case IDW_GENITE:
        default:
            Snd->max_key = 2;
            break;
        }

        if (!LoadWave(i, Snd))
        {
            userNotice(_T("Unable to Load sound."), true);
            return false;
        }

        mpbtSnd it = OpenWave(Snd);

        if (!it.second)
            return false;
    }

    return true;
}


void sys::screen::CloseWave(vtSound& vsnd)
{
    waveOutUnprepareHeader(vsnd.hWaveOut, &vsnd.waveHdr, sizeof(WAVEHDR));
    waveOutReset(vsnd.hWaveOut);
    waveOutClose(vsnd.hWaveOut);
}


void sys::screen::CleanWave()
{
    for (WORD i = IDW_START; i < IDW_ENDWAV; ++i)
    {
        stSound& Snd = pWaves[i - IDW_OFFSET];

        for (mptSnd& vsnd : Snd.vSound)
            CloseWave(vsnd.second);

        Snd.vSound.clear();
    }
}

