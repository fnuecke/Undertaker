/* 
 * File:   log.h
 * Author: fnuecke
 *
 * Created on May 9, 2012, 4:13 PM
 */

#ifndef LOG_H
#define	LOG_H

#include <stdio.h>

#include "config.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define MP_log(level, ...) \
fprintf(MP_log_target, "%s: ", level); \
fprintf(MP_log_target, __VA_ARGS__); \
fflush(MP_log_target)

#define MP_log_info(...) MP_log("INFO", __VA_ARGS__)
#define MP_log_warning(...) MP_log("WARNING", __VA_ARGS__)
#define MP_log_error(...) MP_log("ERROR", __VA_ARGS__)
#define MP_log_fatal(...) MP_log("FATAL", __VA_ARGS__); exit(EXIT_FAILURE)

#ifdef	__cplusplus
}
#endif

#endif	/* LOG_H */

