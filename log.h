/* 
 * Author: fnuecke
 *
 * Created on May 9, 2012, 4:13 PM
 */

#ifndef LOG_H
#define	LOG_H

#include <stdio.h>
#include <string.h>

#include "config.h"

#ifdef	__cplusplus
extern "C" {
#endif

    static const char* fileNameFromPath(const char* path) {
        char* lastForwardSeparator = strrchr(path, '/');
        char* lastBackwardSeparator = strrchr(path, '\\');
        char* lastSeparator = lastForwardSeparator > lastBackwardSeparator ? lastForwardSeparator : lastBackwardSeparator;
        if (lastSeparator) {
            return lastSeparator + 1;
        } else {
            return path;
        }
    }
#define __FILENAME__ fileNameFromPath(__FILE__)

#define MP_log(level, ...) \
fprintf(MP_logTarget, "%s:%s:%d: ", level, __FILENAME__, __LINE__); \
fprintf(MP_logTarget, __VA_ARGS__); \
fflush(MP_logTarget)

#define MP_log_info(...) MP_log("INFO", __VA_ARGS__)
#define MP_log_warning(...) MP_log("WARNING", __VA_ARGS__)
#define MP_log_error(...) MP_log("ERROR", __VA_ARGS__)
#define MP_log_fatal(...) MP_log("FATAL", __VA_ARGS__); exit(EXIT_FAILURE)

#ifdef	__cplusplus
}
#endif

#endif
