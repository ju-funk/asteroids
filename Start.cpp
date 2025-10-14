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

inline bool StartHandleInput()
{
    int wheel;
    POINT mousePos;

    if (output.GetInputState(mousePos, wheel))
    {
        if(keys.GetKeyState(VK_LBUTTON, KeyMan::MustToggle))
            return true;
    }

    if (keys.GetKeyState(VK_F2, KeyMan::MustToggle))
        return true;

    if (keys.GetKeyState(VK_F6, KeyMan::MustToggle))
    {
        output.GetTogMouse(true);
        output.Sound(IDW_GETITE);
    }
 
    if (keys.GetKeyState(VK_F5, KeyMan::MustToggle))
        PostMessage(output.GetWnd(), WM_USER + 1, 0, 0);

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



void ViewText(coreInfo& core, float posx1, float posy1, float posx2, float step)
{
    const TCHAR *tit = output.GetTitle();

    SIZE si, size = ShowText(_T(" Astroids "), core.iCWidth, 0, _T("Segoe Script"), 120, TA_CENTER, RGB(120, 30, 200), RGB(30, 100, 200), 3);
    si = ShowText(tit, core.iCWidth, size.cy + 2, _T("Segoe Script"), 18, TA_CENTER, RGB(192, 192, 192), RGB(0, 0, 0), 1, false, TRANSPARENT);
    ShowText(tit, core.iCWidth + si.cx / 2, size.cy + 2);
    size.cy += si.cx / 3;
    si = ShowText(_T("Start"), core.iCWidth, 0, _T("Arial"), 22, TA_CENTER, RGB(200, 150, 30), RGB(0, 0, 0), 0, false, TRANSPARENT);
    si.cy += si.cy / 3;
        
    TCHAR tbu1[50] = _T("Start with F2 or Left Mouse-Button"), tbu2[50];
    TCHAR tbu3[80] = _T(" F3 Setup | F4 High Score | F5 switch to ");

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
}




bool ShowStart(coreInfo& core)
{
    astDeallocSprites(core);

    entity starfield(core.models.stars, 0.0f, 0.0f, entity::None);
    core.sprites.push_back(starfield);

    float fac  = 7.0f;
    float posxm1, posx = -core.fSWidth + fac*2.0f;
    float posys1, posy = -core.fSHeight + fac*3.5f;
    float posx1 = fac*3.5f;
    posys1 = posy;
    posxm1 = posx + fac*0.75f;

    entity player(core.models.ship, posxm1, posy, entity::Ship);
    core.sprites.push_back(player);

    entity splayer(core.models.shild, posx1, posy, entity::Shild);
    core.sprites.push_back(splayer);

    posy += fac;
    entity bullet(core.models.misile, posx1 , posy, entity::Fire);
    bullet.setDir(1.2f);
    core.sprites.push_back(bullet);

    entity as1(core.models.stroidBig, posx - fac * 0.11f, posy, entity::Astro);
    as1.setDir(1.3f);
    core.sprites.push_back(as1);

    entity as2(core.models.stroidMed, posxm1, posy, entity::Astro);
    as2.setDir(1.5f);
    core.sprites.push_back(as2);

    entity as3(core.models.stroidTiny, posx + fac * 0.42f, posy + fac * 0.4f, entity::Astro);
    as3.setDir(1.7f);
    core.sprites.push_back(as3);


    posy += fac;
    posx += fac * 0.75f;

    vertex w1(posx, posy);
    astGenItems(core, entity::ItFire, w1, true);
    vertex w2(posx1, posy);
    astGenItems(core, entity::ItFireGun, w2, true);

    posy += fac;

    vertex w3(posx, posy);
    astGenItems(core, entity::ItShild, w3, true);
    
    vertex w4(posx1, posy);
    astGenItems(core, entity::ItShip, w4, true);

    while (!bHasTermSignal)
    {
        if(StartHandleInput())
            break;

        StartRenderView(core);

        ViewText(core, posxm1, posys1, posx1, fac);

        // blit frame and clear backbuffer
        output.flipBuffers();
        output.clearBuffer();

        // let other processes have cpu time
        Sleep(10);
    }


    astDeallocSprites(core);
    output.SetNewFont(_T("Arial"));

    return !bHasTermSignal;
}
