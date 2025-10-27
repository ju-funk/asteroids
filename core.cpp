#include "main.h"
#include "core.h"


// unused since perspective turned off
/*inline void astWrapSprite3D( coreInfo &core, entity &sprite )
{
    // compute screen extents for the sprites depth in the scene
    // 640 (screen width) * depth = visible limit at that depth
    float depth = core.fZOrigin + sprite.pos.z;
    float xlimit = (core.iCWidth*depth) / core.fScaleX;
    float ylimit = (core.iCHeight*depth) / core.fScaleY;

    // wrap left/right at any depth - cartesian offset means
    // left extremity is -320.0f, and not 0.0f
    if ( sprite.pos.x < -xlimit )
        sprite.pos.x += xlimit*2;
    else if ( sprite.pos.x >= xlimit )
        sprite.pos.x -= xlimit*2;

    // wrap top/bottom
    if ( sprite.pos.y < -ylimit )
        sprite.pos.y += ylimit*2;
    else if ( sprite.pos.y >= ylimit )
        sprite.pos.y -= ylimit*2;
}*/

void astWrapSprite(coreInfo& core, entity& sprite)
{
    // wrap left/right at any depth - cartesian offset means
    // left extremity is -320.0f, and not 0.0f
    if (sprite.pos.x < -core.fSWidth)
        sprite.pos.x += core.fSWidth * 2;
    else if (sprite.pos.x >= core.fSWidth)
        sprite.pos.x -= core.fSWidth * 2;

    // wrap top/bottom
    if (sprite.pos.y < -core.fSHeight)
        sprite.pos.y += core.fSHeight * 2;
    else if (sprite.pos.y >= core.fSHeight)
        sprite.pos.y -= core.fSHeight * 2;
}

// ------------------------------------------------------------------
// transverse active sprites list, deallocate all entities
// ------------------------------------------------------------------
void astDeallocSprites(coreInfo& core)
{
    core.sprites.clear();
}

inline void coreRenderView( coreInfo &core )
{
    std::lock_guard<std::mutex> lock(core.mtx);
    // loop active sprite list
    array::list<entity>::iterator i = core.sprites.begin();
    for ( ; i != core.sprites.end(); ++i )
    {
        entity *sprite = &*i;
      
        // test for collisions between this entity and all others
        if ( (sprite->TypeEnty & entity::None) != entity::None)
        {
            // for every entity apart from ones already tested...
            array::list<entity>::iterator k = i;
            for ( ++k; k != core.sprites.end(); ++k )
            {
                entity *other = &*k;
                if ( other->TypeEnty & entity::None)
                    continue;
                astCheckCollision( core, sprite, other );
            }

            // do not wrap coords for sprites that can't collide
            astWrapSprite( core, *sprite );
        }

        // used for colour fadeout, avoid per pixel division
        float recp = 0.0f;
        if ( sprite->scale > 1.0f )
            recp = 1/sprite->scale;

        // setup sprites orientation matrix
        array::matrix<float> m( sprite->rx, sprite->ry, sprite->rz );

        // process sprites vertices
        array::block<vertex>::iterator j = sprite->points.pBegin;
        for ( ; j != sprite->points.pEnd; ++j )
        {
            // temporary vertex to hold rotated point. copy colours
            vertex pv = *j + m;

            // scale sprite, for explosion effect
            pv *= sprite->scale;

            // offset vertex with sprite position
            pv += sprite->pos;

            // adjust z position and scale
            //pv.z = core.fZOrigin + pv.z;
            pv.x *= core.fScaleX;// / pv.z;
            pv.y *= core.fScaleY;// / pv.z;

            // if exploding, fade out colours
            if ( sprite->scale > 1.0f )
            {
                pv.r *= recp;
                pv.g *= recp;
                pv.b *= recp;
            }

            // convert 2D position to integer
            int px = (int)(core.iCWidth + pv.x);
            int py = (int)(core.iCHeight + pv.y);

            // compute linear address in frame buffer
            int offset = py * core.iWidth + px;

            // wrap top/bottom pixels, if outside buffer. X naturally wraps
            if ( offset < 0 )
            {
                // skip pixel if model is exploding
                if ( sprite->scale > 1.0f )
                    continue;
                offset += core.iSize;
            }
            else if ( offset >= core.iSize )
            {
                if ( sprite->scale > 1.0f )
                    continue;
                offset -= core.iSize;
            }

            core.pBuffer[offset] = gfxRGB( pv.r, pv.g, pv.b );
        }
    }

    const int maxs = 100;
    TCHAR text[maxs];
    int si = getShildInf(core);
    int it = core.ItemTime.GetTime();
    if(it > 0)
        _stprintf_s(text, _T("%li ▲:%i ●:%li L:%i ۝:%i s I:%i s"), core.Score, core.Ships, core.Fires, core.iGameLevel - 1, si, it);
    else
        _stprintf_s(text, _T("%li ▲:%i ●:%li L:%i ۝:%i s"), core.Score, core.Ships, core.Fires, core.iGameLevel - 1, si);
    output.StrOutText(text, core.iCWidth, 2);
}

int coreMainThread( )
{
    // setup output parameters
    coreInfo core;
    core.LoadHiScore();

    core.pBuffer = output.getScreenBuffer();
    core.iWidth = output.getWidth();
    core.iHeight = output.getHeight();
    core.iSize = core.iWidth*core.iHeight;
    core.iCWidth = core.iWidth / 2;
    core.iCHeight = core.iHeight / 2;
    core.fZOrigin = 1.0f;
    core.fScaleX = core.fScaleY = 10.0f;
    core.fSWidth = core.iCWidth / core.fScaleX;
    core.fSHeight = core.iCHeight / core.fScaleY;

    core.iGameLevel =
    core.Score =
    core.Fires = 
    core.Ships = 0;
    core.ShlTime = core.cShlTime;
    core.ShlTiDel = core.cShlTiDel;
    core.FireGun = core.ShipStop = false;


    // check if load succeeded
    if (coreLoaderThread(core))
    {
        // main draw loop
        while ( !bHasTermSignal )
        {
            // set game vars and redraw frame, break on failure
            astUpdateState(core);
            coreRenderView( core );

            // blit frame and clear backbuffer
            output.flipBuffers();
            output.clearBuffer();

            // let other processes have cpu time
            Sleep( 5 );
        }

        // deallocate sprite list
        bHasTermSignal = true;
        astDeallocSprites( core );
    }

    // signal thread has finished
    output.signalQuit();

    return 0;
}

int coreLoaderThread( coreInfo &core )
{
    // lists hold vertices, pointer to model struct
    array::list<vertex> plist;
    coreInfo::modPtrs &models = core.models;
    float scale;
    vertex colour, col1;

    // seed random number generator
    srand( sys::getSeed() );

    // generate starfield
    scale = 1.0f;
    models.stars.Sets(gfxGenStars(plist, core), scale);

    // make ship model
    scale = 1.5f;
    colour.SetColor( 0.4f, 0.9f, 0.6f );
    col1.SetColor( -1.0f, 0.0f, 0.0f );
    models.ship.Sets( gfxGenShip(plist, scale, 0.08f, colour, col1), scale);

    col1.SetColor(1.0f, 1.0f, 0.0f);
    models.shild.Sets(gfxGenShip(plist, scale, 0.08f, colour, col1), scale);

    // make misile model, store end points
    scale = 0.5f;
    colour.SetColor(0.3f, 0.0f, 0.0f);
    models.misile.Sets(gfxGenAsteroid(plist, scale, 5.0f, colour), scale);

    // make tiny asteroid model
    scale = 1.0f;
    colour.SetColor(0.2f, 0.3f, 0.2f);
    models.stroidTiny.Sets(gfxGenAsteroid(plist, scale, 10.0f, colour), scale);

    // make medium asteroid model
    scale = 2.0f;
    colour.SetColor(0.2f, 0.3f, 0.4f);
    models.stroidMed.Sets(gfxGenAsteroid(plist, scale, 15.0f, colour), scale);

    // make large asteroid model
    scale = 3.0f;
    colour.SetColor(0.5f, 0.3f, 0.3f);
    models.stroidBig.Sets(gfxGenAsteroid(plist, scale, 20.0f, colour), scale);

    // make Items
    scale = 2.57f;
    colour.SetColor(0.3f, 0.0f, 0.0f);
    models.ItemFire.Sets(gfxGenItemFire(plist, scale, 0.08f, colour), scale);

    scale = 2.57f;
    colour.SetColor(1.0f, 1.0f, 0.0f);
    models.ItemShild.Sets(gfxGenItemShild(plist, scale, 0.08f, colour), scale);

    scale = 2.57f;
    colour.SetColor(1.0f, 0.0f, 1.0f);
    models.ItemShip.Sets(gfxGenItemShip(plist, scale, 0.08f, colour), scale);

    scale = 2.57f;
    colour.SetColor(0.3f, 0.0f, 0.0f);
    models.ItemFireGun.Sets(gfxGenItemFireGun(plist, scale, 0.08f, colour), scale);

    scale = 2.57f;
    colour.SetColor(0.1f, 0.7f, 0.9f);
    models.ItemShipStop.Sets(gfxGenItemShipStop(plist, scale, 0.08f, colour), scale);

    // all models generated, convert to linear array
    if(!core.points.init(plist))
        return coreBadAlloc();
    
    // setup star pointers
    models.stars.Copy(core.points.begin());

    // set ship pointers into linear array
    models.ship.Copy(models.stars.pEnd);
    models.shild.Copy(models.ship.pEnd);

    // setup misile model pointers
    models.misile.Copy(models.shild.pEnd);

    // setup asteroid pointers
    models.stroidTiny.Copy(models.misile.pEnd);
    models.stroidMed.Copy(models.stroidTiny.pEnd);
    models.stroidBig.Copy(models.stroidMed.pEnd);

    models.ItemFire.Copy(models.stroidBig.pEnd);
    models.ItemShild.Copy(models.ItemFire.pEnd);
    models.ItemShip.Copy(models.ItemShild.pEnd);
    models.ItemFireGun.Copy(models.ItemShip.pEnd);
    models.ItemShipStop.Copy(models.ItemFireGun.pEnd);

    // all done, initialize game
    astNewGame( core, true);

    return true;
}

bool coreBadAlloc( void )
{
    // notify user and end thread
    sys::userNotice( _T("Insufficient sys resources: Memory allocation failed."), true );
    return false;
}


void coreInfo::LoadHiScore()
{
    DWORD len, cnt = 0, si;
    BYTE *dat = output.GetHiScPtr(len);

    vHiScore.clear();

    while (len > cnt)
    {
        HiScoreEntry enty(0);
        si   = enty.SetName(reinterpret_cast<TCHAR*>(dat));
        cnt += si;
        dat += si;
        si   = enty.SetDat(dat);
        cnt += si;
        dat += si;

        vHiScore.push_back(enty);
    }
}

void coreInfo::SaveHiScore()
{
    DWORD len = 0;
    BYTE *dat;

    std::for_each(vHiScore.begin(), vHiScore.end(), [&len](HiScoreEntry& ent) {
        len += (static_cast<DWORD>(_tcslen(ent.str)) + 1) * sizeof(TCHAR) + sizeof(ent.tipo) + sizeof(ent.Score);
    });

    dat = new BYTE[len];

    std::for_each(vHiScore.begin(), vHiScore.end(), [dat](HiScoreEntry& ent) {
        ent.cpyDat(const_cast<BYTE**>(&dat));
    });


    output.SetNewHiScr(dat, len);

    delete [] dat;
}
