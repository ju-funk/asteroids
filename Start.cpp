#include "main.h"
#include "core.h"


void StartRenderView(coreInfo& core)
{
    std::lock_guard<std::mutex> lock(core.mtx);
    // loop active sprite list
    array::list<entity>::iterator i = core.sprites.begin();
    gfxBlinkStars(core);
    for (; i != core.sprites.end(); ++i)
    {
        entity* sprite = &*i;

        if((sprite->TypeEnty & entity::None) != entity::None)
            sprite->Spin();

        astWrapSprite(core, *sprite);

        // used for colour fadeout, avoid per pixel division

        // setup sprites orientation matrix
        array::matrix<float> m(sprite->rx, sprite->ry, sprite->rz);

        // process sprites vertices
        array::block<vertex>::iterator j = sprite->points.pBegin;
        for (; j != sprite->points.pEnd; ++j)
        {
            // temporary vertex to hold rotated point. copy colours

            vertex pv = *j + m;

            // scale sprite, for explosion effect
            pv *= sprite->scale;

            // offset vertex with sprite position
            pv += sprite->pos;

            // adjust z position and scale
            //pv.z = core.fZOrigin + pv.z;
            pv.x = pv.x * core.fScaleX;// / pv.z;
            pv.y = pv.y * core.fScaleY;// / pv.z;

            // convert 2D position to integer
            int px = (int)(core.iCWidth + pv.x);
            int py = (int)(core.iCHeight + pv.y);

            // compute linear address in frame buffer
            int offset = py * core.iWidth + px;

            // wrap top/bottom pixels, if outside buffer. X naturally wraps
            if (offset < 0)
            {
                // skip pixel if model is exploding
                if (sprite->scale > 1.0f)
                    continue;
                offset += core.iSize;
            }
            else if (offset >= core.iSize)
            {
                if (sprite->scale > 1.0f)
                    continue;
                offset -= core.iSize;
            }

            core.pBuffer[offset] = gfxRGB(pv.r, pv.g, pv.b);
        }
    }
}

HHOOK hhook = nullptr;
HHOOK hhookIg = nullptr;
HWND SetupDlg = nullptr;
int  glId = 0;
TimerClass::tTimerVar flash;

using myVkMap = std::map<int, TCHAR*>;
using mpVkMap = std::pair<int, TCHAR*>;

myVkMap VkMap;

void InitVkMap()
{
    auto VkIns = [](int Vk) -> void
    {
        TCHAR buf[200];
        UINT scanCode = MapVirtualKey(Vk, MAPVK_VK_TO_VSC);
        LONG lParam = (scanCode << 16);
        lParam |= 0x01000000;
        int ret = GetKeyNameText(lParam, buf, sizeof(buf) / sizeof(TCHAR));

        size_t len = _tcslen(buf) + 1;
        if (len > 0)
        {
            TCHAR* pb = new TCHAR[len];
            _tcscpy_s(pb, len, buf);
            //VkMap.insert({ Vk, pb });
            VkMap[Vk] = pb;
        }
    };

    VkIns(VK_END);
    VkIns(VK_HOME);
    VkIns(VK_INSERT);
    VkIns(VK_DELETE);
}

void DelVkMap()
{
    for (mpVkMap it : VkMap)
        delete [] it.second;
}


LRESULT CALLBACK IgnoreProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)  //  process message
        if (wParam != WM_MOUSEMOVE)
            return TRUE;

    return CallNextHookEx(hhook, nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)  //  process message
    {
        KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            switch (pKeyboard->vkCode)
            {
            case VK_LWIN:
            case VK_RWIN:
            case VK_ESCAPE:
            case VK_SNAPSHOT:
            case VK_VOLUME_MUTE:
            case VK_VOLUME_DOWN:
            case VK_VOLUME_UP:
            case VK_LAUNCH_APP1:
            case VK_LAUNCH_APP2:
            case VK_NUMLOCK:
            case VK_SCROLL:
                break;

            default:
                PostMessage(SetupDlg, WM_USER + 10, glId, pKeyboard->vkCode);
            }
        }
        return TRUE;
    }

    return CallNextHookEx(hhook, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    int wheelDelta;

    if (nCode == HC_ACTION)  // process the message 
    {
        switch (wParam)
        {
        case WM_LBUTTONDOWN:
            PostMessage(SetupDlg, WM_USER + 10, glId, VK_LBUTTON);
            return TRUE;
        case WM_RBUTTONDOWN:
            PostMessage(SetupDlg, WM_USER + 10, glId, VK_RBUTTON);
            return TRUE;
        case WM_MBUTTONDOWN:
            PostMessage(SetupDlg, WM_USER + 10, glId, VK_MBUTTON);
            return TRUE;
        case WM_MOUSEWHEEL:
            wheelDelta = GET_WHEEL_DELTA_WPARAM(((MSLLHOOKSTRUCT*)lParam)->mouseData) > 0 ? VK_WHEELUP : VK_WHEELDO;
            PostMessage(SetupDlg, WM_USER + 10, glId, wheelDelta);
            return TRUE;
        }
    }

    return CallNextHookEx(hhook, nCode, wParam, lParam);
}

void WinFlash(HWND box, bool flash)
{
   FLASHWINFO fwi = {};

   fwi.cbSize    = sizeof(FLASHWINFO);
   fwi.hwnd      = box;
   fwi.dwFlags   = flash ? (FLASHW_ALL | FLASHW_TIMER) : FLASHW_STOP;
   fwi.dwTimeout = 0;
   fwi.uCount    = 0;

   FlashWindowEx(&fwi);
   //FlashWindow(box, TRUE);
}

void HndChkBox(HWND dlg, int Id, int keyId)
{
    const TCHAR *vi;
    TCHAR  buf[200] = {};

    if (keyId < 0)
    {  // wait press
        glId = Id;

        if (hhook == nullptr)
        {
            WinFlash(dlg, true);
            flash = 1;
            secTimer.NewTimer(flash, [dlg, Id]() {
                static bool fla = false;
                fla = !fla;
                SendDlgItemMessage(dlg, Id, BM_SETCHECK, fla ? BST_UNCHECKED : BST_CHECKED, 0);
            }, false);

            if (keyId == -1)
            {
                SetDlgItemText(dlg, Id, _T("Press a Key"));
                hhook   = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
                hhookIg = SetWindowsHookEx(WH_MOUSE_LL   , IgnoreProc  , GetModuleHandle(NULL), 0);
            }
            else
            {
                SetDlgItemText(dlg, Id, _T("Press a Mouse Button\n"));
                hhook   = SetWindowsHookEx(WH_MOUSE_LL   , MouseProc  , GetModuleHandle(NULL), 0);
                hhookIg = SetWindowsHookEx(WH_KEYBOARD_LL, IgnoreProc , GetModuleHandle(NULL), 0);
            }
        }
    }
    else
    {  // show
        SendDlgItemMessage(dlg, Id, BM_SETCHECK, BST_UNCHECKED, 0);
        switch (keyId)
        {
        case VK_MBUTTON:
            vi = _T("Middle Button");
            break;
        case VK_LBUTTON:
            vi = _T("Left Button");
            break;
        case VK_RBUTTON:
            vi = _T("Right Button");
            break;
        case VK_WHEELUP:
            vi = _T("Wheel Up");
            break;
        case VK_WHEELDO:
            vi = _T("Wheel Down");
            break;

        case VK_UP:
            vi = _T("▲");
            break;
        case VK_DOWN:
            vi = _T("▼");
            break;
        case VK_RIGHT:
            vi = _T("►");
            break;
        case VK_LEFT:
            vi = _T("◄");
            break;

        case VK_PRIOR:
            vi = _T("PAGE ▲");
            break;
        case VK_NEXT:
            vi = _T("PAGE ▼");
            break;

        case VK_END:
        case VK_HOME:
        case VK_INSERT:
        case VK_DELETE:
            vi = VkMap[keyId];
            break;

        case VK_LSHIFT:
            vi = _T("Left SHIFT");
            break;
        case VK_RSHIFT:
            vi = _T("Right SHIFT");
            break;
        case VK_LCONTROL:
            vi = _T("Left CONTROL");
            break;
        case VK_RCONTROL:
            vi = _T("Right CONTROL");
            break;
        case VK_LMENU:
            vi = _T("Left ALT");
            break;
        case VK_RMENU:
            vi = _T("Right ALT");
            break;

        case VK_APPS:
            vi = _T("APP Key");
            break;

        case VK_BROWSER_BACK:
            vi = _T("BROWSER BACK");
            break;
        case VK_BROWSER_FORWARD:
            vi = _T("BROWSER FORWARD");
            break;
        case VK_BROWSER_REFRESH:
            vi = _T("BROWSER REFRESH");
            break;
        case VK_BROWSER_STOP:
            vi = _T("BROWSER STOP");
            break;
        case VK_BROWSER_SEARCH:
            vi = _T("BROWSER SEARCH");
            break;
        case VK_BROWSER_FAVORITES:
            vi = _T("BROWSER FAVORITES");
            break;
        case VK_BROWSER_HOME:
            vi = _T("BROWSER HOME");
            break;

        case VK_MEDIA_NEXT_TRACK:
            vi = _T("MEDIA NEXT TRACK");
            break;
        case VK_MEDIA_PREV_TRACK:
            vi = _T("MEDIA PREV TRACK");
            break;
        case VK_MEDIA_STOP:
            vi = _T("MEDIA STOP");
            break;
        case VK_MEDIA_PLAY_PAUSE:
            vi = _T("MEDIA PLAY PAUSE");
            break;
        case VK_LAUNCH_MAIL:
            vi = _T("LAUNCH MAIL");
            break;
        case VK_LAUNCH_MEDIA_SELECT:
            vi = _T("LAUNCH MEDIA SELECT");
            break;
 
        default:
            UINT scanCode = MapVirtualKey(keyId, MAPVK_VK_TO_VSC);
            LONG lParam = (scanCode << 16);
            int ret = GetKeyNameText(lParam, buf, sizeof(buf) / sizeof(TCHAR));
            vi = buf;

            switch (ret)
            {
            case 0:
                _stprintf_s(buf, _T("Key = [%03i]"), keyId);
                break;
            case 1:
                _stprintf_s(buf, _T("Key = %c"), buf[0]);
                break;
            default:
                _stprintf_s(buf, _T("%s = [%03i]"), buf, keyId);
                break;
            }
            break;
        }

        SetDlgItemText(dlg, Id, vi);
    }
}



INT_PTR CALLBACK SetupDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static sys::screen::_setup *newset;
    WORD id = LOWORD(wParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        newset = reinterpret_cast<sys::screen::_setup*>(lParam);
        for(int i = IDC_KSPUP; i <= IDC_MSTOP; ++i)
            HndChkBox(hwndDlg, i, newset->Keys[i - IDC_KSPUP]);
        SetupDlg = hwndDlg;
        break;

    case WM_COMMAND:
        switch (id)
        {
        case IDOK:
            EndDialog(hwndDlg, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            break;
        case IDC_KSPUP:
        case IDC_KSPDO:
        case IDC_KRIGHT:
        case IDC_KLEFT:
        case IDC_KFIRE:
        case IDC_KSHILD:
        case IDC_KSTOP:
            HndChkBox(hwndDlg, id, -1);
            break;
        case IDC_MSPUP:
        case IDC_MSPDO:
        case IDC_MFIRE:
        case IDC_MSHILD:
        case IDC_MSTOP:
            HndChkBox(hwndDlg, id, -2);
            break;
        }
        break;

    case WM_USER+10:
        UnhookWindowsHookEx(hhookIg);
        UnhookWindowsHookEx(hhook);

        WinFlash(hwndDlg, false);
        secTimer.StopTimer(flash);
        hhook = hhookIg = nullptr;
        HndChkBox(hwndDlg, id, static_cast<int>(lParam));
        if(id >= IDC_KSPUP && id <= IDC_MSTOP)
            newset->Keys[id - IDC_KSPUP] = static_cast<int>(lParam);
        break;

    case WM_CLOSE:
        EndDialog(hwndDlg, IDCANCEL);
        break;
    }

    return FALSE;
}


void Setup(coreInfo& core)
{
    sys::screen::_setup newset;
    output.GetSetSetup(newset);

    if (output.ShowDlg(IDD_SETUP, SetupDlgProc, reinterpret_cast<LPARAM>(&newset)) == IDOK)
        output.GetSetSetup(newset, false);
}


inline bool StartHandleInput(int &state, coreInfo& core)
{
    POINT mousePos;

    if ((state & ~1) == 4)
    {   // High Score Handling
        if (keys.GetKeyState(KeyMan::eMenJump, KeyMan::MustToggle))
            state = 2;

        if (keys.GetKeyState(KeyMan::eMenEsc, KeyMan::MustToggle))
            state = 2;

        if (output.GetInputState(mousePos))
        {
            if (keys.GetKeyState(KeyMan::eMenMoLe, KeyMan::MustToggle))
                state = 2;
        }
    }
    else
    {
        if (output.GetInputState(mousePos))
        {
            if (keys.GetKeyState(KeyMan::eMenMoLe, KeyMan::MustToggle))
                return true;
        }

        if (keys.GetKeyState(KeyMan::eMenStart, KeyMan::MustToggle))
            return true;

        if (keys.GetKeyState(KeyMan::eMenSetup, KeyMan::MustToggle))
            Setup(core);

        if (keys.GetKeyState(KeyMan::eMenTogg, KeyMan::MustToggle))
        {
            output.GetTogMouse(true);
            output.Sound(IDW_GETITE);
        }

        if (keys.GetKeyState(KeyMan::eMenJump, KeyMan::MustToggle))
            state = 4;

        if (keys.GetKeyState(KeyMan::eMenFull, KeyMan::MustToggle))
            output.ToggScreen();
    }

    return false;
}


SIZE ShowText(const TCHAR* str, int x, int y, const TCHAR* Fn = nullptr, int size = 0, int Ta = 0, COLORREF cT = 0, COLORREF cB = 0, int cF = 0, bool out = true, int Bm = OPAQUE)
{
    if (Fn != nullptr)
        output.SetNewFont(Fn, size, Ta, Bm, cF, cT, cB);

    if(out)
        output.StrOutText(str, x, y);

    SIZE si = output.GetTextSize(str);
    if (out)
    {
        if (Ta != TA_CENTER)
            si.cx += x;
        else
            si.cx = x + si.cx / 2;
        si.cy += y;
    }

    return si;
}


void ShowHiEntry(int x, int y, const TCHAR* pszFmt, ...)
{
    va_list ptr;
    va_start(ptr, pszFmt);
    TCHAR buf[300];
    buf[0] = 0;
    _vstprintf_s(buf, 300, pszFmt, ptr);
    ShowText(buf, x, y);
    va_end(ptr);
}



INT_PTR CALLBACK InputDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static TCHAR *buf = nullptr;
    HWND hEdit;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        buf = reinterpret_cast<TCHAR*>(lParam);
        SetDlgItemText(hwndDlg, IDC_EDNAME, _T("Player"));
        hEdit = GetDlgItem(hwndDlg, IDC_EDNAME);
        SetFocus(hEdit);
        SendMessage(hEdit, EM_SETSEL, 0, -1);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            if (buf != nullptr)
            {
                GetDlgItemText(hwndDlg, IDC_EDNAME, buf, coreInfo::HiScoreEntry::MaxName);

                if (buf[0] != 0)
                    EndDialog(hwndDlg, IDOK);
            }
            else
                EndDialog(hwndDlg, IDOK);
            return TRUE;
        }
        case IDCANCEL:
            return TRUE;
        }
        break;

    case WM_CLOSE:
        return TRUE;
    }
    return FALSE;
}

int InsertHiSore(coreInfo& core)
{
    coreInfo::vtHiScIt it = std::find_if(core.vHiScore.begin(), core.vHiScore.end(), [&core] (coreInfo::HiScoreEntry& hs) {
        return hs.Score < core.Score;
       });

    int idx = static_cast<int>(it - core.vHiScore.begin());
    int len = static_cast<int>(core.vHiScore.size());

    if (len < coreInfo::HiScoreEntry::MaxList && (idx <= -1))
        idx = len;

    if (idx > -1 && idx < coreInfo::HiScoreEntry::MaxList)
    {
        core.vHiScore.emplace(it, coreInfo::HiScoreEntry(core.Score));
        if (core.vHiScore.size() > coreInfo::HiScoreEntry::MaxList)
            core.vHiScore.pop_back();
    }
    else
        idx = -1;

    return idx;
}



void ShowHiScore(coreInfo& core, int HiIdx)
{
    output.GetSetEscDis(true);

    SIZE si, size = ShowText(_T(" High Score "), core.iCWidth, 5, _T("Segoe Script"), 90, TA_CENTER, RGB(120, 30, 200), RGB(30, 100, 200), 3);
    si = ShowText(_T(" F4 Back to Start screen "), core.iCWidth, 0, _T("Arial"), 22, TA_CENTER, RGB(32, 32, 32), RGB(30, 120, 120), 1, false);
    si.cy += si.cy / 3;
    int step, stay, endy = core.iHeight - si.cy * 2;
    ShowText(_T(" Back to Start screen with F4 or ESC or Left Mouse-Button"), core.iCWidth, core.iHeight - si.cy, _T("Arial"), 22, TA_CENTER, RGB(32, 32, 32), RGB(30, 120, 120), 1);

    si = ShowText(_T(" Name "), core.iCWidth, 5, _T("Comic Sans MS"), 25, TA_CENTER, RGB(160, 0, 255), RGB(0, 0, 0), 1, false, TRANSPARENT);
    si.cy += si.cy / 3;
    size.cy += si.cy*2;
    
    if (core.Score > 0)
        ShowHiEntry(core.iCWidth, size.cy, _T(" Your Score : %u"), core.Score);

    size.cy += static_cast<long>(si.cy*2.5f);
    stay = size.cy;

    step = (endy - stay) / coreInfo::HiScoreEntry::MaxList;

    if (HiIdx > -1)
    {
        int lin = stay + step * HiIdx - static_cast<int>(si.cy * 0.1f);
        RECT rect = { 0, lin, core.iWidth, lin + static_cast<int>(si.cy * 1.18f)};

        output.SetRect(&rect, RGB(64, 0, 0));
    }

    int len = static_cast<int>(core.vHiScore.size());

    for (int i = 0; i < len; ++i)
    {
        TCHAR *txt = core.vHiScore[i].str;

        if(i == 0)
            ShowText(txt, 10, stay + step * i, _T("Comic Sans MS"), 25, TA_LEFT, RGB(0, 95, 255), RGB(0, 0, 0), 2, true, TRANSPARENT);
        else
            ShowText(txt, 10, stay + step * i);
    }

    for (int i = 0; i < len; ++i)
    {
        std::time_t now_c = std::chrono::system_clock::to_time_t(core.vHiScore[i].tipo);
        std::tm local_tm;
        localtime_s(&local_tm, &now_c);

        if(i == 0)
            ShowText(_T(" "), core.iCWidth, stay + step * i, _T("Comic Sans MS"), 25, TA_CENTER, RGB(0, 95, 255), RGB(0, 0, 0), 2, false, TRANSPARENT);
        ShowHiEntry(core.iCWidth, stay + step * i, _T("%02d.%02d.%04d, %02d:%02d"), local_tm.tm_mday, local_tm.tm_mon + 1, local_tm.tm_year + 1900, local_tm.tm_hour, local_tm.tm_min);
    }

    for (int i = 0; i < len; ++i)
    {
        if(i == 0)
            ShowText(_T(" "), core.iWidth - 10, stay + step * i, _T("Comic Sans MS"), 25, TA_RIGHT, RGB(0, 95, 255), RGB(0, 0, 0), 2, false, TRANSPARENT);
        ShowHiEntry(core.iWidth - 10, stay + step * i, _T("%lu"), core.vHiScore[i].Score);
    }

    if ((HiIdx > -1) && core.vHiScore[HiIdx].IsNameNotSet())
    {
        output.flipBuffers();
        output.ShowDlg(IDD_INPUT, InputDlgProc, reinterpret_cast<LPARAM>(core.vHiScore[HiIdx].str));

        core.SaveHiScore();
    }
}



void ViewText(coreInfo& core, float posx1, float posy1, float posx2, float step)
{
    const TCHAR *tit = output.GetTitle();

    output.GetSetEscDis(false);

    SIZE si, size = ShowText(_T(" Astroids "), core.iCWidth, 5, _T("Segoe Script"), 120, TA_CENTER, RGB(120, 30, 200), RGB(30, 100, 200), 3);
    si = ShowText(tit, core.iCWidth, size.cy + 2, _T("Segoe Script"), 18, TA_CENTER, RGB(192, 192, 192), RGB(0, 0, 0), 1, false, TRANSPARENT);
    ShowText(tit, core.iCWidth + si.cx / 2, size.cy + 2);
    size.cy += si.cx / 3;
    si = ShowText(_T("Start"), core.iCWidth, 0, _T("Arial"), 22, TA_CENTER, RGB(200, 150, 30), RGB(0, 0, 0), 0, false, TRANSPARENT);
    si.cy += si.cy / 3;
        
    TCHAR tbu1[50] = _T("Start with F2 or Left Mouse-Button"), tbu2[50];
    TCHAR tbu3[80] = _T(" F3 Setup | F4 High Score | ESC Exit | F5 switch to ");

    if (output.GetFullScr())
    {
        if(!output.GetTogMouse())
            _tcscpy_s(tbu1, _T("Start with F2"));

        _tcscpy_s(tbu2, output.GetTogMouse() ? 
            _T("F6 toggle  Key  / [Mouse]") :
            _T("F6 toggle [Key] /  Mouse ")
        );

        _tcscat_s(tbu3, _T("Window "));
        ShowText(tbu2, core.iCWidth, core.iHeight - si.cy*2, _T("Arial"), 22, TA_CENTER, RGB(200, 150, 30), RGB(0, 0, 0), 0);
    }
    else
    {
        ShowText(_T("Remove Mouse out of the Window for Key-Mode"), core.iCWidth, core.iHeight - si.cy * 2, _T("Arial"), 21, TA_CENTER, RGB(200, 150, 30), RGB(0, 0, 0), 0);
        _tcscat_s(tbu3, _T("Fullscreen "));
    }

    ShowText(tbu1, core.iCWidth, size.cy, _T("Comic Sans MS"), 37, TA_CENTER, RGB(200, 150, 30), RGB(0, 0, 0), 3);
    ShowText(tbu3, core.iCWidth, core.iHeight - si.cy, _T("Arial"), 22, TA_CENTER, RGB(32, 32, 32), RGB(30, 120, 120), 1);


    auto Xy = [&core](float xy, bool x = true) -> int
    {
        if(x)
            return core.iCWidth  + static_cast<int>(xy * core.fScaleX);
        else
            return core.iCHeight + static_cast<int>(xy * core.fScaleY);
    };

    size = ShowText(_T("Ship"), 0, 0, _T("Arial"), 18, TA_LEFT, RGB(20, 230, 30), RGB(0, 0, 0), 3, false, TRANSPARENT);
    posx1 += step / 1.6f;
    posx2 += step / 2.0f;
    posy1 -= size.cy / (2 * core.fScaleY);
    ShowText(_T("Ship"), Xy(posx1), Xy(posy1, false));
    ShowText(_T("Ship with Shild"), Xy(posx2), Xy(posy1, false));
    posy1 += step;
    ShowText(_T("Astroids"), Xy(posx1), Xy(posy1, false));
    ShowText(_T("Bullet"), Xy(posx2), Xy(posy1, false));
    posy1 += step;
    ShowText(_T("Bonus bullet (50)"), Xy(posx1), Xy(posy1, false));
    ShowText(_T("Fire-Gun (30 sec)"), Xy(posx2), Xy(posy1, false));
    posy1 += step;
    ShowText(_T("Bonus shild (30 sec)"), Xy(posx1), Xy(posy1, false));
    ShowText(_T("Extra Life"), Xy(posx2), Xy(posy1, false));
    posy1 += step;
    ShowText(_T("You can Stop ship"), Xy(posx1), Xy(posy1, false));
}




bool ShowStart(coreInfo& core)
{
    coreInfo coreSt, coreHS;

    output.Sound(IDW_STARTS);

    astDeallocSprites(core);
    astDeallocSprites(coreSt);
    astDeallocSprites(coreHS);

    entity starfield(core.models.stars, 0.0f, 0.0f, entity::None);
    coreSt.sprites.push_back(starfield);
    coreHS.sprites.push_back(starfield);

    float fac  = 7.0f;
    float posxm1, posx = -core.fSWidth + fac*2.0f;
    float posys1, posy = -core.fSHeight + fac*3.5f;
    float posx1 = fac*3.5f;
    posys1 = posy;
    posxm1 = posx + fac*0.75f;

    entity player(core.models.ship, posxm1, posy, entity::Ship);
    coreSt.sprites.push_back(player);

    entity splayer(core.models.shild, posx1, posy, entity::Shild);
    coreSt.sprites.push_back(splayer);

    posy += fac;
    entity bullet(core.models.misile, posx1 , posy, entity::Fire);
    bullet.setDir(1.2f);
    coreSt.sprites.push_back(bullet);

    entity as1(core.models.stroidBig, posx - fac * 0.11f, posy, entity::Astro);
    as1.setDir(1.3f);
    coreSt.sprites.push_back(as1);
    as1.pos.x =  posx + fac;
    as1.pos.y =  posy - fac*3.8f;
    coreHS.sprites.push_back(as1);
    as1.pos.x =  -posx - fac;
    as1.pos.y =  posy - fac*3.8f;
    coreHS.sprites.push_back(as1);

    entity as2(core.models.stroidMed, posxm1, posy, entity::Astro);
    as2.setDir(1.5f);
    coreSt.sprites.push_back(as2);

    entity as3(core.models.stroidTiny, posx + fac * 0.42f, posy + fac * 0.4f, entity::Astro);
    as3.setDir(1.7f);
    coreSt.sprites.push_back(as3);


    posy += fac;
    posx += fac * 0.75f;

    entity it1(core.models.ItemFire, posx, posy, entity::ItFire);
    it1.setDir(2.5f);
    coreSt.sprites.push_back(it1);

    entity it2(core.models.ItemFireGun, posx1, posy, entity::ItFireGun);
    it2.setDir(2.0f);
    coreSt.sprites.push_back(it2);

    posy += fac;

    entity it3(core.models.ItemShild, posx, posy, entity::ItShild);
    it3.setDir(1.9f);
    coreSt.sprites.push_back(it3);
    
    entity it4(core.models.ItemShip, posx1, posy, entity::ItShip);
    it4.setDir(1.9f);
    coreSt.sprites.push_back(it4);

    posy += fac;

    entity it5(core.models.ItemShipStop, posx, posy, entity::ItShipStop);
    it5.setDir(2.3f);
    coreSt.sprites.push_back(it5);

    int idx, state = 2;
    if (core.Score > 0)
    {
        core.Score += (core.Fires / 10) * 10;
        state = 4;

        idx = InsertHiSore(core);
    }
    else
        idx = -1;

    while (!bHasTermSignal)
    {
        if(StartHandleInput(state, core))
            break;

        if (state == 4)
            core.sprites = coreHS.sprites;
        else if (state == 2)
            core.sprites = coreSt.sprites;

        state |= 1;

        StartRenderView(core);

        if ((state & ~1) == 4)
            ShowHiScore(core, idx);
        else 
            ViewText(core, posxm1, posys1, posx1, fac);

        // blit frame and clear backbuffer
        output.flipBuffers();
        output.clearBuffer();

        // let other processes have cpu time
        Sleep(10);
    }

    astDeallocSprites(core);
    output.SetNewFont(_T("Arial"));

    output.GetSetEscDis(true);

    output.Sound(IDW_STARTS, true);

    return !bHasTermSignal;
}
