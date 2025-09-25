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

    // member vars
    vertex pos;
    float rx, ry, rz;
    float speed;

    int health;
    DWORD liveTime, currFire;
    enum TypesEnty{None=1, Ship=2, Fire=4, Astro=8, Shild=16};
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



class TimerClass
{
private:
    std::atomic<bool> running{ true };   // Thread live
    std::atomic<bool> active{ false };   // Timer
    std::atomic<bool> timeElapsed{ false };
    std::function<void()> callback{ nullptr };
    std::mutex mtx;
    std::condition_variable cv;

    int intervalSeconds{ 0 };
    int currTime{ 0 };
    bool breakOnElapsed{ true };

    void threadFunc()
    {
        std::unique_lock<std::mutex> lock(mtx);

        while (running)
        {
            // wait of timer new start 
            cv.wait(lock, [this] { return active || !running.load(); });

            if (!running)
                break;

            timeElapsed = false;

            for (currTime = intervalSeconds; currTime > 0; --currTime)
            {
                auto wakeUp = std::chrono::steady_clock::now() + std::chrono::seconds(1);
                cv.wait_until(lock, wakeUp);

                if (!active)
                    break; // Stop/Reset
            }

            if (!active)
                continue;

            // time running out
            timeElapsed = true;
            if (callback)
            {
                lock.unlock();
                callback();
                lock.lock();
                active = false;  // Callback -> Timer stop
            }

            if (breakOnElapsed)
                active = false;  // Timer stop

            cv.wait(lock, [this]() { return !timeElapsed.load() || !active.load() || !running.load(); });
        }
    }

public:
    TimerClass()
    {
        timerThread = std::thread(&TimerClass::threadFunc, this);
    }

    ~TimerClass()
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            running = false;
            cv.notify_all();
        }

        if (timerThread.joinable())
            timerThread.join();
    }

    void Start(int second, bool breakFlag)
    {
        std::lock_guard<std::mutex> lock(mtx);
        currTime = intervalSeconds = second;
        breakOnElapsed = breakFlag;
        callback = nullptr;
        timeElapsed = false;
        active = true;
        cv.notify_one();
    }

    void Start(std::function<void()> func, int second)
    {
        std::lock_guard<std::mutex> lock(mtx);
        currTime = intervalSeconds = second;
        callback = func;
        breakOnElapsed = true;
        timeElapsed = false;
        active = true;
        cv.notify_one();
    }

    bool IsTime()
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (timeElapsed)
        {
            timeElapsed = false;
            cv.notify_one(); // Timer wakeup
            return true;
        }
        return !active.load();
    }

    int GetCurrTime()
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (active)
            return currTime;
        return 0;
    }


    void Stop()
    {
        std::lock_guard<std::mutex> lock(mtx);
        active = false;
        timeElapsed = false;
        currTime = 0;
        cv.notify_one();
    }

private:
    std::thread timerThread;
};

