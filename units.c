#include "units.h"

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
#include "jobs.h"
#include "render.h"
#include "units_ai.h"
#include "update.h"
#include "vmath.h"

///////////////////////////////////////////////////////////////////////////////
// Global variables
///////////////////////////////////////////////////////////////////////////////

/** Used for debug rendering of units and paths */
static GLUquadric* quadratic = 0;

/** Units in the game, must be ensured to be non-sparse */
static DK_Unit units[DK_PLAYER_COUNT * DK_UNITS_MAX_PER_PLAYER];

/** Total number of units */
static unsigned int gTotalUnitCount = 0;

/** Number of units per player */
static unsigned int gUnitCount[DK_PLAYER_COUNT] = {0};

///////////////////////////////////////////////////////////////////////////////
// Update
///////////////////////////////////////////////////////////////////////////////

static void onUpdate(void) {
    if (DK_d_ai_enabled) {
        for (unsigned int i = 0; i < gTotalUnitCount; ++i) {
            // Update the unit's AI state.
            DK_UpdateAI(&units[i]);
        }
    }
}

static void onMapChange(void) {
    for (unsigned int i = 0; i < DK_PLAYER_COUNT; ++i) {
        gUnitCount[i] = 0;
    }
    gTotalUnitCount = 0;
}

///////////////////////////////////////////////////////////////////////////////
// Render
///////////////////////////////////////////////////////////////////////////////

static void onRender(void) {
    DK_Material material;

    for (unsigned int i = 0; i < gTotalUnitCount; ++i) {
        const DK_Unit* unit = &units[i];
        //const AI_State* state = unit->ai->current;

        DK_InitMaterial(&material);
        material.specularIntensity = 0.9f;
        material.specularExponent = 25.0f;

        // Set color based on current job.
        material.diffuseColor.c.r = 0.6f;
        material.diffuseColor.c.g = 0.6f;
        material.diffuseColor.c.b = 0.6f;
        /*
                switch (state->jobType) {
                    case DK_JOB_DIG:
                        material.diffuseColor.c.r = 0.6f;
                        material.diffuseColor.c.g = 0.9f;
                        material.diffuseColor.c.b = 0.6f;
                        break;
                    case DK_JOB_CONVERT_TILE:
                    case DK_JOB_CONVERT_WALL:
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
        if (DK_IsUnitMoving(unit)) {
            material.diffuseColor.c.r += 0.2f;
            material.diffuseColor.c.g += 0.2f;
            material.diffuseColor.c.b += 0.2f;
        }
        DK_SetMaterial(&material);

        // Push name of the unit for picking.
        glLoadName(i);

        // Render the unit.
        DK_PushModelMatrix();
        DK_TranslateModelMatrix(unit->position.d.x * DK_BLOCK_SIZE, unit->position.d.y * DK_BLOCK_SIZE, 4);
        gluSphere(quadratic, DK_BLOCK_SIZE / 6.0f, 16, 16);
        DK_PopModelMatrix();

        // If we display pathing render the units current path.
        if (DK_d_draw_paths && DK_IsUnitMoving(unit)) {
            const vec2* path = unit->ai->pathing.nodes;
            const unsigned int depth = unit->ai->pathing.depth;

            // Draw a line along the path.
            DK_InitMaterial(&material);
            material.emissivity = 1.0f;
            material.diffuseColor.c.r = 0.8f;
            material.diffuseColor.c.g = 0.8f;
            material.diffuseColor.c.b = 0.9f;
            DK_SetMaterial(&material);
            glLineWidth(1.75f);

            glBegin(GL_LINES);
            {
                unsigned int j = 0;
                glVertex3f(path[1].d.x * DK_BLOCK_SIZE, path[1].d.y * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT);
                /*
                                for (j = 2; j <= depth; ++j) {
                                    // Somewhere in the middle, smooth the path.
                                    for (unsigned int k = 1; k < 20; ++k) {
                                        const float t = k / 20.0f;
                                        const float x = catmull_rom(path[j - 2].d.x, path[j - 1].d.x, path[j].d.x, path[j + 1].d.x, t);
                                        const float y = catmull_rom(path[j - 2].d.y, path[j - 1].d.y, path[j].d.y, path[j + 1].d.y, t);
                                        glVertex3f(x * DK_BLOCK_SIZE, y * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT);
                                        glVertex3f(x * DK_BLOCK_SIZE, y * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT);
                                    }
                                }
                 */
                glVertex3f(path[j - 1].d.x * DK_BLOCK_SIZE, path[j - 1].d.y * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT);
            }
            glEnd();

            // Draw points marking the waypoints.
            material.diffuseColor.c.r = 0.8f;
            material.diffuseColor.c.g = 0.4f;
            material.diffuseColor.c.b = 0.4f;
            DK_SetMaterial(&material);

            for (unsigned int j = 1; j <= depth; ++j) {
                DK_PushModelMatrix();
                DK_TranslateModelMatrix(path[j].d.x * DK_BLOCK_SIZE, path[j].d.y * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT);
                gluSphere(quadratic, 0.5f, 8, 8);
                DK_PopModelMatrix();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

bool DK_IsUnitMoving(const DK_Unit* unit) {
    return unit->ai->pathing.index < unit->ai->pathing.depth;
}

int DK_AddUnit(DK_Player player, const DK_UnitMeta* type, const vec2* position) {
    DK_Unit* unit = &units[gTotalUnitCount];

    // Can there be any more units?
    if (gTotalUnitCount > DK_PLAYER_COUNT * DK_UNITS_MAX_PER_PLAYER) {
        return 0;
    }

    // Initialize a new unit. We do this up front because it's needed for
    // the next spawn point validity check.
    unit->meta = type;
    unit->owner = player;
    unit->position = *position;

    // Can that kind of unit be spawned at the specified position?
    if (!DK_IsBlockPassableBy(DK_GetBlockAt((int) floorf(position->d.x), (int) floorf(position->d.y)), unit)) {
        return 0;
    }

    // OK, allocate AI data, if necessary.
    if (!unit->ai && !(unit->ai = calloc(1, sizeof (DK_AI_Info)))) {
        fprintf(stderr, "Out of memory while allocating AI info.\n");
        exit(EXIT_FAILURE);
    }

    // Set the pointer past the lower stack bound, so we know we have to
    // find something to do.
    unit->ai->current = &unit->ai->stack[DK_AI_STACK_DEPTH];

    // Increment counters.
    ++gTotalUnitCount;
    ++gUnitCount[player];

    return 1;
}

void DK_StopJob(DK_Job* job) {
    if (job && job->worker) {
        // Traverse the AI stack, starting at the current node.
        AI_State* state = job->worker->ai->current;
        while (state < &job->worker->ai->stack[DK_AI_STACK_DEPTH]) {
            // If we have a match, make it the current one and tell it to stop.
            if (state->job == job) {
                job->worker->ai->current = state;
                job->worker = NULL;
                state->job = NULL;
                state->jobNumber = 0;
                state->shouldCancel = true;

                // And we're done.
                return;
            } else {
                // Move on to the next.
                ++state;
            }
        }
    }
}

void DK_InitUnits(void) {
    DK_OnUpdate(onUpdate);
    DK_OnRender(onRender);
    DK_OnMapSizeChange(onMapChange);

    quadratic = gluNewQuadric();
}
