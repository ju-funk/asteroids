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
    // constructor
    entity( model &source, float xpos, float ypos );

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
    enum TypesEnty : DWORD {None=1, Ship=2, Fire=4, Astro=8, Shild=16};
    TypesEnty TypeEnty;

    TypesEnty operator |(entity &a) {return static_cast<TypesEnty>(static_cast<DWORD>(TypeEnty) | static_cast<DWORD>(a.TypeEnty));}
    void operator |=(TypesEnty a) { TypeEnty = static_cast<TypesEnty>(static_cast<DWORD>(TypeEnty) | static_cast<DWORD>(a));}

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



class TimerClass
{
public:
    struct tTimerVar
    {
        int Idx = 0;
        int currTime = 0;
        int orgTime  = 0;

        void operator =(int val) {orgTime = val;}
        int GetTime() { return currTime; }
    };

private:
    std::atomic<bool> running{ true };   // Thread live
    enum tAccState {none, loopstart, inloop, change};
    std::atomic<tAccState> chgvec{ none };   // signal for change vector
    std::mutex mtx;
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
    int  Index = 0;

    void threadFunc()
    {
        std::unique_lock<std::mutex> lock(mtx);

        while (running)
        {
            // wait of timer new start
            TRACE0(_T("threadFunc : cv.wait running\n"));
            cv.wait(lock, [this] { return chgvec == loopstart || !running.load(); });
            TRACE0(_T("threadFunc : cv.wait after\n"));

            if (!running)
                break;
            
            TRACE0(_T("threadFunc : set inloop\n"));

            chgvec = inloop;
            cv.notify_one();

            TRACE0(_T("threadFunc : go in vector loop\n"));
            while (!vTiDat.empty() && chgvec == inloop)
            {
                auto wakeUp = std::chrono::steady_clock::now() + std::chrono::seconds(1);
                cv.wait_until(lock, wakeUp, [this] { return chgvec != inloop || !running; });
                TRACE1(_T("threadFunc : out wait_until te = %i\n"), (int)chgvec);

                if(chgvec != inloop || vTiDat.empty())
                    break;
                
                TRACE0(_T("threadFunc : timer stuff\n"));

                std::for_each(vTiDat.begin(), vTiDat.end(), [](tTimeData& dat) {
                    --dat.currTi->currTime;

                    if (dat.currTi->currTime == 0)
                    {
                        if (dat.callback != nullptr)
                            dat.callback();

                        if (!dat.breakFlag)
                            dat.currTi->currTime = dat.currTi->orgTime;
                    }
                });

                if (chgvec != inloop)
                    break;

                itTiDat it = std::remove_if(vTiDat.begin(), vTiDat.end(), [](const tTimeData& dat) {return dat.currTi->currTime == 0; });
                vTiDat.erase(it, vTiDat.end());
            }

            if (chgvec == change)
            {
                TRACE0(_T("threadFunc : change send noty\n"));
                chgvec = none;
                cv.notify_one();
                TRACE0(_T("threadFunc : change was send noty\n"));
            }
            else
                chgvec = vTiDat.empty() ? none : inloop;
        }
    }


    bool HandleAccess(tTimeData *td, bool end = false)
    {
        std::unique_lock<std::mutex> lock(mtx);
        bool ret = true;

        if (chgvec == inloop)
        {
            TRACE0(_T("HandleAccess : is inloop, set change\n"));
            chgvec = change;
            cv.notify_one();
            cv.wait(lock, [this] { return chgvec == none; });
            TRACE0(_T("HandleAccess : is inloop, get none\n"));
        }

        if (td == nullptr)
        {
            if(!end)
                std::for_each(vTiDat.begin(), vTiDat.end(), [&lock](tTimeData& dat) { dat.currTi->currTime = 0; });
            vTiDat.clear();
            running = !end;
            cv.notify_one();
        }
        else
        {
            if (td->currTi->orgTime == 0)
            {
                itTiDat it = std::find_if(vTiDat.begin(), vTiDat.end(), [td](const tTimeData& dat) {return dat.currTi->Idx == td->currTi->Idx; });
             
                if(it != vTiDat.end())
                    vTiDat.erase(it);
                else
                    ret = false;
            }
            else
            {
                itTiDat it = std::find_if(vTiDat.begin(), vTiDat.end(), [td](const tTimeData& dat) {return dat.currTi == td->currTi; });
                // not insert the same var twice
                if(it == vTiDat.end())
                    vTiDat.push_back(*td);
                else
                    ret = false;
            }

            //output.DebugOut(_T("HandleAccess : set loopstart\n"));
            chgvec = loopstart;
            cv.notify_one();
            //output.DebugOut(_T("HandleAccess : set loopstart send noty\n"));
            cv.wait(lock, [this] { return chgvec == inloop; });
            //output.DebugOut(_T("HandleAccess : get inloop\n"));
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
        second.currTime = second.orgTime;
        second.Idx      = ++Index;
        tTimeData td(&second, breakFlag);
        return HandleAccess(&td);
    }

    bool NewTimer(tTimerVar &second, std::function<void()> func, bool breakFlag = true)
    {
        second.currTime = second.orgTime;
        second.Idx      = ++Index;
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

