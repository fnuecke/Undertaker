/* 
 * File:   players.h
 * Author: fnuecke
 *
 * Created on April 14, 2012, 9:48 AM
 */

#ifndef PLAYERS_H
#define	PLAYERS_H

/** Player ids */
typedef enum {
    DK_PLAYER_NONE,
    DK_PLAYER_RED,
    DK_PLAYER_BLUE,
    DK_PLAYER_GREEN,
    DK_PLAYER_YELLOW,
    DK_PLAYER_WHITE,

    DK_PLAYER_COUNT //< Number of possible values, used for array init.
} DK_Player;

#endif	/* PLAYERS_H */

