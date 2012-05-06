#include "timer.h"

#include <stdlib.h>
#ifdef WIN32   // Windows system specific
#include <windows.h>
#else          // Unix based system specific
#include <sys/time.h>
#endif

double gStartTimeInMicroSec; // starting time in micro-second
double gEndTimeInMicroSec; // ending time in micro-second
int gStopped; // stop flag 
#ifdef WIN32
LARGE_INTEGER gFrequency; // ticks per second
LARGE_INTEGER gStartCount;
LARGE_INTEGER gEndCount;
#else
timeval gStartCount;
timeval gEndCount;
#endif

void T_Init(void) {
#ifdef WIN32
    QueryPerformanceFrequency(&gFrequency);
    gStartCount.QuadPart = 0;
    gEndCount.QuadPart = 0;
#else
    gSartCount.tv_sec = gStartCount.tv_usec = 0;
    gEndCount.tv_sec = gEndCount.tv_usec = 0;
#endif

    gStopped = 0;
    gStartTimeInMicroSec = 0;
    gEndTimeInMicroSec = 0;
}

void T_Start(void) {
    gStopped = 0; // reset stop flag
#ifdef WIN32
    QueryPerformanceCounter(&gStartCount);
#else
    gettimeofday(&gStartCount, NULL);
#endif
}

void T_Stop(void) {
    gStopped = 1; // set timer stopped flag

#ifdef WIN32
    QueryPerformanceCounter(&gEndCount);
#else
    gettimeofday(&gEndCount, NULL);
#endif
}

double T_GetElapsedTimeInMicroSec(void) {
#ifdef WIN32
    if (!gStopped) {
        QueryPerformanceCounter(&gEndCount);
    }

    gStartTimeInMicroSec = gStartCount.QuadPart * (1000000.0 / gFrequency.QuadPart);
    gEndTimeInMicroSec = gEndCount.QuadPart * (1000000.0 / gFrequency.QuadPart);
#else
    if (!gStopped) {
        gettimeofday(&gEndCount, NULL);
    }

    gStartTimeInMicroSec = (gStartCount.tv_sec * 1000000.0) + gStartCount.tv_usec;
    gEndTimeInMicroSec = (gEndCount.tv_sec * 1000000.0) + gEndCount.tv_usec;
#endif

    return gEndTimeInMicroSec - gStartTimeInMicroSec;
}

double T_GetElapsedTimeInMilliSec(void) {
    return T_GetElapsedTimeInMicroSec() * 0.001;
}

double T_GetElapsedTimeInSec(void) {
    return T_GetElapsedTimeInMicroSec() * 0.000001;
}

double T_GetElapsedTime(void) {
    return T_GetElapsedTimeInSec();
}
