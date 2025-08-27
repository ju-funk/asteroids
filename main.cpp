#include "main.h"
#include "core.h"

// global running status flags
bool bHasTermSignal = false;
sys::screen output;


#ifdef  UNICODE
#define tWinMain wWinMain
#else
#define tWinMain WinMain
#endif

int WINAPI tWinMain( HINSTANCE, HINSTANCE, LPTSTR, int )
{
    // query user about screen mode
    bool bFullScreen = sys::userQuery( _T("Would you like to play in fullscreen mode?") );
    bool bmouse = true;
        
    if(bFullScreen)
        bmouse = sys::userQuery( _T("Would you like to play with mouse?") );

    // create video buffer in a desktop window
    output.Init( _T("dila/2006"), 1280, 720, bFullScreen, bmouse);
    if ( !output )
        return 0;

    // spawn the render thread. see core.cpp
    sys::thread blitter( coreMainThread, false, &output );
    if ( !blitter )
    {
        sys::userNotice( _T("Failed to spawn game thread."), true );
        return 0;
    }

    // process window messages until quit
    while ( output.doEvents() );

    // wait for render thread to exit
    blitter.waitForSignal();

    // return wParam, as msdn describes
    return output.getExitCode();
}
