// CORE THREAD PROCEEDURES AND PARAMETER STRUCTURES
// ------------------------------------------------------------------
#include "gfx.h"
#include "asteroids.h"

struct coreInfo
{
    // device settings
    unsigned long int *pBuffer;
    float fScaleX, fScaleY;
    float fSWidth, fSHeight;
    float fZOrigin;
    int iWidth, iHeight;
    int iCWidth, iCHeight;
    int iSize;
    HDC hDC;

    std::mutex mtx;

    // vertex array
    array::block<vertex> points;

    // list of active entities
    // the players sprite is always
    // first in the list
    array::list<entity> sprites;
    // ship is on array 1
    inline entity* GetShip() {return &*(++sprites.begin());}

    // group the models in a struct
    struct modPtrs
    {
        model stars, ship, shild, misile;
        model stroidTiny, stroidMed, stroidBig;
        model ItemFire, ItemShild, ItemShip, ItemFireGun;
    } models;

    // other game specific vars
    int iGameLevel;
    DWORD Score, Fires; 
    const DWORD cShlTime = 3, cShlTiDel = 10;
    TimerClass::tTimerVar  ShlTime, ShlTiDel, ItemTime;
    int Ships;
    bool FireGun;
};

// prototypes called by core.cpp
void gfxDrawLoader( coreInfo &info, int loop );
void gfxBlinkStars( coreInfo &core );
void astWrapSprite( coreInfo &core, entity &sprite );

bool ShowStart(coreInfo &core);

void astDeallocSprites( coreInfo &core );

// thread entry proceedures in core.cpp
int  coreMainThread( );
int  coreLoaderThread( coreInfo &core );
bool coreBadAlloc( void );
