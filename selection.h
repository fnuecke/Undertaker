/* 
 * Author: fnuecke
 *
 * Created on April 18, 2012, 3:57 PM
 */

#ifndef SELECTION_H
#define	SELECTION_H

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////////
    // Types
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Used for making current selection accessible.
     */
    typedef struct {
        /**
         * The x/y coordinate where the selection starts. Guaranteed to be less or
         * equal to the end coordinate on either axis.
         */
        int startX, startY;

        /**
         * The x/y coordinate where the selection ends. Guaranteed to be larger or
         * equal to the start coordinate on either axis.
         */
        int endX, endY;
    } MP_Selection;

    ///////////////////////////////////////////////////////////////////////////////
    // Accessors
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Get the currently selected region.
     * @return the currently selected area.
     */
    MP_Selection MP_GetSelection(void);

    /**
     * Checks if the specified block is selectable by the specified player.
     * @param player the player for whom to check.
     * @param block the block for which to check.
     * @return whether the block is selectable (1) or not (0).
     */
    bool MP_IsBlockSelectable(const MP_Block* block, MP_Player player);

    /**
     * Checks if the specified block is selected by the specified player.
     * @return whether the block is selected (1) or not (0).
     */
    bool MP_IsBlockSelected(const MP_Block* block, MP_Player player);

    ///////////////////////////////////////////////////////////////////////////////
    // User area selection
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Start selecting an area of the map, uses currently hovered block as start.
     * This will do nothing if the selection was started before, or the block
     * currently under the cursor is not selectable.
     * @return whether selection started (1) or not (0).
     */
    bool MP_BeginSelection(void);

    /**
     * Cancel selecting. This will discard the current selection.
     * @return whether we were in select mode (1) or not (0).
     */
    bool MP_DiscardSelection(void);

    /**
     * Done selecting, uses currently hovered block as end. This will automatically
     * select all blocks in the selection for the local player. This will do nothing
     * if the selection was not successfully started before.
     */
    void MP_ConfirmSelection(void);

    ///////////////////////////////////////////////////////////////////////////////
    // Modifiers
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Select the specified block for the specified player.
     * Will do nothing if the block is not selectable. If a selection change
     * took place, this will also trigger an event for AI scripts.
     * @param the block.
     * @param player the player to select the block for.
     */
    void MP_SelectBlock(MP_Block* block, MP_Player player);

    /**
     * Deselect the specified block for the specified player.
     * Will do nothing if the coordinates are invalid. If a selection change
     * took place, this will also trigger an event for AI scripts.
     * @param player the player to deselect the block for.
     */
    void MP_DeselectBlock(MP_Block* block, MP_Player player);

    ///////////////////////////////////////////////////////////////////////////////
    // Initialization / Update
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Initialize selection logic.
     */
    void MP_InitSelection(void);

#ifdef	__cplusplus
}
#endif

#endif
