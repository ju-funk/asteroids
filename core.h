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
        model ItemFire, ItemShild, ItemShip, ItemFireGun, ItemShipStop;
    } models;

    // other game specific vars
    int iGameLevel;
    DWORD Score, Fires; 
    const DWORD cShlTime = 3, cShlTiDel = 10;
    TimerClass::tTimerVar  ShlTime, ShlTiDel, ItemTime;
    int Ships;
    bool FireGun, ShipStop;

    struct HiScoreEntry
    {
        const static int MaxName = 30;
        const static int MaxList = 10;

        TCHAR str[MaxName] = {};
        std::chrono::system_clock::time_point tipo;
        DWORD Score;

        DWORD SetName(const TCHAR* Na)
        {
            _tcscpy_s(str, Na);
            return (static_cast<DWORD>(_tcslen(str)) + 1) * sizeof(TCHAR);
        }

        DWORD SetDat(BYTE* dat)
        {
            DWORD len = sizeof(tipo) + sizeof(Score);
            memcpy(&tipo, dat, len);

            return len;
        }

        void cpyDat(BYTE** dat)
        {
            size_t len = _tcslen(str) + 1;
            _tcscpy_s(reinterpret_cast<TCHAR*>(*dat), len, str);
            *dat += len * sizeof(TCHAR);

            len = sizeof(tipo) + sizeof(Score);
            memcpy(*dat, &tipo, len);
            *dat += len;
        }

        HiScoreEntry(DWORD score, const TCHAR* Na = _T(""), std::chrono::system_clock::time_point ti = std::chrono::system_clock::now())
        {
            SetName(Na);
            tipo = ti;
            Score = score;
        }

        bool IsNameNotSet() const {return str[0] == 0;}
    };

    using vtHiSc = std::vector<HiScoreEntry>;
    using vtHiScIt = vtHiSc::iterator;

    vtHiSc vHiScore;

    void LoadHiScore();
    void SaveHiScore();

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

void InitVkMap();
void DelVkMap();

