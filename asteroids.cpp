#include "main.h"
#include "core.h"

#include <math.h>

inline bool astHandleInput( coreInfo &core )
{
    // a pointer to our spaceship
    entity *spaceship = core.sprites.begin()->value;

    int ff = 1;
    POINT mousePos; ;
    if(!output.GetMousePos(mousePos))
        ff = 2;

    if ( GetAsyncKeyState(VK_SPACE) & 1 || GetAsyncKeyState(VK_LBUTTON) & ff)   // VK_MBUTTON
        if ( !astFireBullet(core) )  
            return false;

    if (ff == 1)
    {
        mousePos.y -= core.iCHeight;
        mousePos.x -= core.iCWidth;

        spaceship->rz = (float)atan2(mousePos.x, -mousePos.y);
        if (spaceship->rz < 0)
            spaceship->rz += 2 * M_PI; // Bereich 0...2Pi
    }

    if ( GetAsyncKeyState(VK_UP) )
        spaceship->speed += 0.01f;
    if ( GetAsyncKeyState(VK_DOWN) )
        spaceship->speed -= 0.01f;
    if ( GetAsyncKeyState(VK_LEFT) )
        spaceship->rz -= 0.1f;
    if ( GetAsyncKeyState(VK_RIGHT) )
        spaceship->rz += 0.1f;

    if ( GetAsyncKeyState(VK_CONTROL) || GetAsyncKeyState(VK_RBUTTON) )
        spaceship->pos.g = spaceship->speed = spaceship->pos.r = 0.0f;

    return true;
}

// ------------------------------------------------------------------
// transverse active sprites list, deallocate all entities
// ------------------------------------------------------------------
void astDeallocSprites( coreInfo &core )
{
    while ( core.sprites.size() )
    {
        entity *sprite = core.sprites.pop_back();
        delete sprite;
    }
}

// ------------------------------------------------------------------
// check and process sprite collisions
// ------------------------------------------------------------------
void astCheckCollision( coreInfo &core, entity *enta, entity *entb )
{
    // adjust true x/y positions using next direction step
    float axdist = enta->pos.x - enta->pos.r * enta->speed;
    float aydist = enta->pos.y - enta->pos.g * enta->speed;
    float bxdist = entb->pos.x - entb->pos.r * entb->speed;
    float bydist = entb->pos.y - entb->pos.g * entb->speed;

    float xdif = fabsf(axdist - bxdist);
    float ydif = fabsf(aydist - bydist);
    float maxdist = enta->points.scale + entb->points.scale;

    // temp position wrap, fixes bugs at edge of screen
    if ( xdif-core.fSWidth > maxdist )
        xdif -= core.fSWidth*2;
    if ( ydif-core.fSHeight > maxdist )
        ydif -= core.fSHeight*2;

    // has a collision occured?
    float dist = sqrtf( xdif*xdif + ydif*ydif );
    if ( dist < maxdist )
    {
        // two asteroids collide, switch direction vectors
        if ( enta->isAsteroid  &&  entb->isAsteroid )
        {
            enta->swapDir( entb );
            enta->swapSpeed( entb );

            // prevent asteroids from getting locked together
            float oldspeed = enta->speed;
            enta->speed = (maxdist - dist);
            enta->updatePos();
            enta->speed = oldspeed;
        }
        else // other collisions cause damage to entities
        {
            // if the first entity is dead, start exploding
            if ( !--enta->health )
            {
                enta->canCollide = false;
                enta->scale += 0.1f;
                enta->pos.z = -10.0f;
            }

            // same with the second entity
            if ( !--entb->health )
            {
                entb->canCollide = false;
                entb->scale += 0.1f;
                entb->pos.z = -10.0f;
            }
        }
    }
}

// ------------------------------------------------------------------
// spawn bullet sprite from current position using ships direction
// ------------------------------------------------------------------
bool astFireBullet( coreInfo &core )
{
    // copy angle from players space ship
    // allocate new sprite using bullet model
    entity *ship = core.sprites.begin()->value;
    entity *bullet = new entity( core.models.misile, 0.0f, 0.0f );
    if ( !bullet )  
        return false;

    // copy position
    bullet->pos = ship->pos;

    // prevent bullet colliding with ship
    bullet->speed = core.models.ship.scale;
    bullet->addDir( ship->rz );
    bullet->addPos();

    // set actual speed and set direction
    bullet->speed = 0.7f;
    bullet->setDir( ship->rz );

    // add bullet to active sprite list
    if ( !core.sprites.push_back(bullet) )
        return false;

    return true;
}

// ------------------------------------------------------------------
// spawn asteroids in a circle arround point based on old model type
// ------------------------------------------------------------------
bool astSpawnStroids( coreInfo &core, model *type, vertex &where )
{
    coreInfo::modPtrs &models = core.models;

    // choose sprite details based on old model type
    model newType = core.models.stroidTiny;
    int iSpawnCount = 3, iHealth = 1;
    float typeScale = models.stroidTiny.scale + 1.0f;

    if ( type == 0 ) // spawn large asteroids
    {
        newType = models.stroidBig;
        typeScale = 15.0f;
        iSpawnCount = core.iGameLevel;
        iHealth = 3;
    }
    else if ( type->pBegin == models.stroidBig.pBegin )
    {
        newType = models.stroidMed;
        typeScale = models.stroidMed.scale + 1.0f;
        iSpawnCount = 2; iHealth = 2;
    }
    // else model is tiny asteroid

    float posrad = 2*M_PI*frand(), posinc = 2*M_PI / iSpawnCount;
    for ( int i = 0; i < iSpawnCount; posrad+=posinc, ++i )
    {
        // prevent collision between asteroids
        // by generating cicle coords
        float xpos = where.x + cosf(posrad) * typeScale;
        float ypos = where.y + sinf(posrad) * typeScale;

        // make asteroid sprite using player values
        entity *sprite = new entity( newType, xpos, ypos );
        if ( !sprite )
            return false;
        sprite->isAsteroid = true;
        sprite->speed = 0.1f + 0.2f * frand();
        sprite->health = iHealth;

        // set random direction, add to sprite list
        sprite->setDir( posrad );
        if ( !core.sprites.push_back(sprite) )
            return false;
    }

    return true;
}

// ------------------------------------------------------------------
// game variable update function, called before screen is redrawn
// ------------------------------------------------------------------
bool astUpdateState( coreInfo &core )
{
    // check for game over
    entity *ship = core.sprites.begin()->value;
    if ( !ship->health )
    {
        if ( !astNewGame(&core) )
            return coreBadAlloc();
        else
            return true;
    }

    // handle key events
    if ( !astHandleInput(core) )
        return coreBadAlloc();

    // move ship
    ship->addDir( ship->rz );
    ship->addPos();

    // process all other sprites
    int astCount = 0;
    array::list<entity*>::iterator i = core.sprites.begin();
    for ( ++i, ++i; i != core.sprites.end(); ++i )
    {
        entity *sprite = *i;

        // count number of asteroids
        if ( sprite->isAsteroid )
            ++astCount;

        // check if entity is exploding
        if ( sprite->scale > 1.0f )
        {
            if ( sprite->scale > 5.0f )
            {
                // entity exploded, remove from list
                delete sprite;
                core.sprites.remove( i );
            }
            else  // entity still exploding
            {
                // if dead, but not yet replaced with smaller asteroids
                if ( !sprite->health  && sprite->isAsteroid )
                {
                    // replace with smaller asteroids
                    if ( sprite->points.pBegin != core.models.stroidTiny.pBegin )
                    {
                        bool bResult = astSpawnStroids( core, &sprite->points, sprite->pos );
                        if ( !bResult )
                            return coreBadAlloc();
                    }

                    // set "children spawned" flag
                    sprite->health = -1;
                }
                // keep exploding
                sprite->scale += 0.1f;
            }

            // next entity, this one is doomed
            continue;
        }

        // spin the model a bit
        sprite->rx += sprite->pos.r/(70-sprite->speed);
        sprite->ry += sprite->pos.g/(70-sprite->speed);
        sprite->rz += 0.005f;

        // adjust position
        sprite->updatePos();
    }

    // if no asteroids left, level up!
    if ( !astCount )
    {
        ++core.iGameLevel;
        if ( !astNewGame(&core) )
            return coreBadAlloc();
    }

    // make stars twinkle :D
    gfxBlinkStars( core );

    return true;
}

// ------------------------------------------------------------------
// reset game. add asteroids, reset health points .etc
// ------------------------------------------------------------------
bool astNewGame( coreInfo *core )
{
    // get rid of any bullets
    astDeallocSprites( *core );

    // ship entity must always be first
    entity *player = new entity( core->models.ship, 0.0f, 0.0f );
    if ( !player )
        return false;
    bool bResult = core->sprites.push_back( player );
    if ( !bResult )
        return false;

    // add starfield
    entity *starfield = new entity( core->models.stars, 0.0f, 0.0f );
    if ( !starfield )
        return false;
    starfield->canCollide = false;
    bResult = core->sprites.push_back( starfield );
    if ( !bResult )
        return false;

    // populate space
    bResult = astSpawnStroids( *core, 0, player->pos );
    if ( !bResult )
        return false;

    return true;
}

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

void astWrapSprite( coreInfo &core, entity &sprite )
{
    // wrap left/right at any depth - cartesian offset means
    // left extremity is -320.0f, and not 0.0f
    if ( sprite.pos.x < -core.fSWidth )
        sprite.pos.x += core.fSWidth*2;
    else if ( sprite.pos.x >= core.fSWidth )
        sprite.pos.x -= core.fSWidth*2;

    // wrap top/bottom
    if ( sprite.pos.y < -core.fSHeight )
        sprite.pos.y += core.fSHeight*2;
    else if ( sprite.pos.y >= core.fSHeight )
        sprite.pos.y -= core.fSHeight*2;
}

// ------------------------------------------------------------------
// entity object. constructor. initializes velocity/direction vectors
// ------------------------------------------------------------------
entity::entity( model &source, float xpos, float ypos )
{
    // set initial position
    pos.x = xpos;
    pos.y = ypos;

    // initialize zpos and direction
    pos.z = pos.r = pos.g = pos.b = 0.0f;

    // initialize model orientation and speed
    speed = rx = ry = rz = 0.0f;

    // set sprite model and scale
    scale = 1.0f;
    points = source;

    // set default flags
    canCollide = true;
    isAsteroid = false;

    // temp health point
    health = 1;
}

// ------------------------------------------------------------------
// set direction vector, held in RGB values of pos vector
// ------------------------------------------------------------------
void entity::setDir( float angle )
{
    pos.r = -sinf(angle);
    pos.g = cosf(angle);
}

// ------------------------------------------------------------------
// change dir vector. keeps original direction, resets speed
// ------------------------------------------------------------------
void entity::addDir( float angle )
{
    pos.r += -sinf(angle) * speed;
    pos.g += cosf(angle) * speed;

    // cap speed. fixes upb's crash bug
    float max = 3.0f;
    if ( pos.r > max )
        pos.r = max;
    else if ( pos.r < -max )
        pos.r = -max;

    if ( pos.g > max )
        pos.g = max;
    else if ( pos.g < -max )
        pos.g = -max;

    speed = 0.0f;
}

// ------------------------------------------------------------------
// add direction to position without speed mul - used only for ship
// ------------------------------------------------------------------
void entity::addPos( void )
{
    // ships direction also accumulates speed
    // since speed is reset, ship can't use updatePos
    // which results in 0 direction
    pos.x -= pos.r;
    pos.y -= pos.g;
}

// ------------------------------------------------------------------
// adjust sprites position by subtracting velocity vector
// ------------------------------------------------------------------
void entity::updatePos( void )
{
    // smaller coords towards screen top/left, so subtract direction
    pos.x -= pos.r * speed;
    pos.y -= pos.g * speed;
}

// ------------------------------------------------------------------
// swap direction vectors with another entity
// ------------------------------------------------------------------
void entity::swapDir( entity *with )
{
    vertex temp = with->pos;
    with->pos.r = pos.r;
    with->pos.g = pos.g;
    with->pos.b = pos.b;
    pos.r = temp.r;
    pos.g = temp.g;
    pos.b = temp.b;
}

// ------------------------------------------------------------------
// swap speeds with another entity
// ------------------------------------------------------------------
void entity::swapSpeed( entity *with )
{
    float temp = with->speed;
    with->speed = speed;
    speed = temp;
}
