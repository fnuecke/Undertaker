#include <assert.h>
#include <math.h>

#include "astar.h"
#include "astar_mp.h"
#include "block.h"
#include "map.h"
#include "unit.h"
#include "room.h"

static const MP_UnitType* gUnitType = NULL;

static int isPassable(float x, float y) {
    const MP_Block* block = MP_GetBlockAt((int) floorf(x), (int) floorf(y));
    return block && MP_IsBlockPassableBy(block, gUnitType);
}

bool MP_AStar(const MP_Unit* unit, const vec2* goal, vec2* path, unsigned int* depth, float* length) {
    assert(unit);

    // Remember we're working on this unit (passable checks).
    gUnitType = unit->type;
    return (bool) AStar(&unit->position, goal, isPassable, MP_GetMapSize(), path, depth, length);
}
