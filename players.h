/* 
 * File:   players.h
 * Author: fnuecke
 *
 * Created on April 14, 2012, 9:48 AM
 */

#ifndef PLAYERS_H
#define	PLAYERS_H

/**
 * Player IDs for possible players in a game.
 */
typedef enum {
    /**
     * No player / invalid player.
     */
    DK_PLAYER_NONE,

    /**
     * First player.
     */
    DK_PLAYER_ONE,

    /**
     * Second player.
     */
    DK_PLAYER_TWO,

    /**
     * Third player.
     */
    DK_PLAYER_THREE,

    /**
     * Fourth player.
     */
    DK_PLAYER_FOUR,

    /**
     * Fifth player.
     */
    DK_PLAYER_FIVE,

    /**
     * Number of possible values, used for array initialization.
     */
    DK_PLAYER_COUNT
} DK_Player;

#endif	/* PLAYERS_H */
