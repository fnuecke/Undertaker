/* 
 * File:   selection.h
 * Author: fnuecke
 *
 * Created on April 18, 2012, 3:57 PM
 */

#ifndef SELECTION_H
#define	SELECTION_H

#include "players.h"

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
} DK_Selection;

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

/**
 * Get the currently selected region.
 * @return the currently selected area.
 */
DK_Selection DK_GetSelection(void);

/**
 * Checks if the specified block is selectable by the specified player.
 * @return whether the block is selectable (1) or not (0).
 */
int DK_IsBlockSelectable(DK_Player player, int x, int y);

/**
 * Checks if the specified block is selected by the specified player.
 * @return whether the block is selected (1) or not (0).
 */
int DK_IsBlockSelected(DK_Player player, unsigned short x, unsigned short y);

///////////////////////////////////////////////////////////////////////////////
// User area selection
///////////////////////////////////////////////////////////////////////////////

/**
 * Start selecting an area of the map, uses currently hovered block as start.
 * This will do nothing if the selection was started before, or the block
 * currently under the cursor is not selectable.
 * @return whether selection started (1) or not (0).
 */
int DK_BeginSelection(void);

/**
 * Cancel selecting. This will discard the current selection.
 * @return whether we were in select mode (1) or not (0).
 */
int DK_DiscardSelection(void);

/**
 * Done selecting, uses currently hovered block as end. This will automatically
 * select all blocks in the selection for the local player. This will do nothing
 * if the selection was not successfully started before.
 */
void DK_ConfirmSelection(void);

///////////////////////////////////////////////////////////////////////////////
// Modifiers
///////////////////////////////////////////////////////////////////////////////

/**
 * Select the block at the specified coordinates for the specified player. Will
 * do nothing if the block is not selectable. If a selection change took place,
 * this will also trigger an update to the nearby jobs.
 * @param player the player to select the block for.
 * @param x the x coordinate of the block, in map space.
 * @param y the y coordinate of the block, in map space.
 */
void DK_SelectBlock(DK_Player player, int x, int y);

/**
 * Deselect the block at the specified coordinates for the specified player.
 * Will do nothing if the coordinates are invalid. If a selection change took
 * place, this will also trigger an update to the nearby jobs.
 * @param player the player to deselect the block for.
 * @param x the x coordinate of the block, in map space.
 * @param y the y coordinate of the block, in map space.
 */
void DK_DeselectBlock(DK_Player player, int x, int y);

///////////////////////////////////////////////////////////////////////////////
// Initialization / Update
///////////////////////////////////////////////////////////////////////////////

/**
 * (Re)Initialize selection data structures after map change.
 */
void DK_InitSelection(void);

/**
 * Update the current selection. Should be called each frame.
 */
void DK_UpdateSelection(void);

#ifdef	__cplusplus
}
#endif

#endif	/* SELECTION_H */

