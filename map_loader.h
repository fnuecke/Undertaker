/* 
 * Author: fnuecke
 *
 * Created on May 5, 2012, 5:53 PM
 */

#ifndef MAP_LOADER_H
#define	MAP_LOADER_H

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Save / Load
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Load the map with the specified name.
     */
    void MP_LoadMap(const char* name);

    /**
     * Save the current map to a file with the specified name.
     */
    void MP_SaveMap(const char* filename);

    /**
     * Initialize map loader for event processing. In particular, the map loader
     * will attach itself to the map size change event, to inject new meta data.
     * If map size changes otherwise, meta data shall remain the same.
     */
    void MP_InitMapLoader(void);

#ifdef	__cplusplus
}
#endif

#endif
