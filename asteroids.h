// ------------------------------------------------------------------
// entity object. stores sprite relevant variables
// ------------------------------------------------------------------
#include <vector>
#include <chrono>

class entity
{
public:
    // constructor
    entity( model &source, float xpos, float ypos );

    // direction control
    void updatePos( void );
    void addPos( void );
    void setDir( float angle );
    void addDir( float angle );
    void swapDir( entity *with );
    void Spin();
    void swapSpeed( entity *with );

    // member vars
    vertex pos;
    float rx, ry, rz;
    float speed;

    int health;
    DWORD liveTime;
    enum TypesEnty{None=1, Ship=2, Fire=4, Astro=8};
    DWORD TypeEnty;

    // model pointer and scale
    float scale;
    model points;
};

class KeyMan
{
public:
    enum { IsDown = 0, MustToggle = -1, eKeyNone = 0, eKeyShift = 1, eKeyCtrl = 2, eKeyAlt = 4 }; // Todo > 0 --> timeout
    bool GetKeyState(int Key, int Todo, int extkey = eKeyNone);

private:
    struct kdat
    {
        int Key, extKeys;
        bool isPress;
        std::chrono::steady_clock::time_point last;

        kdat(int key, int ekey)
        {
            Key = key;
            extKeys = ekey;
            isPress = false;
            last = std::chrono::high_resolution_clock::now();
        }
    };

    std::vector<kdat> vkDat;

    kdat& GetKDat(int Key, int extkey);
};


