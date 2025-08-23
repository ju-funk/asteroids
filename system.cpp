#include "main.h"

// ------------------------------------------------------------------
// free function: userNotice - displays standard windows dialog
// ------------------------------------------------------------------
void sys::userNotice( const TCHAR *szMessage, bool isFatal )
{
    unsigned long int iType = MB_OK|MB_TOPMOST;
    const TCHAR *szCaption;

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
// free function: userQuery - displays standard windows dialog
// ------------------------------------------------------------------
bool sys::userQuery( const TCHAR *szMessage )
{
    // get user response
    int iResult = MessageBox( 0, szMessage, _T("dila/2006"), MB_YESNO|MB_TOPMOST|MB_ICONQUESTION );

    // return user selection
    if ( iResult == IDYES )  return true;
    else  return false;
}

// ------------------------------------------------------------------
// free function: getSeed - uses native api's to get random seed
// ------------------------------------------------------------------
unsigned long sys::getSeed( void )
{
    // get clock ticks, remove higher bytes
    unsigned long int iLow = GetTickCount() & 0xFFFF;

    // retrieve system clock
    SYSTEMTIME lpTime = { 0 };
    GetLocalTime( &lpTime );

    // get clock milloseconds
    unsigned long int iHigh = lpTime.wMilliseconds;

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

// ------------------------------------------------------------------
// screen object: winDlgProc - static event handling proceedure
// ------------------------------------------------------------------
LRESULT CALLBACK sys::screen::winDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
    case WM_KEYDOWN:
        if ( wParam != VK_ESCAPE )  break;
        // handle escape key as close event

    case WM_CLOSE:
        // if threads are running, signal quit
        if ( !bHasTermSignal )
        {
            bHasTermSignal = true;
            ShowWindow( hWnd, SW_HIDE );
        } else {
            // if threads have finished, kill the dialog
            screen *owner = (screen *)wParam;
            owner->cleanup();
        }
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

// ------------------------------------------------------------------
// screen object: constructor - create window and set fullscreen
// ------------------------------------------------------------------
sys::screen::screen( const TCHAR *szCaption, int width, int height, bool fullScreen )
: wasInitialized(false), hasFullScreen(false), iWidth(width), iHeight(height)
{
    // change display mode if required
    if ( fullScreen )
    {
        if ( !toggleFullScreen() )
        {
            // revert to windowed mode on failure
            userNotice( _T("Sorry, the display mode could not be changed.\nWindowed mode will be used."), false );
            fullScreen = false;
        }
    }

    // create the window, verify success
    bool bResult = create( fullScreen, !fullScreen, !fullScreen );
    if ( !bResult )  return;

    // set caption text
    setCaption( szCaption );

    // set success flag
    wasInitialized = true;
}

// ------------------------------------------------------------------
// screen object: cleanup - deallocate desktop window resources
// ------------------------------------------------------------------
void sys::screen::cleanup( void )
{
    // reset display mode
    if ( hasFullScreen )  toggleFullScreen();

    // cleanup gdi buffer
    SelectObject( hVideoDC, hOldObject );
    DeleteObject( hBitmap );
    DeleteDC( hVideoDC );

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
    BOOL iResult = GetMessage( &wMsg, 0, 0, 0 );

    // handle errors, according to msdn
    if ( iResult == -1 )
    {
        userNotice( _T("Window message pump failure."), true );
        return false;
    }

    // recieved quit message
    if ( !iResult )
    {
        // must return exit code, as described by MSDN
        iExitCode = (int)wMsg.wParam;

        // signal quit event
        bHasTermSignal = true;
        return false;
    }

    // let windows handle the message
    TranslateMessage( &wMsg );
    DispatchMessage( &wMsg );

    return true;
}

// ------------------------------------------------------------------
// screen object: create - create window and video buffer
// ------------------------------------------------------------------
bool sys::screen::create( bool topMost, bool hasCaption, bool scrCenter )
{
    // fill class struct
    WNDCLASSEX wClass = { 0 };
    wClass.cbSize = sizeof(WNDCLASSEX);
    wClass.lpfnWndProc = winDlgProc;
    wClass.hInstance = hInstance = GetModuleHandle(0);
    wClass.lpszClassName = szClass = (TCHAR *) _T("dilaDemo");
    wClass.hCursor = LoadCursor( 0, IDC_ARROW );

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
    DWORD dwStyle = WS_POPUP|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_MAXIMIZE;
    if ( hasCaption )
        dwStyle = WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;

    // get window to caluclate exact size of window based on style
    if ( !AdjustWindowRectEx(&rSize, dwStyle, 0, 0) )
    {
        UnregisterClass( szClass, hInstance );
        userNotice( _T("Could not set correct window dimensions."), true );
        return false;
    }

    // set true height and width of window area
    rSize.right -= rSize.left;  rSize.bottom -= rSize.top;

    // if centered, set top/left
    if ( scrCenter )
    {
        // get screen center
        int cx = GetSystemMetrics( SM_CXSCREEN );
        int cy = GetSystemMetrics( SM_CYSCREEN );

        if ( !cx || !cy )
        {
            UnregisterClass( szClass, hInstance );
            userNotice( _T("Could not determine current screen dimensions."), true );
            return false;
        }

        // center window area
        rSize.left = cx/2 - iWidth/2;
        rSize.top = cy/2 - iHeight/2;
    }
    else
    {
        // set top left of screen
        rSize.left = rSize.top = 0;
    }

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
    hDC = GetDC( hWnd );

    if ( !hDC )
    {
        DestroyWindow( hWnd );
        UnregisterClass( szClass, hInstance );
        userNotice( _T("Could not obtain window device handle."), true );
        return false;
    }

    // force desktop zpos, if required
    if ( topMost )
        SetWindowPos( hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );

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
    pBmpEnd = pBitmap + iWidth*iHeight;
    iBmpSize = iWidth*iHeight*4;

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

    return true;
}

// ------------------------------------------------------------------
// screen object: toggleFullScreen - change display resolution
// ------------------------------------------------------------------
bool sys::screen::toggleFullScreen( void )
{
    // set fullscreen mode
    if ( !hasFullScreen )
    {
        // initialize device configuration
        dmOriginalConfig.dmSize = sizeof(DEVMODE);
        dmOriginalConfig.dmDriverExtra = 0;

        // retrieve current configuration, so that it may be restored
        BOOL bResult = EnumDisplaySettings( 0, ENUM_CURRENT_SETTINGS, &dmOriginalConfig );

        // verify success
        if ( !bResult )  return false;

        // duplicate current config, and adjust resolution
        DEVMODE dmNewConfig = dmOriginalConfig;
        dmNewConfig.dmPelsWidth = iWidth;
        dmNewConfig.dmPelsHeight = iHeight;

        // test if the display mode is valid
        LONG iResult = ChangeDisplaySettings( &dmNewConfig, CDS_TEST );
        if ( iResult != DISP_CHANGE_SUCCESSFUL )  return false;

        // if the display test succeeded, change the resolution
        iResult = ChangeDisplaySettings( &dmNewConfig, CDS_FULLSCREEN );

        // verify success
        if ( iResult != DISP_CHANGE_SUCCESSFUL )  return false;

        // hide system cursor
        ShowCursor( false );

        // change success, set status flag
        hasFullScreen = true;
    }
    else  // restore original configurtion
    {
        // attempt to restore previous configuration
        LONG iResult = ChangeDisplaySettings( &dmOriginalConfig, CDS_RESET );

        // verify display mode was restored
        if ( iResult != DISP_CHANGE_SUCCESSFUL )
        {
            // restore display with registry settings
            ChangeDisplaySettings( 0, 0 );
        }

        // display cursor, set status flag
        ShowCursor( true );
        hasFullScreen = false;
    }

    // return success
    return true;
}

// ------------------------------------------------------------------
// screen object: setCaption - changes window caption bar text
// ------------------------------------------------------------------
void sys::screen::setCaption( const TCHAR *szCaption )
{
    // update window caption text
    SetWindowText( hWnd, szCaption );
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
// screen object: redrawWindow - draw buffer to window area
// ------------------------------------------------------------------
void sys::screen::redrawWindow( const int &x, const int &y, const int &width, const int &height )
{
    BitBlt( hDC, x, y, width, height, hVideoDC, x, y, SRCCOPY );
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

// ------------------------------------------------------------------
// thread object: constructor - start thread in rquested state
// ------------------------------------------------------------------
sys::thread::thread( void *threadProc, bool startPaused, void *threadInfo )
: wasInitialized(false), bThreadState( !startPaused )
{
    // set initial run state
    DWORD dwFlags = ( startPaused ? CREATE_SUSPENDED : 0 );

    // start the thread with the arguments passed in
    hThread = CreateThread( 0, 0,
        (LPTHREAD_START_ROUTINE)threadProc, (LPVOID)threadInfo, dwFlags, &dwThread );

    // verify success
    if ( !hThread )  return;

    // set success flag
    wasInitialized = true;
}

// ------------------------------------------------------------------
// thread object: destructor - terminate thread
// ------------------------------------------------------------------
sys::thread::~thread( void )
{
    // delay thread termination
    DWORD dwResult = WaitForSingleObject( hThread, 200 );

    // if the thread does not terminate, kill it
    if ( dwResult == WAIT_TIMEOUT )
    {
        TerminateThread( hThread, 0 );
    }
}

// ------------------------------------------------------------------
// thread object: pause - pause thread execution
// ------------------------------------------------------------------
bool sys::thread::pause( void )
{
    // check current suspend count
    if ( dwPauseCount == MAXIMUM_SUSPEND_COUNT )  return false;

    // attempt to pause the thread
    dwPauseCount = SuspendThread( hThread );

    // verify success
    if ( dwPauseCount == -1 )  return false;

    return true;
}

// ------------------------------------------------------------------
// thread object: resume - resume thread execution
// ------------------------------------------------------------------
bool sys::thread::resume( void )
{
    // check if thread is already running
    if ( !dwPauseCount )  return false;

    // attempt to resume execution
    dwPauseCount = ResumeThread( hThread );

    // verify success
    if ( dwPauseCount == -1 )  return false;

    return true;
}

// ------------------------------------------------------------------
// thread object: waitTillDone - block till terminate signal received
// ------------------------------------------------------------------
void sys::thread::waitForSignal( void )
{
    // wait for thread to finish
    WaitForSingleObject( hThread, INFINITE );
}

// ------------------------------------------------------------------
// thread object: isRunning - return the threads running state
// ------------------------------------------------------------------
bool sys::thread::isRunning( void )
{
    // get thread state
    DWORD dwResult = WaitForSingleObject( hThread, 0 );

    // if the thread is still running, return false
    if ( dwResult == WAIT_OBJECT_0 )  return false;

    // else the thread is finished
    return true;
}

// ------------------------------------------------------------------
// thread object: getExitCode - retrieve the threads exit code
// ------------------------------------------------------------------
int sys::thread::getExitCode( void )
{
    DWORD dwExitCode;
    GetExitCodeThread( hThread, &dwExitCode );

    return dwExitCode;
}
