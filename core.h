// CORE THREAD PROCEEDURES AND PARAMETER STRUCTURES
// ------------------------------------------------------------------
#include "gfx.h"
#include "asteroids.h"

struct coreInfo
{
    // device settings
    unsigned long int *pBuffer;
    float *pFrame, *pFrameEnd;
    float fScaleX, fScaleY;
    float fSWidth, fSHeight;
    float fZOrigin;
    int iWidth, iHeight;
    int iCWidth, iCHeight;
    int iSize;

    // vertex array
    array::block<vertex> *points;

    // list of active entities
    // the players sprite is always
    // first in the list
    array::list<entity*> sprites;

    // group the models in a struct
    struct modPtrs
    {
        model ship, misile, stars;
        model stroidTiny, stroidMed, stroidBig;
    } models;

    // other game specific vars
    int iGameLevel;
};

// prototypes called by core.cpp
void gfxDrawLoader( coreInfo &info, float &ticker );
void gfxBlinkStars( coreInfo &core );
void astWrapSprite( coreInfo &core, entity &sprite );
bool astFireBullet( coreInfo &core );
bool astNewGame( coreInfo *core );
bool astUpdateState( coreInfo &core );
void astDeallocSprites( coreInfo &core );
void astCheckCollision( coreInfo &core, entity *enta, entity *entb );

// thread entry proceedures in core.cpp
int coreMainThread( sys::screen *output );
int coreLoaderThread( coreInfo *core );
bool coreBadAlloc( void );
