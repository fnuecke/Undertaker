#include "hand.h"

#include <float.h>
#include <malloc.h>

#include "unit.h"
#include "unit_ai.h"
#include "map.h"
#include "block.h"

typedef enum HandEntryType {
    UNIT,
    GOLD
} HandEntryType;

typedef struct HandEntry {
    HandEntryType type;
    MP_Unit* unit;
    unsigned int gold;
} HandEntry;

static HandEntry* gHand[MP_PLAYER_COUNT] = {0};
static unsigned int gObjectsInHandCount[MP_PLAYER_COUNT] = {0};
static unsigned int gObjectsInHandCapacity[MP_PLAYER_COUNT] = {0};

static void ensureCapacity(MP_Player player) {
    if (gObjectsInHandCount[player] >= gObjectsInHandCapacity[player]) {
        gObjectsInHandCapacity[player] = gObjectsInHandCapacity[player] * 2 + 1;
        if (!(gHand[player] = realloc(gHand[player], gObjectsInHandCapacity[player] * sizeof (HandEntry)))) {
            MP_log_fatal("Out of memory while resizing hand list.\n");
        }
    }
}

void MP_PickUpUnit(MP_Player player, MP_Unit* unit) {
    HandEntry* entry;

    ensureCapacity(player);

    entry = &gHand[player][gObjectsInHandCount[player]++];
    entry->type = UNIT;
    entry->unit = unit;

    unit->position.d.x = -FLT_MAX;
    unit->position.d.y = -FLT_MAX;
    unit->ai->isInHand = true;
}

void MP_PickUpGold(MP_Player player, unsigned int amount) {
    HandEntry* entry;

    ensureCapacity(player);

    entry = &gHand[player][gObjectsInHandCount[player]++];
    entry->type = GOLD;
    entry->gold = amount;
}

void MP_DropTopHandEntry(MP_Player player, const vec2* position) {
    if (position && gObjectsInHandCount[player] > 0) {
        const HandEntry* entry = &gHand[player][gObjectsInHandCount[player] - 1];
        const MP_Block* block = MP_GetBlockUnderCursor();
        if (MP_IsBlockPassable(block) && block->owner == player) {
            switch (entry->type) {
                case UNIT:
                    if (MP_IsBlockPassableBy(block, entry->unit->meta)) {
                        entry->unit->position = *position;
                        entry->unit->ai->isInHand = false;
                        // Don't continue moving (would jump the unit to that path).
                        entry->unit->ai->pathing.index = entry->unit->ai->pathing.depth + 1;
                        // Immediately look for a new job.
                        entry->unit->ai->state.jobSearchDelay = 0;
                        entry->unit->ai->state.jobRunDelay = 0;

                        // Drop successful.
                        --gObjectsInHandCount[player];
                    }
                    break;
                case GOLD:
                    break;
            }
        }
    }
}

unsigned int MP_ObjectsInHandCount(MP_Player player) {
    return gObjectsInHandCount[player];
}
