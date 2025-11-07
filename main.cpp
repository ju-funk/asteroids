#include "main.h"
#include "core.h"

// global running status flags
bool bHasTermSignal = false;

int APIENTRY WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
{
    // query user about screen mode
    bool bFullScreen = system::userQuery( "Would you like to play in fullscreen mode?" );

    // create video buffer in a desktop window
    system::screen output( "dila/2006", 640, 480, bFullScreen );
    if ( !output ) return 0;

    // spawn the render thread. see core.cpp
    system::thread blitter( coreMainThread, false, &output );
    if ( !blitter )
    {
        system::userNotice( "Failed to spawn game thread.", true );
        return 0;
    }

    // process window messages until quit
    while ( output.doEvents() );

    // wait for render thread to exit
    blitter.waitForSignal();

    // return wParam, as msdn describes
    return output.getExitCode();
}
