/* 
 * File:   input.h
 * Author: fnuecke
 *
 * Created on April 16, 2012, 1:25 PM
 */

#ifndef INPUT_H
#define	INPUT_H

#include "callbacks.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /**
     * Handle SDL events.
     */
    void DK_Events(void);

    /**
     * Register a method that should be called when an update is performed.
     * Methods are called in the order in which they are registered.
     * @param callback the method to call.
     */
    void DK_OnUpdate(callback method);

    /**
     * Register a method that should be called when the map size changes.
     * Methods are called in the order in which they are registered.
     * This should be used for disposing old data and register new meta data.
     * @param callback the method to call.
     */
    void DK_OnMapSizeChange(callback method);

    /**
     * Register methods here that need to execute before rendering, but after
     * the view has been set up.
     * @param callback the method to call.
     */
    void DK_OnPreRender(callback method);

    /**
     * Register a method that should be called when an render pass is performed.
     * Methods are called in the order in which they are registered.
     * @param callback the method to call.
     */
    void DK_OnRender(callback method);

    /**
     * Register methods that need to render on top of the finished world render,
     * such as overlays.
     * @param callback the method to call.
     */
    void DK_OnPostRender(callback method);

    /**
     * Register a method to be notified whenever the model matrix changes.
     * @param method the method to call when the model matrix changes.
     */
    void DK_OnModelMatrixChanged(callback method);

#ifdef	__cplusplus
}
#endif

#endif	/* INPUT_H */

