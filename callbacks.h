/* 
 * File:   callbacks.h
 * Author: fnuecke
 *
 * Created on April 25, 2012, 5:20 PM
 */

#ifndef CALLBACKS_H
#define	CALLBACKS_H

#ifdef	__cplusplus
extern "C" {
#endif

    /** Signature of supported methods */
    typedef void(*callback)(void);

    /** The data structure used to track the callback list */
    typedef struct Callbacks Callbacks;

    /**
     * Create a new callback list.
     * @return the allocated callback list.
     */
    Callbacks* CB_New(void);

    /**
     * Free the memory occupied by a callback list.
     * @param list the list to free.
     */
    void CB_Delete(Callbacks* list);

    /**
     * Add a callback to a callback list. Methods are called in the order in
     * which they are registered.
     * @param list the list to add the callback to.
     * @param callback the callback to add.
     */
    void CB_Add(Callbacks* list, callback method);

    /**
     * Call all entries in a callback list.
     * @param list the callback list to process.
     */
    void CB_Call(Callbacks* list);

#ifdef	__cplusplus
}
#endif

#endif	/* CALLBACKS_H */
