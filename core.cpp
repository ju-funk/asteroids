#include "main.h"
#include "core.h"

inline void coreRenderView( coreInfo &core )
{
    // loop active sprite list
    array::list<entity*>::iterator i = core.sprites.begin();
    for ( ; i != core.sprites.end(); ++i )
    {
        entity *sprite = *i;
      
        // test for collisions between this entity and all others
        if ( sprite->canCollide )
        {
            // do not wrap coords for sprites that can't collide
            astWrapSprite( core, *sprite );

            // for every entity apart from ones already tested...
            array::list<entity*>::iterator k = i;
            for ( ++k; k != core.sprites.end(); ++k )
            {
                entity *other = *k;
                if ( !other->canCollide )
                    continue;
                astCheckCollision( core, sprite, other );
            }
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
            vertex pv = { 0.0f, 0.0f, 0.0f, j->r, j->g, j->b };

            // rotate vertex using model matrix
            pv.x = j->x*m[0] + j->y*m[1] + j->z*m[2];
            pv.y = j->x*m[3] + j->y*m[4] + j->z*m[5];
            pv.z = j->x*m[6] + j->y*m[7] + j->z*m[8];

            // scale sprite, for explosion effect
            pv.x *= sprite->scale;
            pv.y *= sprite->scale;
            pv.z *= sprite->scale;

            // offset vertex with sprite position
            pv.x += sprite->pos.x;
            pv.y += sprite->pos.y;
            pv.z += sprite->pos.z;

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

            // test depth in frame buffer
            float *pos = &core.pFrame[ offset ];
            if ( *pos > pv.z )
            {
                *pos = pv.z;
                core.pBuffer[offset] = gfxRGB( pv.r, pv.g, pv.b );
            }
        }
    }

    // reset frame buffer
    for ( float *i = core.pFrame; i != core.pFrameEnd; ++i )
        *i = 1.0f;
}

int coreMainThread( sys::screen *output )
{
    // setup output parameters
    coreInfo core;
    core.pBuffer = output->getScreenBuffer();
    core.iWidth = output->getWidth();
    core.iHeight = output->getHeight();
    core.iSize = core.iWidth*core.iHeight;
    core.iCWidth = core.iWidth / 2;
    core.iCHeight = core.iHeight / 2;
    core.fZOrigin = 1.0f;
    core.fScaleX = core.fScaleY = 10.0f;
    core.fSWidth = core.iCWidth / core.fScaleX;
    core.fSHeight = core.iCHeight / core.fScaleY;
    core.points = 0;
    core.pFrame = 0;

    // spawn loader thread
    sys::thread loader( coreLoaderThread, false, &core );
    if ( !loader )
    {
        bHasTermSignal = true;
        output->signalQuit();
        sys::userNotice( _T("Failed to spawn loader thread."), true );
        return 0;
    }

    // wait for loading to complete
    float lticker = 0.0f;
    output->setCaption( _T("dila/2006 - Loading ...") );
    output->setVisible( true );
    while ( loader.isRunning() )
    {
        gfxDrawLoader( core, lticker );
        output->flipBuffers();
        Sleep( 10 );
    }

    // check if load succeeded
    if ( !loader.getExitCode() )
    {
        // something went wrong
        bHasTermSignal = true;
    }
    else
    {
        output->setCaption( _T("dila/2006") );
        output->clearBuffer();

        // main draw loop
        while ( !bHasTermSignal )
        {
            // set game vars and redraw frame, break on failure
            if ( !astUpdateState(core) )  break;
            coreRenderView( core );

            // blit frame and clear backbuffer
            output->flipBuffers();
            output->clearBuffer();

            // let other processes have cpu time
            Sleep( 10 );
        }

        // deallocate sprite list
        bHasTermSignal = true;
        astDeallocSprites( core );
    }

    // cleanup
    delete core.points;
    delete [] core.pFrame;

    // signal thread has finished
    output->signalQuit();

    return 0;
}

int coreLoaderThread( coreInfo *core )
{
    // lists hold vertices, pointer to model struct
    array::list<vertex> plist;
    if ( !plist )
        return coreBadAlloc();
    array::list<vertex>::size_type nlast;
    coreInfo::modPtrs &models = core->models;

    // allocate frame buffer - not used for loading animation
    core->pFrame = new float[ core->iSize ];
    if ( !core->pFrame )
        return coreBadAlloc();
    core->pFrameEnd = core->pFrame + core->iSize;

    // seed random number generator
    srand( sys::getSeed() );

    // make ship model
    models.ship.scale = 1.0f;
    if ( !gfxGenShip(plist, 0.05f) )
        return coreBadAlloc();
    models.ship.npoints = plist.size();
    nlast = models.ship.npoints;

    // make misile model, store end points
    models.misile.scale = 0.5f;
    vertex colour = { 0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f };
    bool bResult = gfxGenAsteroid( plist, models.misile.scale, 5.0f, colour );
    if ( !bResult )
        return coreBadAlloc();
    models.misile.npoints = plist.size() - nlast;
    nlast += models.misile.npoints;

    // make tiny asteroid model
    models.stroidTiny.scale = 1.0f;
    colour.r = 0.2f;  colour.g = 0.3f;  colour.b = 0.2f;
    bResult = gfxGenAsteroid( plist, models.stroidTiny.scale, 10.0f, colour );
    if ( !bResult )
        return coreBadAlloc();
    models.stroidTiny.npoints = plist.size() - nlast;
    nlast += models.stroidTiny.npoints;

    // make medium asteroid model
    models.stroidMed.scale = 2.0f;
    colour.r = 0.2f;  colour.g = 0.3f;  colour.b = 0.4f;
    bResult = gfxGenAsteroid( plist, models.stroidMed.scale, 15.0f, colour );
    if ( !bResult )
        return coreBadAlloc();
    models.stroidMed.npoints = plist.size() - nlast;
    nlast += models.stroidMed.npoints;

    // make large asteroid model
    models.stroidBig.scale = 3.0f;
    colour.r = 0.5f;  colour.g = 0.3f;  colour.b = 0.3f;
    bResult = gfxGenAsteroid( plist, models.stroidBig.scale, 20.0f, colour );
    if ( !bResult )
        return coreBadAlloc();
    models.stroidBig.npoints = plist.size() - nlast;
    nlast += models.stroidBig.npoints;

    // generate starfield
    models.stars.scale = 1.0f;
    if ( !gfxGenStars(plist, 50) )
        return coreBadAlloc();
    models.stars.npoints = plist.size() - nlast;
    nlast += models.stars.npoints;

    // all models generated, convert to linear array
    core->points = new array::block<vertex>( plist );
    if ( !core->points )
        return coreBadAlloc();

    // set ship pointers into linear array
    models.ship.pBegin = core->points->begin();
    models.ship.pEnd = models.ship.pBegin + models.ship.npoints;

    // setup misile model pointers
    models.misile.pBegin = models.ship.pEnd;
    models.misile.pEnd = models.misile.pBegin + models.misile.npoints;

    // setup asteroid pointers
    models.stroidTiny.pBegin = models.misile.pEnd;
    models.stroidTiny.pEnd = models.stroidTiny.pBegin + models.stroidTiny.npoints;
    models.stroidMed.pBegin = models.stroidTiny.pEnd;
    models.stroidMed.pEnd = models.stroidMed.pBegin + models.stroidMed.npoints;
    models.stroidBig.pBegin = models.stroidMed.pEnd;
    models.stroidBig.pEnd = models.stroidBig.pBegin + models.stroidBig.npoints;

    // setup star pointers
    models.stars.pBegin = models.stroidBig.pEnd;
    models.stars.pEnd = models.stars.pBegin + models.stars.npoints;

    // all done, initialize game
    core->iGameLevel = 2;
    bResult = astNewGame( core );
    if ( !bResult )
    {
        astDeallocSprites( *core );
        return coreBadAlloc();
    }

    return true;
}

bool coreBadAlloc( void )
{
    // notify user and end thread
    sys::userNotice( _T("Insufficient sys resources: Memory allocation failed."), true );
    return false;
}
