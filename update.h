/* 
 * File:   update.h
 * Author: fnuecke
 *
 * Created on April 23, 2012, 2:24 PM
 */

#ifndef UPDATE_H
#define	UPDATE_H

#include "callbacks.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /**
     * Drives update logic by calling registered callbacks.
     */
    void DK_Update(void);

    /**
     * Register a method that should be called when an update is performed.
     * Methods are called in the order in which they are registered.
     * @param callback the method to call.
     */
    void DK_OnUpdate(callback method);

#ifdef	__cplusplus
}
#endif

#endif	/* UPDATE_H */

