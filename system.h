#include "resource.h"
#include <mmsystem.h>
#include <algorithm>
#include <map>
#include <tchar.h>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>


// SYSTEM OBJECT DECLERATIONS
// ------------------------------------------------------------------
namespace sys
{
    class screen;

    // native api wrappers
    void userNotice( const TCHAR *szMessage, bool isFatal );
    unsigned long getSeed( void );

#ifdef _DEBUG
    void DebugOut(const TCHAR* pszFmt, ...);
#else
#define DebugOut
#endif // DEBUG
}

#define  TRACE0(sz)     sys::DebugOut(sz)
#define  TRACE1(sz, p1) sys::DebugOut(sz, p1)


// ------------------------------------------------------------------
// screen object
// ------------------------------------------------------------------
class sys::screen
{
public:
    screen() {};
    bool Create(int width, int height, const TCHAR* szCaption);

    ~screen(void) { cleanup(); }
    void cleanup(void);

    // display setup functions
    void flipBuffers(void);
    void clearBuffer(void);

    // event handling function
    bool doEvents(void);
    void signalQuit(void) { PostMessage(hWnd, WM_CLOSE, (WPARAM)this, 0); }

    // private accessors
    int getExitCode(void) { return iExitCode; }
    unsigned long* getScreenBuffer(void) { return pBitmap; }
    int getWidth(void) { return iWidth; }
    int getHeight(void) { return iHeight; }

    inline HDC Get_DC() { return hVideoDC; }
    inline const TCHAR* GetTitle() { return pcTitle; }
    inline HWND GetWnd() {return hWnd;}
    inline bool GetFullScr() {return hasFullScreen;}
    bool GetTogMouse(bool ena = false)
    {
        if(ena)
            EnaMouse = !EnaMouse;
        return EnaMouse;
    }

    bool GetInputState(POINT& mousePos, int& wheel);
    void SetInputState(UINT uMsg, WPARAM wParam, LPARAM lParam);
    inline bool IsActiv() { return hWnd == GetForegroundWindow(); }


    void Sound(WORD id);


    void SetNewFont(const TCHAR* Fn, int Size = 24, int Ta = TA_CENTER, int Bm = TRANSPARENT, int cF = 0, COLORREF ClTx = RGB(255, 255, 0), COLORREF ClBa = RGB(0, 0, 0));
    void StrOutText(const TCHAR* Str, int x, int y) const {
        TextOut(hVideoDC, x, y, Str, static_cast<int>(_tcslen(Str)));
    }

    SIZE GetTextSize(const TCHAR* text);




private:
    // main init function
    bool create();
    bool LoadWaves();
    void CleanWave();
    void setVisible(bool state);

    // fullscreen helper
    bool toggleFullScreen(void);
    inline bool IsSetFull() const { return msgLoop; }
    inline void SetFull(bool val) {msgLoop = val;}

    // message handling proceedure
    static LRESULT CALLBACK winDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    int StWheel = 0;
    bool EnaMouse = true;
    bool msgLoop = false;

    // window members
    HINSTANCE hInstance = nullptr;
    HWND hWnd = nullptr;
    HDC hDC = nullptr;
    MSG wMsg = {};

    // gdi members
    HDC hVideoDC = nullptr;
    HBITMAP hBitmap = nullptr;
    HGDIOBJ hOldObject = nullptr;
    HGDIOBJ hOldFontObj = nullptr;
    unsigned long *pBitmap = nullptr;
    int iBmpSize = 0;
    int iWidth = 0;
    int iHeight = 0;

    RECT rSize = {};
    TCHAR* szClass = nullptr;
    const TCHAR* pcTitle = nullptr;
    bool bVisibleState = false;
    int iExitCode = 0;

    // full screen memebrs
    bool hasFullScreen = false;
    DEVMODE dmOriginalConfig = {};
    LONG_PTR windowStyle = 0;

    // waves  stuff
    const WORD StartHeader = 20;
    const WORD WaveStartData = StartHeader + 24;
   
    struct vtSound
    {
        enum tprep : int {Del=0, Use=1, FreeInUse=2, Mask=3};
        tprep prepared = Use;        // bit 0 = 0 can erase,  = 1 sound usable
                                     // bit 1 = 0 sound free, = 1 sound in use
        void operator|=(tprep a) {prepared = static_cast<tprep>(static_cast<int>(prepared) |  static_cast<int>(a));}
        void operator&=(tprep a) {prepared = static_cast<tprep>(static_cast<int>(prepared) & ~static_cast<int>(a));}

        HWAVEOUT hWaveOut = nullptr;
        WAVEHDR waveHdr = {};
        size_t  Idx;
        vtSound(size_t idx) : Idx{idx} {}
    };

    using mtSnd   = std::map<size_t, vtSound>;
    using mptSnd  = std::pair<const size_t, vtSound>;
    using mtSndIt = mtSnd::iterator;
    using mpbtSnd = std::pair<mtSndIt, bool>;

    struct stSound
    {
        BYTE *buffer = nullptr;
        DWORD size = 0;
        WAVEFORMATEX wfx = {};

        size_t key = 0;
        size_t max_key = 0;
        mtSnd vSound;
    };

    static void CALLBACK WaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    mpbtSnd OpenWave(stSound *Snd);
    void CloseWave(vtSound& vsnd);

    stSound pWaves[IDW_ENDWAV - IDW_OFFSET];
};


/// /////////////////////////////////////////////////////////
/// helper classes for Key/Mouse-handling & TimerClass 1 sec

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
        int orgTime = 0;

        void operator =(int val) { orgTime = val; }
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
        tTimerVar* currTi;
        bool breakFlag;

        tTimeData(tTimerVar* curr, bool breakf = true, std::function<void()> call = nullptr) :
            currTi{ curr }, breakFlag{ breakf }, callback{ call } {
        }
    };

    using vtTiDat = std::vector<tTimeData>;
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

            if (!running)
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


    bool HandleAccess(tTimeData* td, bool end = false)
    {
        std::lock_guard<std::mutex> lock(mtx);

        bool ret = true;

        if (td == nullptr)
        {
            if (!end)
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
                if (it != vTiDat.end())
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

    bool NewTimer(tTimerVar& second, bool breakFlag)
    {
        tTimeData td(&second, breakFlag);
        return HandleAccess(&td);
    }

    bool NewTimer(tTimerVar& second, std::function<void()> func, bool breakFlag = true)
    {
        tTimeData td(&second, breakFlag, func);
        return HandleAccess(&td);
    }

    bool StopTimer(tTimerVar& second)
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

