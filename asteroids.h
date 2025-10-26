// ------------------------------------------------------------------
// entity object. stores sprite relevant variables
// ------------------------------------------------------------------


class entity
{
public:
    // direction control
    void updatePos( void );
    void addPos( void );
    void setDir( float angle );
    void addDir( float angle );
    void swapAstDir( entity *with, float speed );
    void swapSldDir( entity *with, float speed );
    void Spin();
    bool checkShip();
    void TestLive();
    void swapSpeed( entity *with );
    bool setExplore(bool destry=false);

    // member vars
    vertex pos;
    float rx, ry, rz;
    float speed;

    int health;
    DWORD liveTime, currFire;
    enum TypesEnty : DWORD {Zero=0, None=1, Ship=2, Fire=4, Astro=8, Shild=16, ItFire=32, ItShild=64, ItShip = 128, ItFireGun = 256, ItShipStop = 512};
    static const int MaxItems = 29;
    static const TypesEnty Items[MaxItems+1];

    TypesEnty TypeEnty;

    TypesEnty operator |(entity &a) {return static_cast<TypesEnty>(static_cast<DWORD>(TypeEnty) | static_cast<DWORD>(a.TypeEnty));}
    void operator |=(TypesEnty a) { TypeEnty = static_cast<TypesEnty>(static_cast<DWORD>(TypeEnty) | static_cast<DWORD>(a));}

    // model pointer and scale
    float scale;
    model points;

    // constructor
    entity( model &source, float xpos, float ypos, TypesEnty typeEnty);
};


void astFireBullet(coreInfo& core);
void astShipShild(coreInfo& core, bool shild);
int getShildInf(coreInfo& core);
void astNewGame(coreInfo& core, bool newgame);
void astUpdateState(coreInfo& core);
void astCheckCollision(coreInfo& core, entity* enta, entity* entb);
void astGenItems(coreInfo& core, entity::TypesEnty ty, vertex& where);



