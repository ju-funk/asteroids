// ------------------------------------------------------------------
// entity object. stores sprite relevant variables
// ------------------------------------------------------------------
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>


#define  TRACE0(sz)     //output.DebugOut(sz)
#define  TRACE1(sz, p1) //output.DebugOut(sz, p1)


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
    bool setExplore();

    // member vars
    vertex pos;
    float rx, ry, rz;
    float speed;

    int health;
    DWORD liveTime, currFire;
    enum TypesEnty : DWORD {Zero=0, None=1, Ship=2, Fire=4, Astro=8, Shild=16, ItFire=32, ItShild=64, ItShip = 128, ItFireGun = 256};
    static const int MaxItems = 22;
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



class TimerClass
{
public:
    struct tTimerVar
    {
        int currTime = 0;
        int orgTime  = 0;

        void operator =(int val) {orgTime = val;}
        int GetTime() const { return currTime; }
    };

private:
    std::atomic<bool> running{ true };   // Thread live
    std::atomic<bool> active{ false };   // Timer active
    std::mutex mtx_loop, mtx;
    std::condition_variable cv;
    std::thread timerThread;


    struct tTimeData
    {
        std::function<void()> callback;
        tTimerVar *currTi;
        bool breakFlag;

        tTimeData(tTimerVar *curr, bool breakf = true, std::function<void()> call = nullptr) :
            currTi{curr}, breakFlag{breakf}, callback{call} {}
    };

    using vtTiDat  = std::vector<tTimeData>;
    using itTiDat = vtTiDat::iterator;
    vtTiDat vTiDat;

    void DecVec()
    {
        std::unique_lock<std::mutex> lock(mtx);

        std::for_each(vTiDat.begin(), vTiDat.end(), [&lock](tTimeData& dat) {
            --dat.currTi->currTime;

            if (dat.currTi->currTime == 0)
            {
                if (dat.callback != nullptr)
                {
                    lock.unlock();
                    dat.callback();
                    lock.lock();
                }

                if (!dat.breakFlag)
                    dat.currTi->currTime = dat.currTi->orgTime;
            }
        });

        itTiDat it = std::remove_if(vTiDat.begin(), vTiDat.end(), [](const tTimeData& dat) {return dat.currTi->currTime == 0; });
        vTiDat.erase(it, vTiDat.end());
        active = vTiDat.size() > 0;
    }


    void threadFunc()
    {
        std::unique_lock<std::mutex> lock(mtx_loop);

        while (running)
        {
            cv.wait(lock, [this] { return active.load() || !running; });

            if(!running)
                break;

            while (active)
            {
                auto wakeUp = std::chrono::steady_clock::now() + std::chrono::seconds(1);
                cv.wait_until(lock, wakeUp, [this] { return !running; });
 
                if (running)
                    DecVec();
                else
                    active = false;
            }
        }
    }


    bool HandleAccess(tTimeData *td, bool end = false)
    {
        std::lock_guard<std::mutex> lock(mtx);

        bool ret = true;

        if (td == nullptr)
        {
            if(!end)
                std::for_each(vTiDat.begin(), vTiDat.end(), [&lock](tTimeData& dat) { dat.currTi->currTime = 0; });
            vTiDat.clear();
            running = !end;
            active = false;
            cv.notify_one();
        }
        else
        {
            itTiDat it = std::find_if(vTiDat.begin(), vTiDat.end(), [td](const tTimeData& dat) {return dat.currTi == td->currTi; });
            
            if (td->currTi->orgTime == 0)
            {
                if(it != vTiDat.end())
                    vTiDat.erase(it);
                else
                    ret = false;
            }
            else
            {
                // not insert the same var twice
                if (it == vTiDat.end())
                {
                    td->currTi->currTime = td->currTi->orgTime;

                    vTiDat.push_back(*td);
                    if (!active)
                    {
                        active = true;
                        cv.notify_one();
                    }
                }
                else
                    ret = false;
            }
        }

        return ret;
    }

public:
    TimerClass()
    {
        timerThread = std::thread(&TimerClass::threadFunc, this);
    }

    ~TimerClass()
    {
        EndTimer();

        if (timerThread.joinable())
            timerThread.join();
    }

    bool NewTimer(tTimerVar &second, bool breakFlag)
    {
        tTimeData td(&second, breakFlag);
        return HandleAccess(&td);
    }

    bool NewTimer(tTimerVar &second, std::function<void()> func, bool breakFlag = true)
    {
        tTimeData td(&second, breakFlag, func);
        return HandleAccess(&td);
    }

    bool StopTimer(tTimerVar &second)
    {
        second = 0;
        tTimeData td(&second);
        return HandleAccess(&td);
    }

    void StopTimer()
    {
        HandleAccess(nullptr);
    }

    void EndTimer()
    {
        HandleAccess(nullptr, true);
    }
};

