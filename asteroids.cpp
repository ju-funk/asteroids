#include "main.h"
#include "core.h"

#include <math.h>


KeyMan keys;
TimerClass tiStayShild;
TimerClass tiNextShild;


inline void astHandleInput( coreInfo &core )
{
    // a pointer to our spaceship
    entity *spaceship = &*core.sprites.begin();
    if(spaceship->health == 0)
        return;

    int wheel;

    POINT mousePos;

    if(output.GetInputState(mousePos, wheel))
    {
        mousePos.y -= core.iCHeight + static_cast<long>(spaceship->pos.y * core.fScaleY);
        mousePos.x -= core.iCWidth + static_cast<long>(spaceship->pos.x * core.fScaleX);

        spaceship->rz = (float)atan2(mousePos.x, -mousePos.y);
        if (spaceship->rz < 0)
            spaceship->rz += M_2PI; // Bereich 0...2Pi

        if(keys.GetKeyState(VK_LBUTTON, KeyMan::MustToggle))
           astFireBullet(core);

        //if(keys.GetKeyState(VK_LBUTTON, 30, KeyMan::eKeyCtrl))    // 19 ==> typeRate, ideal 30
        //   astFireBullet(core);

        if(keys.GetKeyState(VK_MBUTTON, KeyMan::MustToggle))
            spaceship->pos.g = spaceship->pos.r = 0.0f;

        if(keys.GetKeyState(VK_RBUTTON, KeyMan::MustToggle)) 
            astShipShild(core, true);

        if((wheel & 1) == 1)
            spaceship->speed = 0.01f;
        if((wheel & 2) == 2)
            spaceship->speed = -0.01f;
    }

    if(keys.GetKeyState(VK_SPACE, KeyMan::MustToggle))
        astFireBullet(core);

    if(keys.GetKeyState(VK_UP, KeyMan::IsDown))
        spaceship->speed = 0.01f;
    if(keys.GetKeyState(VK_DOWN, KeyMan::IsDown))
        spaceship->speed = -0.01f;

    if(keys.GetKeyState(VK_LEFT, KeyMan::IsDown))
        spaceship->rz -= 0.06f;
    if(keys.GetKeyState(VK_RIGHT, KeyMan::IsDown))
        spaceship->rz += 0.06f;

    if(keys.GetKeyState(VK_CONTROL, KeyMan::IsDown, KeyMan::eKeyCtrl))
        spaceship->pos.g = spaceship->pos.r = 0.0f;
}

// ------------------------------------------------------------------
// transverse active sprites list, deallocate all entities
// ------------------------------------------------------------------
void astDeallocSprites( coreInfo &core )
{
    core.sprites.clear();
}

// ------------------------------------------------------------------
// check and process sprite collisions
// ------------------------------------------------------------------
void astCheckCollision( coreInfo &core, entity *enta, entity *entb )
{
    // adjust true x/y positions using next direction step

    float axdist;
    float aydist;

    if (enta->speed != 0.0)
    {
        axdist = enta->pos.x - enta->pos.r * enta->speed;
        aydist = enta->pos.y - enta->pos.g * enta->speed;
    }
    else
    {   // ship has no speed
        axdist = enta->pos.x - enta->pos.r;
        aydist = enta->pos.y - enta->pos.g;
    }

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
        DWORD state = enta->TypeEnty | entb->TypeEnty;

        switch (state)                           //None=1, Ship=2, Fire=4, Astro=8, Shild=16
        {
        case entity::Shild | entity::Astro:
            // enta --> shild
            enta->swapSldDir(entb, maxdist - dist);
            output.Sound(IDW_ASTSHL);
            break;

        case entity::Astro | entity::Astro:
            // two asteroids collide, switch direction vectors
            enta->swapAstDir(entb, maxdist - dist);
            output.Sound(IDW_COLAST);
            break;

        case entity::Fire | entity::Astro:
        case entity::Ship | entity::Astro:
        case entity::Ship | entity::Fire:
        case entity::Fire | entity::Fire:
            // if the first entity is dead, start exploding
            if ( !--enta->health )
            {
                enta->TypeEnty |= entity::None;
                enta->scale += 0.1f;
                enta->pos.z = -10.0f;
                output.Sound(IDW_ASTEXP);
            }
            else if(state == (entity::Fire | entity::Astro))
                output.Sound(IDW_ASTHIT);

            // same with the second entity
            if ( !--entb->health )
            {
                entb->TypeEnty |= entity::None;
                entb->scale += 0.1f;
                entb->pos.z = -10.0f;
            }

            break;

        }
    }
}

// ------------------------------------------------------------------
// spawn bullet sprite from current position using ships direction
// ------------------------------------------------------------------
void astFireBullet( coreInfo &core )
{
    if(core.Fires == 0)
        return;

    // copy angle from players space ship
    // allocate new sprite using bullet model
    entity *ship = &*core.sprites.begin();
    entity bullet( core.models.misile, 0.0f, 0.0f );

    // copy position
    bullet.pos = ship->pos;

    // prevent bullet colliding with ship
    bullet.speed = core.models.ship.scale;
    bullet.addDir( ship->rz );
    bullet.addPos();

    // set actual speed and set direction
    bullet.speed = 0.7f;
    bullet.setDir( ship->rz );
    bullet.TypeEnty = entity::Fire;
    bullet.liveTime = static_cast<DWORD>(40 / bullet.speed) + 1;
    bullet.currFire = --core.Fires;
    if (core.Fires == 2)
        output.Sound(IDW_FIRWAR);

    // add bullet to active sprite list
    core.sprites.push_back(bullet);


    output.Sound(IDW_FIRESH);
}

void astShipShild(coreInfo& core, bool shild)
{
    entity* ship = &*core.sprites.begin();
    if (shild)
    {
        if (tiNextShild.IsTime())
        {
            ship->points = core.models.shild;
            ship->TypeEnty = entity::Shild;
            tiStayShild.Start(std::bind(astShipShild, std::ref(core), false), core.ShlTime);
            tiNextShild.Start(core.ShlTiDel, true);
        }
    }
    else
    {
        std::lock_guard<std::mutex> lock(core.mtx);
        ship->points = core.models.ship;
        ship->TypeEnty = entity::Ship;
    }
}


int getShildInf(int ti)
{
    int i = tiStayShild.GetCurrTime();
    if(i > 0)
        return i;

    i = tiNextShild.GetCurrTime();

    if(i == 0)
        return ti;

    return -i;
}



// ------------------------------------------------------------------
// spawn asteroids in a circle arround point based on old model type
// ------------------------------------------------------------------
void astSpawnStroids( coreInfo &core, model *type, vertex &where )
{
    coreInfo::modPtrs &models = core.models;

    // choose sprite details based on old model type
    model newType = core.models.stroidTiny;
    int fac = core.iGameLevel - 2;
    int iSpawnCount = 3 + fac;
    int iHealth = 1 + fac;
    float typeScale = models.stroidTiny.scale + 1.0f;

    if ( type == 0 ) // spawn large asteroids
    {
        newType = models.stroidBig;
        typeScale = 15.0f;
        iSpawnCount = core.iGameLevel;
        iHealth = 3 + fac;
    }
    else if ( type->pBegin == models.stroidBig.pBegin )
    {
        newType = models.stroidMed;
        typeScale = models.stroidMed.scale + 1.0f;
        iSpawnCount = 2 + fac;
        iHealth = 2 + fac;
    }
    // else model is tiny asteroid

    float posrad = M_2PI*frand(), posinc = M_2PI / iSpawnCount;
    for ( int i = 0; i < iSpawnCount; posrad+=posinc, ++i )
    {
        // prevent collision between asteroids
        // by generating cicle coords
        float xpos = where.x + cosf(posrad) * typeScale;
        float ypos = where.y + sinf(posrad) * typeScale;

        // make asteroid sprite using player values
        entity sprite( newType, xpos, ypos );
        sprite.TypeEnty = entity::Astro;
        sprite.speed = 0.1f + (0.2f + (fac * 0.1f)) * frand();
        sprite.health = iHealth;

        // set random direction, add to sprite list
        sprite.setDir( posrad );
        core.sprites.push_back(sprite);
    }
}

// ------------------------------------------------------------------
// game variable update function, called before screen is redrawn
// ------------------------------------------------------------------
void astUpdateState( coreInfo &core )
{
    // handle key events
    astHandleInput(core);

    // check for game over
    array::list<entity>::iterator i = core.sprites.begin();
    entity *ship = &*i;
    ++i;
    if (ship->checkShip())
    {
        astNewGame(core, true);
        return;
    }

    // process all other sprites
    int astCount = 0;
    ++i;
    while(i != core.sprites.end())
    {
        entity *sprite = &*i;
        bool IsAstro = sprite->TypeEnty & entity::Astro;

        // count number of asteroids
        if (IsAstro)
            ++astCount;

        // check if entity is exploding
        if ( sprite->scale > 1.0f )
        {
            if ( sprite->scale > 5.0f )
            {
                if (IsAstro)
                {
                    core.Score += 10 * (core.iGameLevel + 2);
                    core.Fires += 3 + core.iGameLevel;
                }
                else if(sprite->currFire == 0 && core.Fires == 0)
                {
                    ship->health = 0;
                    ship->TypeEnty |= entity::None;
                    ship->scale += 0.1f;
                    ship->pos.z = -10.0f;
                }

                // entity exploded, remove from list
                i = core.sprites.erase(i);
            }
            else  // entity still exploding
            {
                // if dead, but not yet replaced with smaller asteroids
                if ( !sprite->health && IsAstro )
                {
                    // replace with smaller asteroids
                    if ( sprite->points.pBegin != core.models.stroidTiny.pBegin )
                        astSpawnStroids( core, &sprite->points, sprite->pos );

                    // set "children spawned" flag
                    sprite->health = -1;
                }
                // keep exploding
                sprite->scale += 0.1f;

                ++i;
            }

            // next entity, this one is doomed
            continue;
        }
        else
            ++i;

        sprite->Spin();

        sprite->TestLive();
    }

    // if no asteroids left, level up!
    if ( !astCount )
        astNewGame(core, false);

    // make stars twinkle :D
    gfxBlinkStars( core );
}

// ------------------------------------------------------------------
// reset game. add asteroids, reset health points .etc
// ------------------------------------------------------------------
void astNewGame( coreInfo &core, bool newgame )
{
    // get rid of any bullets

    bool restart = (core.Ships <= 1 && newgame);
    bool levelup = (core.Ships > 0 && !newgame);

    tiStayShild.Stop();
    tiNextShild.Stop();

    if (restart)
    {
        core.iGameLevel = 2;
        core.Ships = 3;
        core.Fires = 10;
        core.Score = 0;
        output.Sound(IDW_START);
        gfxDrawLoader(core, 3);
    }
    else if (levelup)
    {
        ++core.iGameLevel;
        output.Sound(IDW_LEVEL);
        gfxDrawLoader(core, 2);

        entity *ship = &*core.sprites.begin();
        if (ship->scale > 1.0 && ship->health == 1)
        {
            --core.Ships;
            if (core.Ships == 0)
            {
                astNewGame(core, true);
                return;
            }
        }
    }
    else // new ship
    {
        --core.Ships;
        gfxDrawLoader(core, 1);
        entity* ship = &*core.sprites.begin();
        entity nship(core.models.ship, 0.0f, 0.0f);
        nship.TypeEnty = entity::Ship;
        *ship = nship;
        if(core.Fires == 0) 
            core.Fires = 3 + core.iGameLevel;

        return;
    }

    astDeallocSprites(core);

    // ship entity must always be first
    entity player(core.models.ship, 0.0f, 0.0f);
    player.TypeEnty = entity::Ship;
    core.sprites.push_back(player);

    // add starfield
    entity starfield(core.models.stars, 0.0f, 0.0f);
    core.sprites.push_back(starfield);

    // populate space
    astSpawnStroids(core, 0, player.pos);
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
    TypeEnty = None;

    // temp health point
    health   = 1;
    liveTime = currFire = 0;
}


bool entity::checkShip()
{
    if (!health)
    {
        if (scale < 1.12f)
            output.Sound(IDW_SHIPEX);

        if (scale > 5.0f)
            return true;
        else
            scale += 0.02f;
    }

    // move ship
    addDir(rz);
    addPos();

    return false;
}


void entity::TestLive()
{
    if (liveTime != 0)
    {
        --liveTime;
        if (liveTime == 0)
        {
            health = 0;
            TypeEnty |= entity::None;
            scale += 1.1f;
            pos.z = -10.0f;
        }
    }
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
void entity::swapAstDir( entity *with, float Speed )
{
    vertex temp = with->pos;
    with->pos.r = pos.r;
    with->pos.g = pos.g;
    pos.r = temp.r;
    pos.g = temp.g;

    swapSpeed(with);
    // prevent asteroids from getting locked together
    float oldspeed = speed;
    speed = Speed;
    updatePos();
    speed = oldspeed;
}

void entity::swapSldDir(entity* with, float Speed)
{
    if(pos.r == 0.0f)
        pos.r = -with->pos.r / 3;

    with->pos.r = -with->pos.r;
   
    if(pos.g == 0.0f)
        pos.g = -with->pos.g / 3;

    with->pos.g = -with->pos.g;

    pos.r = -pos.r;
    pos.g = -pos.g;

    float v1 = pos.r + pos.g;
    float v2 = with->pos.r + with->pos.r;

    if (((v1 < 0.0f) && (v2 < 0.0f)) || ((v1 > 0.0f) && (v2 > 0.0f)))
    {
        if (fabsf(v1) > fabsf(v2))
        {
            with->pos.r = -with->pos.r;
            with->pos.g = -with->pos.g;
        }
        else
        {
            pos.r = -pos.r;
            pos.g = -pos.g;
        }
    }

    addDir(0.0f);
    speed = Speed;
    updatePos();
    speed = 0.0f;
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

void entity::Spin()
{
    // spin the model a bit
    rx += pos.r / (70 - speed);
    ry += pos.g / (70 - speed);
    rz += 0.005f;

    // adjust position
    updatePos();
}






bool KeyMan::GetKeyState(int Key, int Todo, int extkey)
{
    bool ret = false, down = false;

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

