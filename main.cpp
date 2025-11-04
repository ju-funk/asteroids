#include "main.h"
#include "core.h"

// global running status flags
bool bHasTermSignal = false;
KeyMan keys;
sys::screen output;
TimerClass secTimer;


#ifdef  UNICODE
#define tWinMain wWinMain
#else
#define tWinMain WinMain
#endif

int WINAPI tWinMain( HINSTANCE, HINSTANCE, LPTSTR, int )
{
    // create video buffer in a desktop window
    if(!output.Create(1280, 720, _T("Asteroids v1.0"), _T("dila (02/06) & funk (10/25)")))
        return 0;

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
