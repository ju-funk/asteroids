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
    void Init( const TCHAR *szCaption, int width, int height, bool fullScreen, bool mouse );
    ~screen( void ) { cleanup(); }
    void cleanup( void );

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

    bool GetMousePos(POINT &mousePos, int *&wheel);

private:
    // main init function
    bool create( bool topMost, bool hasCaption, bool scrCenter );

    // fullscreen helper
    bool toggleFullScreen( void );

    // message handling proceedure
    static LRESULT CALLBACK winDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static int StWheel;
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
    unsigned long *pBitmap;
    unsigned long *pBmpEnd;
    int iBmpSize;
    int iWidth;
    int iHeight;

    RECT rSize;
    TCHAR *szClass;
    bool bVisibleState;
    int iExitCode;

    // full screen memebrs
    bool hasFullScreen;
    DEVMODE dmOriginalConfig;

    // initialized success flag
    bool wasInitialized;
};
