/* 
 * File:   selection.h
 * Author: fnuecke
 *
 * Created on April 18, 2012, 3:57 PM
 */

#ifndef SELECTION_H
#define	SELECTION_H

#ifdef	__cplusplus
extern "C" {
#endif

/** Checks if the specified block is selectable by the specified player */
int DK_block_is_selectable(DK_Player player, int x, int y);

/** Select the block at the given coordinates for the specified player */
void DK_block_select(DK_Player player, unsigned short x, unsigned short y);

/** Deselect the block at the given coordinates for the specified player */
void DK_block_deselect(DK_Player player, unsigned short x, unsigned short y);

/** Checks if the specified block is selected by the specified player */
int DK_block_is_selected(DK_Player player, unsigned short x, unsigned short y);

#ifdef	__cplusplus
}
#endif

#endif	/* SELECTION_H */

