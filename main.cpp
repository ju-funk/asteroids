#include "main.h"
#include "core.h"
#include <thread>

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
    // create video buffer in a desktop window
    if(!output.Create(1280, 720, _T("dila & ju-funk / 2025")))
        return 0;

    // query user about screen mode
    bool bFullScreen = sys::userQuery( _T("Would you like to play in fullscreen mode?") );
    output.FullScreen(bFullScreen);

    // spawn the render thread. see core.cpp
    std::thread blitter( coreMainThread);
    if ( !blitter.joinable() )
    {
        sys::userNotice( _T("Failed to spawn game thread."), true );
        return 0;
    }

    // process window messages until quit
    while(output.doEvents());

    // wait for render thread to exit
    blitter.join();

    // return wParam, as msdn describes
    return output.getExitCode();
}
