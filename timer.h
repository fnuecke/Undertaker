/* 
 * Author: fnuecke
 *
 * Created on May 6, 2012, 5:07 AM
 */

#ifndef TIMER_H
#define	TIMER_H

#ifdef	__cplusplus
extern "C" {
#endif

    void T_Init(void);

    // start timer
    void T_Start(void);

    // stop the timer
    void T_Stop(void);

    // get elapsed time in second
    double T_GetElapsedTime(void);

    // get elapsed time in second (same as getElapsedTime)
    double T_GetElapsedTimeInSec(void);

    // get elapsed time in milli-second
    double T_GetElapsedTimeInMilliSec(void);

    // get elapsed time in micro-second
    double T_GetElapsedTimeInMicroSec(void);

#ifdef	__cplusplus
}
#endif

#endif
