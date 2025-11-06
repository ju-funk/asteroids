#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include <stdlib.h>
#include "array.h"
#include "system.h"

// global running state flags
extern bool bHasTermSignal;
extern sys::screen output;
extern KeyMan keys;
extern TimerClass secTimer;


