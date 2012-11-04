#include <assert.h>

#include "bitset.h"
#include "block.h"
#include "events.h"
#include "job.h"
#include "map.h"
#include "selection.h"
#include "script.h"

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

static unsigned int getIndex(const MP_Block* block) {
    unsigned short x, y;
    MP_GetBlockCoordinates(block, &x, &y);

    return y * MP_GetMapSize() + x;
}

///////////////////////////////////////////////////////////////////////////////
// Update
///////////////////////////////////////////////////////////////////////////////

static void onUpdate(void) {
    MP_GetBlockCoordinatesUnderCursor(&gCurrentSelection.endX, &gCurrentSelection.endY);
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

bool MP_IsBlockSelectable(const MP_Block* block, MP_Player player) {
    assert(block);
    assert(player > MP_PLAYER_NONE && player < MP_PLAYER_COUNT);

    return MP_IsBlockDestructible(block) &&
            (block->player == MP_PLAYER_NONE || block->player == player);
}

bool MP_IsBlockSelected(const MP_Block* block, MP_Player player) {
    assert(block);
    assert(player > MP_PLAYER_NONE && player < MP_PLAYER_COUNT);

    return BS_Test(gPlayerSelection[player], getIndex(block));
}

///////////////////////////////////////////////////////////////////////////////
// User area selection
///////////////////////////////////////////////////////////////////////////////

bool MP_BeginSelection(void) {
    if (gMode == MODE_NONE) {
        const MP_Block* block = MP_GetBlockAt(gCurrentSelection.startX, gCurrentSelection.startY);
        if (MP_IsBlockSelectable(block, gLocalPlayer)) {
            // OK, if it's selectable, start selection.
            if (MP_IsBlockSelected(block, gLocalPlayer)) {
                gMode = MODE_DESELECT;
            } else {
                gMode = MODE_SELECT;
            }
            return true;
        }
    }
    return false;
}

bool MP_DiscardSelection(void) {
    if (gMode != MODE_NONE) {
        // Reset mode.
        gMode = MODE_NONE;
        gCurrentSelection.startX = gCurrentSelection.endX;
        gCurrentSelection.startY = gCurrentSelection.endY;
        return true;
    }
    return false;
}

void MP_ConfirmSelection(void) {
    if (gMode != MODE_NONE) {
        // Validate the selection.
        validate(&gCurrentSelection);

        // Set selection for the drawn area.
        for (int x = gCurrentSelection.startX; x <= gCurrentSelection.endX; ++x) {
            for (int y = gCurrentSelection.startY; y <= gCurrentSelection.endY; ++y) {
                MP_Block* block = MP_GetBlockAt(x, y);
                assert(block);
                if (gMode == MODE_SELECT) {
                    MP_SelectBlock(block, gLocalPlayer);
                } else if (gMode == MODE_DESELECT) {
                    MP_DeselectBlock(block, gLocalPlayer);
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

void MP_SelectBlock(MP_Block* block, MP_Player player) {
    assert(block);
    assert(player > MP_PLAYER_NONE && player < MP_PLAYER_COUNT);
    if (MP_IsBlockSelectable(block, player)) {
        unsigned int idx = getIndex(block);

        // Only update if something changed.
        if (!BS_Test(gPlayerSelection[player], idx)) {
            BS_Set(gPlayerSelection[player], idx);
            // Send event to AI scripts.
            MP_DispatchBlockSelectionChangedEvent(block, player);
        }
    }
}

void MP_DeselectBlock(MP_Block* block, MP_Player player) {
    assert(block);
    assert(player > MP_PLAYER_NONE && player < MP_PLAYER_COUNT);
    {
        unsigned int idx = getIndex(block);

        // Only update if something changed.
        if (BS_Test(gPlayerSelection[player], idx)) {
            BS_Unset(gPlayerSelection[player], idx);
            // Send event to AI scripts.
            MP_DispatchBlockSelectionChangedEvent(block, player);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Initialization
///////////////////////////////////////////////////////////////////////////////

void MP_InitSelection(void) {
    MP_AddUpdateEventListener(onUpdate);
    MP_AddMapChangeEventListener(onMapChange);
}
