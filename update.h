/* 
 * File:   update.h
 * Author: fnuecke
 *
 * Created on April 23, 2012, 2:24 PM
 */

#ifndef UPDATE_H
#define	UPDATE_H

#ifdef	__cplusplus
extern "C" {
#endif

    /**
     * Drives update logic by calling registered callbacks.
     */
    void DK_Update(void);

#ifdef	__cplusplus
}
#endif

#endif	/* UPDATE_H */

