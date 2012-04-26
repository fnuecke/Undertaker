#include "bitset.h"
#include "jobs.h"
#include "map.h"
#include "players.h"
#include "selection.h"
#include "update.h"

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

/**
 * Selected blocks, per player.
 * This is a bitset, where each bit represents a block in the map.
 */
static BitSet gPlayerSelection[DK_PLAYER_COUNT] = {0};

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
static DK_Selection gCurrentSelection;

// TODO use local player via some variable (for network gaming)
static DK_Player gLocalPlayer = DK_PLAYER_ONE;

///////////////////////////////////////////////////////////////////////////////
// Utility
///////////////////////////////////////////////////////////////////////////////

/**
 * Validates a selection by inverting coordinates if necessary, so that start
 * coordinates are lower than or equal to end coordinates.
 * @param selection the selection to validate.
 */
static void validate(DK_Selection* selection) {
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
    DK_GetBlockUnderCursor(&gCurrentSelection.endX, &gCurrentSelection.endY);
    if (gMode == MODE_NONE) {
        gCurrentSelection.startX = gCurrentSelection.endX;
        gCurrentSelection.startY = gCurrentSelection.endY;
    }
}

static void onMapChange(void) {
    for (int i = 0; i < DK_PLAYER_COUNT; ++i) {
        BS_Delete(gPlayerSelection[i]);
        gPlayerSelection[i] = BS_New(DK_GetMapSize() * DK_GetMapSize());
    }
}

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

DK_Selection DK_GetSelection(void) {
    DK_Selection currentSelection = gCurrentSelection;
    validate(&currentSelection);
    return currentSelection;
}

int DK_IsBlockSelectable(DK_Player player, int x, int y) {
    const DK_Block* block = DK_GetBlockAt(x, y);
    return block &&
            (block->type == DK_BLOCK_DIRT ||
            block->type == DK_BLOCK_GOLD ||
            block->type == DK_BLOCK_GEM) &&
            (block->owner == DK_PLAYER_NONE || block->owner == player);
}

int DK_IsBlockSelected(DK_Player player, unsigned short x, unsigned short y) {
    return BS_Test(gPlayerSelection[player], y * DK_GetMapSize() + x);
}

///////////////////////////////////////////////////////////////////////////////
// User area selection
///////////////////////////////////////////////////////////////////////////////

int DK_BeginSelection(void) {
    if (gMode == MODE_NONE) {
        if (DK_IsBlockSelectable(gLocalPlayer, gCurrentSelection.startX, gCurrentSelection.startY)) {
            // OK, if it's selectable, start selection.
            if (DK_IsBlockSelected(gLocalPlayer, gCurrentSelection.startX, gCurrentSelection.startY)) {
                gMode = MODE_DESELECT;
            } else {
                gMode = MODE_SELECT;
            }
            return 1;
        }
    }
    return 0;
}

int DK_DiscardSelection(void) {
    if (gMode != MODE_NONE) {
        // Reset mode.
        gMode = MODE_NONE;
        gCurrentSelection.startX = gCurrentSelection.endX;
        gCurrentSelection.startY = gCurrentSelection.endY;
        return 1;
    }
    return 0;
}

void DK_ConfirmSelection(void) {
    if (gMode != MODE_NONE) {
        // Validate the selection.
        validate(&gCurrentSelection);

        // Set selection for the drawn area.
        for (int x = gCurrentSelection.startX; x <= gCurrentSelection.endX; ++x) {
            for (int y = gCurrentSelection.startY; y <= gCurrentSelection.endY; ++y) {
                if (gMode == MODE_SELECT) {
                    DK_SelectBlock(gLocalPlayer, x, y);
                } else if (gMode == MODE_DESELECT) {
                    DK_DeselectBlock(gLocalPlayer, x, y);
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

int DK_SelectBlock(DK_Player player, int x, int y) {
    if (DK_IsBlockSelectable(player, x, y)) {
        const unsigned int idx = y * DK_GetMapSize() + x;

        // Only update if something changed.
        if (!BS_Test(gPlayerSelection[player], idx)) {
            BS_Set(gPlayerSelection[player], idx);
            DK_FindJobs(player, x, y);
            return 1;
        }
    }
    return 0;
}

int DK_DeselectBlock(DK_Player player, int x, int y) {
    if (x >= 0 && y >= 0 && x < DK_GetMapSize() && y < DK_GetMapSize()) {
        const unsigned int idx = y * DK_GetMapSize() + x;

        // Only update if something changed.
        if (BS_Test(gPlayerSelection[player], idx)) {
            BS_Unset(gPlayerSelection[player], idx);
            DK_FindJobs(player, x, y);
            return 1;
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Initialization
///////////////////////////////////////////////////////////////////////////////

void DK_InitSelection(void) {
    DK_OnUpdate(onUpdate);
    DK_OnMapChange(onMapChange);
}
