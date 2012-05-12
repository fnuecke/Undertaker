#include "unit.h"

#include <float.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>

#include <GL/glew.h>

#include "astar.h"
#include "bitset.h"
#include "block.h"
#include "config.h"
#include "graphics.h"
#include "map.h"
#include "job.h"
#include "job_script.h"
#include "render.h"
#include "unit_ai.h"
#include "update.h"
#include "vmath.h"

///////////////////////////////////////////////////////////////////////////////
// Global variables
///////////////////////////////////////////////////////////////////////////////

/** Used for debug rendering of units and paths */
static GLUquadric* quadratic = 0;

/** Units in the game, must be ensured to be non-sparse */
static MP_Unit units[MP_PLAYER_COUNT * MP_UNITS_MAX_PER_PLAYER];

/** Total number of units */
static unsigned int gTotalUnitCount = 0;

/** Number of units per player */
static unsigned int gUnitCount[MP_PLAYER_COUNT] = {0};

///////////////////////////////////////////////////////////////////////////////
// Update
///////////////////////////////////////////////////////////////////////////////

static void onUpdate(void) {
    if (MP_d_ai_enabled) {
        for (unsigned int i = 0; i < gTotalUnitCount; ++i) {
            // Update the unit's AI state.
            MP_UpdateAI(&units[i]);
        }
    }
}

static void onMapChange(void) {
    for (unsigned int i = 0; i < MP_PLAYER_COUNT; ++i) {
        gUnitCount[i] = 0;
    }
    gTotalUnitCount = 0;
}

///////////////////////////////////////////////////////////////////////////////
// Render
///////////////////////////////////////////////////////////////////////////////

/** Catmull-rom interpolation */
static float cr(float p0, float p1, float p2, float p3, float t) {
    const float m1 = MP_AI_CATMULL_ROM_T * (p2 - p0);
    const float m2 = MP_AI_CATMULL_ROM_T * (p3 - p1);
    return (2 * p1 - 2 * p2 + m1 + m2) * t * t * t +
            (-3 * p1 + 3 * p2 - 2 * m1 - m2) * t * t +
            m1 * t +
            p1;
}

static void onRender(void) {
    MP_Material material;

    for (unsigned int i = 0; i < gTotalUnitCount; ++i) {
        const MP_Unit* unit = &units[i];
        //const AI_State* state = unit->ai->current;

        MP_InitMaterial(&material);
        material.specularIntensity = 0.9f;
        material.specularExponent = 25.0f;

        // Set color based on current job.
        material.diffuseColor.c.r = 0.6f;
        material.diffuseColor.c.g = 0.6f;
        material.diffuseColor.c.b = 0.6f;
        /*
                switch (state->jobType) {
                    case MP_JOB_DIG:
                        material.diffuseColor.c.r = 0.6f;
                        material.diffuseColor.c.g = 0.9f;
                        material.diffuseColor.c.b = 0.6f;
                        break;
                    case MP_JOB_CONVERT_TILE:
                    case MP_JOB_CONVERT_WALL:
                        material.diffuseColor.c.r = 0.6f;
                        material.diffuseColor.c.g = 0.6f;
                        material.diffuseColor.c.b = 0.9f;
                        break;
                    default:
                        material.diffuseColor.c.r = 0.6f;
                        material.diffuseColor.c.g = 0.6f;
                        material.diffuseColor.c.b = 0.6f;
                        break;
                }
         */
        // Highlight, if the unit is moving.
        if (MP_IsUnitMoving(unit)) {
            material.diffuseColor.c.r += 0.2f;
            material.diffuseColor.c.g += 0.2f;
            material.diffuseColor.c.b += 0.2f;
        }
        MP_SetMaterial(&material);

        // Push name of the unit for picking.
        glLoadName(i);

        // Render the unit.
        MP_PushModelMatrix();
        MP_TranslateModelMatrix(unit->position.d.x * MP_BLOCK_SIZE, unit->position.d.y * MP_BLOCK_SIZE, 4);
        gluSphere(quadratic, MP_BLOCK_SIZE / 6.0f, 16, 16);
        MP_PopModelMatrix();

        // If we display pathing render the units current path.
        if (MP_d_draw_paths && MP_IsUnitMoving(unit)) {
            const vec2* path = unit->ai->pathing.nodes;
            const unsigned int depth = unit->ai->pathing.depth;

            // Draw a line along the path.
            MP_InitMaterial(&material);
            material.emissivity = 1.0f;
            material.diffuseColor.c.r = 0.8f;
            material.diffuseColor.c.g = 0.8f;
            material.diffuseColor.c.b = 0.9f;
            MP_SetMaterial(&material);
            glLineWidth(1.75f);

            glBegin(GL_LINES);
            {
                unsigned int j = 0;
                glVertex3f(path[1].d.x * MP_BLOCK_SIZE, path[1].d.y * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT);
                for (j = 2; j <= depth; ++j) {
                    // Somewhere in the middle, smooth the path.
                    for (unsigned int k = 1; k < 20; ++k) {
                        const float t = k / 20.0f;
                        const float x = cr(path[j - 2].d.x, path[j - 1].d.x, path[j].d.x, path[j + 1].d.x, t);
                        const float y = cr(path[j - 2].d.y, path[j - 1].d.y, path[j].d.y, path[j + 1].d.y, t);
                        glVertex3f(x * MP_BLOCK_SIZE, y * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT);
                        glVertex3f(x * MP_BLOCK_SIZE, y * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT);
                    }
                }
                glVertex3f(path[j - 1].d.x * MP_BLOCK_SIZE, path[j - 1].d.y * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT);
            }
            glEnd();

            // Draw points marking the waypoints.
            material.diffuseColor.c.r = 0.8f;
            material.diffuseColor.c.g = 0.4f;
            material.diffuseColor.c.b = 0.4f;
            MP_SetMaterial(&material);

            for (unsigned int j = 1; j <= depth; ++j) {
                MP_PushModelMatrix();
                MP_TranslateModelMatrix(path[j].d.x * MP_BLOCK_SIZE, path[j].d.y * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT);
                gluSphere(quadratic, 0.5f, 8, 8);
                MP_PopModelMatrix();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

bool MP_IsUnitMoving(const MP_Unit* unit) {
    return unit->ai->pathing.index <= unit->ai->pathing.depth;
}

int MP_AddUnit(MP_Player player, const MP_UnitMeta* meta, const vec2* position) {
    MP_Unit* unit;

    // Unit definition valid?
    if (!meta) {
        return 0;
    }

    // Can there be any more units?
    if (gTotalUnitCount > MP_PLAYER_COUNT * MP_UNITS_MAX_PER_PLAYER) {
        return 0;
    }

    // Initialize a new unit. We do this up front because it's needed for
    // the next spawn point validity check.
    unit = &units[gTotalUnitCount];
    unit->meta = meta;
    unit->owner = player;
    unit->position = *position;

    // Can that kind of unit be spawned at the specified position?
    if (!MP_IsBlockPassableBy(MP_GetBlockAt((int) floorf(position->d.x), (int) floorf(position->d.y)), unit)) {
        return 0;
    }

    // OK, allocate AI data, if necessary.
    if (!unit->ai && !(unit->ai = calloc(1, sizeof (MP_AI_Info)))) {
        MP_log_fatal("Out of memory while allocating AI info.\n");
    } else {
        memset(unit->ai, 0, sizeof (MP_AI_Info));
    }

    // Disable movement initially.
    unit->ai->pathing.index = 1;

    // Allocate job saturation memory.
    if (!unit->satisfaction.jobSaturation &&
            !(unit->satisfaction.jobSaturation = calloc(unit->meta->jobCount, sizeof (float)))) {
        MP_log_fatal("Out of memory while allocating job saturation.\n");
    }

    // Set initial job saturations.
    for (unsigned int number = 0; number < unit->meta->jobCount; ++number) {
        unit->satisfaction.jobSaturation[number] =
                unit->meta->satisfaction.jobSaturation[number].initialValue;
    }

    // Increment counters.
    ++gTotalUnitCount;
    ++gUnitCount[player];

    // Send event to AI scripts.
    MP_Lua_OnUnitAdded(unit);

    return 1;
}

void MP_StopJob(MP_Job* job) {
    if (job && job->worker) {
        AI_State* state = &job->worker->ai->state;
        state->job = NULL;
        state->jobRunDelay = 0;
        state->jobSearchDelay = 0;
        state->active = false;
        job->worker = NULL;
    }
}

void MP_InitUnits(void) {
    MP_OnUpdate(onUpdate);
    MP_OnRender(onRender);
    MP_OnMapSizeChange(onMapChange);

    quadratic = gluNewQuadric();
}
