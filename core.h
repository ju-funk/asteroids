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

    // group the models in a struct
    struct modPtrs
    {
        model ship, shild, misile, stars;
        model stroidTiny, stroidMed, stroidBig;
    } models;

    // other game specific vars
    int iGameLevel;
    DWORD Score, Fires;
    int   Ships;
};

// prototypes called by core.cpp
void gfxDrawLoader( coreInfo &info, int loop );
void gfxBlinkStars( coreInfo &core );
void astWrapSprite( coreInfo &core, entity &sprite );
void astFireBullet( coreInfo &core );
void astShipShild( coreInfo &core, bool shild);
int  getShildInf();
void astNewGame( coreInfo &core, bool newgame );
void astUpdateState( coreInfo &core );
void astDeallocSprites( coreInfo &core );
void astCheckCollision( coreInfo &core, entity *enta, entity *entb );

// thread entry proceedures in core.cpp
int  coreMainThread( );
int  coreLoaderThread( coreInfo &core );
bool coreBadAlloc( void );
