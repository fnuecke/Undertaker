#include "timer.h"

#include <stdlib.h>
#ifdef WIN32   // Windows system specific
#include <windows.h>
#else          // Unix based system specific
#include <sys/time.h>
#endif

double gStartTimeInMicroSec; // starting time in micro-second
double gEndTimeInMicroSec; // ending time in micro-second
int gRunning; // timer currently running?
#ifdef WIN32
static LARGE_INTEGER gFrequency; // ticks per second
static LARGE_INTEGER gStartCount;
static LARGE_INTEGER gEndCount;
#else
static struct timeval gStartCount;
static struct timeval gEndCount;
#endif

void T_Init(void) {
#ifdef WIN32
    QueryPerformanceFrequency(&gFrequency);
    gStartCount.QuadPart = 0;
    gEndCount.QuadPart = 0;
#else
    gStartCount.tv_sec = gStartCount.tv_usec = 0;
    gEndCount.tv_sec = gEndCount.tv_usec = 0;
#endif

    gRunning = 0;
    gStartTimeInMicroSec = 0;
    gEndTimeInMicroSec = 0;
}

void T_Start(void) {
    if (!gRunning) {
        gRunning = 1;
#ifdef WIN32
        QueryPerformanceCounter(&gStartCount);
        gStartTimeInMicroSec = gStartCount.QuadPart * 1000000.0 / gFrequency.QuadPart;
#else
        gettimeofday(&gStartCount, NULL);
        gStartTimeInMicroSec = (gStartCount.tv_sec * 1000000.0) + gStartCount.tv_usec;
#endif
    }
}

void T_Stop(void) {
    if (gRunning) {
        gRunning = 0;
#ifdef WIN32
        QueryPerformanceCounter(&gEndCount);
#else
        gettimeofday(&gEndCount, NULL);
#endif
    }
}

double T_GetElapsedTimeInMicroSec(void) {
#ifdef WIN32
    if (gRunning) {
        QueryPerformanceCounter(&gEndCount);
    }
    gEndTimeInMicroSec = gEndCount.QuadPart * 1000000.0 / gFrequency.QuadPart;
#else
    if (gRunning) {
        gettimeofday(&gEndCount, NULL);
    }
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
