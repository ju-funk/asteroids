#include "main.h"
#include "core.h"
#include <TCHAR.h>

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
            pv.x = pv.x * core.fScaleX;// / pv.z;
            pv.y = pv.y * core.fScaleY;// / pv.z;

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
    _stprintf_s(text, _T("%li ▲:%i ●:%li L:%i ۝ :%i s"), core.Score, core.Ships, core.Fires, core.iGameLevel-1, si);
    TextOut(core.hDC, core.iCWidth, 2, text, static_cast<int>(_tcslen(text)));
}

int coreMainThread( )
{
    // setup output parameters
    coreInfo core;
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
    core.hDC    =  output.Get_DC();

    core.Score =
    core.Fires = 0;
    core.Ships = 0;
    core.ShlTime = 3; 
    core.ShlTiDel = 10;


    // check if load succeeded
    output.setVisible(true);
    output.clearBuffer();

    if ( !coreLoaderThread(core) )
    {
        // something went wrong
        bHasTermSignal = true;
    }
    else
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
            Sleep( 10 );
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

    // seed random number generator
    srand( sys::getSeed() );

    // make ship model
    scale = 1.0f;
    models.ship.Sets(gfxGenShip(plist, scale, 0.08f, false), scale);

    models.shild.Sets(gfxGenShip(plist, scale, 0.08f, true), scale);

    // make misile model, store end points
    scale = 0.5f;
    vertex colour = { 1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.5f };
    models.misile.Sets(gfxGenAsteroid(plist, scale, 5.0f, colour), scale);

    // make tiny asteroid model
    scale = 1.0f;
    colour.r = 0.2f;  colour.g = 0.3f;  colour.b = 0.2f;
    models.stroidTiny.Sets(gfxGenAsteroid(plist, scale, 10.0f, colour), scale);

    // make medium asteroid model
    scale = 2.0f;
    colour.r = 0.2f;  colour.g = 0.3f;  colour.b = 0.4f;
    models.stroidMed.Sets(gfxGenAsteroid(plist, scale, 15.0f, colour), scale);

    // make large asteroid model
    scale = 3.0f;
    colour.r = 0.5f;  colour.g = 0.3f;  colour.b = 0.3f;
    models.stroidBig.Sets(gfxGenAsteroid(plist, scale, 20.0f, colour), scale);

    // generate starfield
    scale = 1.0f;
    models.stars.Sets(gfxGenStars(plist, static_cast<int>(1.65e-4 * core.iWidth * core.iHeight)), scale);

    // all models generated, convert to linear array
    if(!core.points.init(plist))
        return coreBadAlloc();

    // set ship pointers into linear array
    models.ship.Copy(core.points.begin());
    models.shild.Copy(models.ship.pEnd);

    // setup misile model pointers
    models.misile.Copy(models.shild.pEnd);

    // setup asteroid pointers
    models.stroidTiny.Copy(models.misile.pEnd);
    models.stroidMed.Copy(models.stroidTiny.pEnd);
    models.stroidBig.Copy(models.stroidMed.pEnd);

    // setup star pointers
    models.stars.Copy(models.stroidBig.pEnd);

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
