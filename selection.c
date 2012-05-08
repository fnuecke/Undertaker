#include "selection.h"

#include "bitset.h"
#include "block.h"
#include "jobs.h"
#include "map.h"
#include "update.h"

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

/**
 * Selected blocks, per player.
 * This is a bitset, where each bit represents a block in the map.
 */
static BitSet gPlayerSelection[MP_PLAYER_COUNT] = {0};

/**
 * Current local selection mode.
 */
static enum {
    /**
     * Not currently selecting anything.
     */
    MODE_NONE,

    /**
     * Currently choosing area to select more blocks.
     */
    MODE_SELECT,

    /**
     * Currently choosing area to deselect some blocks.
     */
    MODE_DESELECT
} gMode = MODE_NONE;

/**
 * The current area selection for the local player.
 */
static MP_Selection gCurrentSelection;

// TODO use local player via some variable (for network gaming)
static MP_Player gLocalPlayer = MP_PLAYER_ONE;

///////////////////////////////////////////////////////////////////////////////
// Utility
///////////////////////////////////////////////////////////////////////////////

/**
 * Validates a selection by inverting coordinates if necessary, so that start
 * coordinates are lower than or equal to end coordinates.
 * @param selection the selection to validate.
 */
static void validate(MP_Selection* selection) {
    if (selection->startX > selection->endX) {
        int tmp = selection->endX;
        selection->endX = selection->startX;
        selection->startX = tmp;
    }
    if (selection->startY > selection->endY) {
        int tmp = selection->endY;
        selection->endY = selection->startY;
        selection->startY = tmp;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Update
///////////////////////////////////////////////////////////////////////////////

static void onUpdate(void) {
    MP_GetBlockUnderCursor(&gCurrentSelection.endX, &gCurrentSelection.endY);
    if (gMode == MODE_NONE) {
        gCurrentSelection.startX = gCurrentSelection.endX;
        gCurrentSelection.startY = gCurrentSelection.endY;
    }
}

static void onMapChange(void) {
    for (int i = 0; i < MP_PLAYER_COUNT; ++i) {
        BS_Delete(gPlayerSelection[i]);
        gPlayerSelection[i] = BS_New(MP_GetMapSize() * MP_GetMapSize());
    }
}

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

MP_Selection MP_GetSelection(void) {
    MP_Selection currentSelection = gCurrentSelection;
    validate(&currentSelection);
    return currentSelection;
}

int MP_IsBlockSelectable(MP_Player player, int x, int y) {
    const MP_Block* block = MP_GetBlockAt(x, y);
    return MP_IsBlockDestructible(block) &&
            (block->owner == MP_PLAYER_NONE || block->owner == player);
}

int MP_IsBlockSelected(MP_Player player, unsigned short x, unsigned short y) {
    return BS_Test(gPlayerSelection[player], y * MP_GetMapSize() + x);
}

///////////////////////////////////////////////////////////////////////////////
// User area selection
///////////////////////////////////////////////////////////////////////////////

int MP_BeginSelection(void) {
    if (gMode == MODE_NONE) {
        if (MP_IsBlockSelectable(gLocalPlayer, gCurrentSelection.startX, gCurrentSelection.startY)) {
            // OK, if it's selectable, start selection.
            if (MP_IsBlockSelected(gLocalPlayer, gCurrentSelection.startX, gCurrentSelection.startY)) {
                gMode = MODE_DESELECT;
            } else {
                gMode = MODE_SELECT;
            }
            return 1;
        }
    }
    return 0;
}

int MP_DiscardSelection(void) {
    if (gMode != MODE_NONE) {
        // Reset mode.
        gMode = MODE_NONE;
        gCurrentSelection.startX = gCurrentSelection.endX;
        gCurrentSelection.startY = gCurrentSelection.endY;
        return 1;
    }
    return 0;
}

void MP_ConfirmSelection(void) {
    if (gMode != MODE_NONE) {
        // Validate the selection.
        validate(&gCurrentSelection);

        // Set selection for the drawn area.
        for (int x = gCurrentSelection.startX; x <= gCurrentSelection.endX; ++x) {
            for (int y = gCurrentSelection.startY; y <= gCurrentSelection.endY; ++y) {
                if (gMode == MODE_SELECT) {
                    MP_SelectBlock(gLocalPlayer, x, y);
                } else if (gMode == MODE_DESELECT) {
                    MP_DeselectBlock(gLocalPlayer, x, y);
                }
            }
        }

        // Reset mode.
        gMode = MODE_NONE;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Modifiers
///////////////////////////////////////////////////////////////////////////////

int MP_SelectBlock(MP_Player player, int x, int y) {
    if (MP_IsBlockSelectable(player, x, y)) {
        const unsigned int idx = y * MP_GetMapSize() + x;

        // Only update if something changed.
        if (!BS_Test(gPlayerSelection[player], idx)) {
            BS_Set(gPlayerSelection[player], idx);
            //MP_UpdateJobsForBlock(player, x, y);
            // TODO event
            return 1;
        }
    }
    return 0;
}

int MP_DeselectBlock(MP_Player player, int x, int y) {
    if (x >= 0 && y >= 0 && x < MP_GetMapSize() && y < MP_GetMapSize()) {
        const unsigned int idx = y * MP_GetMapSize() + x;

        // Only update if something changed.
        if (BS_Test(gPlayerSelection[player], idx)) {
            BS_Unset(gPlayerSelection[player], idx);
            //MP_UpdateJobsForBlock(player, x, y);
            // TODO event
            return 1;
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Initialization
///////////////////////////////////////////////////////////////////////////////

void MP_InitSelection(void) {
    MP_OnUpdate(onUpdate);
    MP_OnMapSizeChange(onMapChange);
}
