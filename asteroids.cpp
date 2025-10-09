#include "main.h"
#include "core.h"

#include <math.h>


KeyMan keys;
TimerClass secTimer;
const entity::TypesEnty entity::Items[entity::MaxItems + 1] = { entity::Zero, entity::Zero, entity::Zero, entity::Zero, entity::Zero, entity::ItFire, entity::Zero, entity::Zero, entity::Zero, entity::Zero, entity::ItShild, entity::Zero, entity::Zero, entity::Zero, entity::Zero, entity::ItShip, entity::Zero, entity::Zero, entity::Zero, entity::Zero, entity::Zero, entity::ItFireGun, entity::Zero };


inline void astHandleInput( coreInfo &core )
{
    // a pointer to our spaceship
    entity *spaceship = core.GetShip();
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

        if(keys.GetKeyState(VK_LBUTTON, core.FireGun ? 30 : KeyMan::MustToggle))
           astFireBullet(core);

        //if(keys.GetKeyState(VK_LBUTTON, 30, KeyMan::eKeyCtrl))    // 19 ==> typeRate, ideal 30
        //   astFireBullet(core);

        if(keys.GetKeyState(VK_MBUTTON, KeyMan::MustToggle))
            spaceship->pos.g = spaceship->pos.r = 0.0f;

        if(keys.GetKeyState(VK_RBUTTON, KeyMan::MustToggle)) 
            astShipShild(core, true);

        if((wheel & 1) == 1)
            spaceship->speed = 0.02f;
        if((wheel & 2) == 2)
            spaceship->speed = -0.02f;
    }

    if(keys.GetKeyState(VK_SPACE, core.FireGun ? 30 : KeyMan::MustToggle))
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
        entity::TypesEnty state = *enta | *entb;

        switch (state)                           //None=1, Ship=2, Fire=4, Astro=8, Shild=16
        {
        case entity::Shild | entity::Astro:
            // enta --> shild
            enta->swapSldDir(entb, maxdist - dist);
            output.Sound(IDW_ASTSHL);
            break;

        case entity::Astro | entity::Astro:
        case entity::Astro | entity::ItFire:
        case entity::Astro | entity::ItShild:
        case entity::Astro | entity::ItShip:
        case entity::Astro | entity::ItFireGun:

        case entity::ItFire | entity::ItShild:
        case entity::ItFire | entity::ItShip:
        case entity::ItShild | entity::ItShip:

        case entity::ItFireGun | entity::ItFire:
        case entity::ItFireGun | entity::ItShip:
        case entity::ItFireGun | entity::ItShild:
            // two asteroids collide, switch direction vectors
            enta->swapAstDir(entb, maxdist - dist);
            output.Sound(IDW_COLAST);
            break;

        case entity::Fire | entity::Astro:
        case entity::Fire | entity::Fire:

        case entity::Ship | entity::Astro:
        case entity::Ship | entity::Fire:
            // if the first entity is dead, start exploding
            if ( enta->setExplore())
                output.Sound(IDW_ASTEXP);
            else if(state & (entity::Fire | entity::Astro))
                output.Sound(IDW_ASTHIT);

            // same with the second entity
            entb->setExplore();
            break;

        case entity::Shild | entity::Fire:
            entb->setExplore();
            output.Sound(IDW_ASTSHL);
            break;

        case entity::Fire | entity::ItFire:
        case entity::Fire | entity::ItShild:
        case entity::Fire | entity::ItShip:
        case entity::Fire | entity::ItFireGun:
            enta->setExplore();
            entb->setExplore();
            output.Sound(IDW_ASTEXP);
            break;

        case entity::Shild | entity::ItShild:
        case entity::Shild | entity::ItFire:
        case entity::Shild | entity::ItShip:
        case entity::Shild | entity::ItFireGun:
            entb->setExplore();
            output.Sound(IDW_ASTEXP);
            break;

        case entity::Ship | entity::ItFire:
            entb->setExplore();
            output.Sound(IDW_GETITE);
            core.Fires += 50;
            break;

        case entity::Ship | entity::ItFireGun:
            entb->setExplore();
            output.Sound(IDW_GETITE);
            core.FireGun = true;
            core.ItemTime = 30;
            secTimer.NewTimer(core.ItemTime,[&core]() {core.FireGun = false;});
            break;

        case entity::Ship | entity::ItShild:
            entb->setExplore();
            core.ShlTime  = core.ShlTime.orgTime + core.cShlTime * 10;
            core.ShlTiDel = core.ShlTime.orgTime + 1;
            output.Sound(IDW_GETITE);
            break;

        case entity::Ship | entity::ItShip:
            entb->setExplore();
            output.Sound(IDW_GETITE);
            ++core.Ships;
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
    entity *ship = core.GetShip();
    entity bullet( core.models.misile, 0.0f, 0.0f, entity::Fire);

    // copy position
    bullet.pos = ship->pos;

    // prevent bullet colliding with ship
    bullet.speed = core.models.ship.scale;
    bullet.addDir( ship->rz );
    bullet.addPos();

    // set actual speed and set direction
    bullet.speed = 0.7f;
    bullet.setDir( ship->rz );
    bullet.liveTime = core.FireGun ? 0 : static_cast<DWORD>(50 / bullet.speed) + 1;
    bullet.currFire = core.FireGun ? 1 : --core.Fires;
    if (core.Fires == 3)
        output.Sound(IDW_FIRWAR);

    // add bullet to active sprite list
    core.sprites.push_back(bullet);

    output.Sound(IDW_FIRESH);
}

void astShipShild(coreInfo& core, bool shild)
{
    entity* ship = core.GetShip();
    if (shild)
    {
        if (core.ShlTiDel.GetTime() == 0)
        {
            ship->points = core.models.shild;
            ship->TypeEnty = entity::Shild;
            secTimer.NewTimer(core.ShlTiDel, true);
            secTimer.NewTimer(core.ShlTime, std::bind(astShipShild, std::ref(core), false));
        }
    }
    else
    {
        std::lock_guard<std::mutex> lock(core.mtx);
        ship->points = core.models.ship;
        ship->TypeEnty = entity::Ship;

        core.ShlTime = core.cShlTime;
        core.ShlTiDel = core.cShlTiDel;
    }
}


int getShildInf(coreInfo& core)
{
    int i = core.ShlTime.GetTime();
    if(i > 0)
        return i;

    i = core.ShlTiDel.GetTime();

    if(i == 0)
        return core.ShlTime.orgTime;

    return -i;
}


inline entity::TypesEnty Itrand(void)
{
    entity::TypesEnty item = static_cast<entity::TypesEnty>(rand() / (RAND_MAX / entity::MaxItems));

    return entity::Items[item];
}



void astGenItems(coreInfo& core, entity::TypesEnty ty, vertex& where)
{
    int fac = core.iGameLevel - 2;
    float posrad = M_2PI * frand();

    model newType;

    switch (ty)
    {
    case entity::ItFire:
        newType = core.models.ItemFire;
        break;
    case entity::ItShild:
        newType = core.models.ItemShild;
        break;
    case entity::ItShip:
        newType = core.models.ItemShip;
        break;
    case entity::ItFireGun:
        newType = core.models.ItemFireGun;
        break;
    default:
        return;
    }

    entity Item(newType, where.x, where.y, ty);
    Item.health = 2;
    Item.speed = 0.1f + (0.2f + (fac * 0.1f)) * frand();
    Item.liveTime = static_cast<DWORD>(200 / Item.speed) + 1;
    Item.setDir(posrad);

    core.sprites.push_back(Item);
    output.Sound(IDW_START);
}


// ------------------------------------------------------------------
// spawn asteroids in a circle arround point based on old model type
// ------------------------------------------------------------------
void astSpawnStroids( coreInfo &core, model *type, vertex &where )
{
    coreInfo::modPtrs &models = core.models;
    entity::TypesEnty item = entity::Zero;

    // choose sprite details based on old model type
    model newType = models.stroidTiny;
    int fac = core.iGameLevel - 2;
    int iSpawnCount = 3 + fac;
    int iHealth = 1 + fac;
    float typeScale = models.stroidTiny.scale + 1.0f;

    if ( type == 0 ) // spawn large asteroids
    {
        newType = models.stroidBig;
        typeScale = models.stroidBig.scale * 5;
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
    else // model is tiny asteroid
        item = Itrand();


    float posrad = M_2PI*frand(), posinc = M_2PI / iSpawnCount;
    for ( int i = 0; i < iSpawnCount; posrad+=posinc, ++i )
    {
        // prevent collision between asteroids
        // by generating cicle coords
        float xpos = where.x + cosf(posrad) * typeScale;
        float ypos = where.y + sinf(posrad) * typeScale;

        // make asteroid sprite using player values
        entity sprite( newType, xpos, ypos, entity::Astro );
        sprite.speed = 0.1f + (0.2f + (fac * 0.1f)) * frand();
        sprite.health = iHealth;

        // set random direction, add to sprite list
        sprite.setDir( posrad );
        core.sprites.push_back(sprite);
    }

    posrad+=posinc;
    float xpos = where.x + cosf(posrad) * typeScale;
    float ypos = where.y + sinf(posrad) * typeScale;
    vertex who(xpos, ypos);
    astGenItems(core, item, who);
}

// ------------------------------------------------------------------
// game variable update function, called before screen is redrawn
// ------------------------------------------------------------------
void astUpdateState( coreInfo &core )
{
    // handle key events
    astHandleInput(core);

    // check for game over
    entity *ship = core.GetShip();
    if (ship->checkShip())
    {
        astNewGame(core, true);
        return;
    }

    // process all other sprites
    int astCount = 0;
    array::list<entity>::iterator i = core.sprites.begin();
    ++i, ++i;
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
                    ship->setExplore();

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

    secTimer.StopTimer();
    core.ShlTime = core.cShlTime;
    core.ShlTiDel = core.cShlTiDel;
    core.FireGun = false;


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

        entity *ship = core.GetShip();
        if (ship->scale > 1.0)
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
        entity* ship = core.GetShip();
        entity nship(core.models.ship, 0.0f, 0.0f, entity::Ship);
        *ship = nship;
        if(core.Fires == 0) 
            core.Fires = 3 + core.iGameLevel;

        return;
    }

    astDeallocSprites(core);

    // add starfield  must always be first
    entity starfield(core.models.stars, 0.0f, 0.0f, entity::None);
    core.sprites.push_back(starfield);

    // ship entity always on pos 1
    entity player(core.models.ship, 0.0f, 0.0f, entity::Ship);
    core.sprites.push_back(player);


    // populate space
    astSpawnStroids(core, 0, player.pos);

    /*
    vertex w1(5.0f, 5.0f);
    astGenItems(core, entity::ItFire, w1);

    vertex w2(-5.0f, -5.0f);
    astGenItems(core, entity::ItShild, w2);

    vertex w3(-5.0f, 5.0f);
    astGenItems(core, entity::ItShip, w3);

    vertex w4(5.0f, -5.0f);
    astGenItems(core, entity::ItFireGun, w4);
    */
}


// ------------------------------------------------------------------
// entity object. constructor. initializes velocity/direction vectors
// ------------------------------------------------------------------
entity::entity( model &source, float xpos, float ypos, TypesEnty typeEnty)
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
    TypeEnty = typeEnty;

    // temp health point
    health   = currFire = 1;
    liveTime = 0;
}


bool entity::setExplore()
{
    if (!--health)
    {
        *this |= entity::None;
        scale += 0.1f;
        pos.z = -10.0f;

        return true;
    }

    return false;
}


bool entity::checkShip()
{
    if (health <= 0)
    {
        if (scale < 1.12f)
            output.Sound(IDW_SHIPEX);

        if (scale > 5.0f)
            return true;
        else
            scale += 0.02f;

        return false;
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
            setExplore();
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

    if(((pos.r < 0.0f) && (with->pos.r < 0.0f)) && ((pos.g < 0.0f) && (with->pos.g < 0.0f)) ||
       ((pos.r > 0.0f) && (with->pos.r > 0.0f)) && ((pos.g > 0.0f) && (with->pos.g > 0.0f)) )
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

    speed = 0.1f;
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

