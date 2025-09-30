#include "resource.h"
#include <mmsystem.h>
#include <algorithm>
#include <map>


// SYSTEM OBJECT DECLERATIONS
// ------------------------------------------------------------------
namespace sys
{
    class screen;

    // native api wrappers
    void userNotice( const TCHAR *szMessage, bool isFatal );
    bool userQuery( const TCHAR *szMessage );
    unsigned long getSeed( void );
}

// ------------------------------------------------------------------
// screen object
// ------------------------------------------------------------------
class sys::screen
{
public:
    // member operators
    bool operator !( void ) { return !wasInitialized; }

    // constructor, cleanup proceedure
    screen(const TCHAR* szCaption) {pcTitle = szCaption;}
    void Create(int width, int height, bool fullScreen, bool mouse);
    ~screen(void) {cleanup();}
    void cleanup(void);

    // display setup functions
    void setCaption( const TCHAR *szCaption );
    void setVisible( bool state );
    void flipBuffers( void );
    void clearBuffer( void );

    // event handling function
    bool doEvents( void );
    void signalQuit( void ) { PostMessage( hWnd, WM_CLOSE, (WPARAM)this, 0 ); }

    // private accessors
    bool isVisible( void ) { return bVisibleState; }
    int getExitCode( void ) { return iExitCode; }
    unsigned long *getScreenBuffer( void ){ return pBitmap; }
    int getWidth( void ) { return iWidth; }
    int getHeight( void ) { return iHeight; }

    HDC Get_DC() { return hVideoDC; }
    const TCHAR *GetTitle() {return pcTitle;}

    bool GetInputState(POINT& mousePos, int& wheel);
    void SetInputState(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void Sound(WORD id);


#ifdef _DEBUG
    static void DebugOut(const TCHAR* pszFmt, ...);
#else
    #define DebugOut
#endif // DEBUG


private:
    // main init function
    bool create( bool topMost, bool hasCaption, bool scrCenter );
    bool LoadWaves();
    void CleanWave();

    // fullscreen helper
    bool toggleFullScreen(void);

    // message handling proceedure
    static LRESULT CALLBACK winDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    int StWheel;
    bool EnaMouse;

    // window members
    HINSTANCE hInstance;
    HWND hWnd;
    HDC hDC;
    MSG wMsg;

    // gdi members
    HDC hVideoDC;
    HBITMAP hBitmap;
    HGDIOBJ hOldObject;
    HGDIOBJ hOldFontObj;
    unsigned long *pBitmap;
    unsigned long *pBmpEnd;
    int iBmpSize;
    int iWidth;
    int iHeight;

    RECT rSize;
    TCHAR* szClass;
    const TCHAR* pcTitle;
    bool bVisibleState;
    int iExitCode;

    // full screen memebrs
    bool hasFullScreen;
    DEVMODE dmOriginalConfig;

    // initialized success flag
    bool wasInitialized;


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
        mtSnd vSound;
    };

    static void CALLBACK WaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    mpbtSnd OpenWave(stSound *Snd);
    void CloseWave(vtSound& vsnd);


    stSound pWaves[IDW_ENDWAV - IDW_OFFSET];
};
