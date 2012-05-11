#include "astar_mp.h"

#include <math.h>

#include "astar.h"
#include "block.h"
#include "map.h"
#include "unit.h"

static const MP_Unit* gUnit;

static int isPassable(float x, float y) {
    return MP_IsBlockPassableBy(MP_GetBlockAt((int) floorf(x), (int) floorf(y)), gUnit);
}

int MP_AStar(const MP_Unit* unit, const vec2* goal, vec2* path, unsigned int* depth, float* length) {
    // Remember we're working on this unit (passable checks).
    gUnit = unit;
    return AStar(&unit->position, goal, isPassable, MP_GetMapSize(), path, depth, length);
}
