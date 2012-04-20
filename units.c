#include <float.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>

#include "GLee.h"
#include <GL/glu.h>

#include "astar.h"
#include "bitset.h"
#include "config.h"
#include "units.h"
#include "map.h"
#include "jobs.h"

///////////////////////////////////////////////////////////////////////////////
// Abilities
///////////////////////////////////////////////////////////////////////////////

/** Imp abilities */
enum {
    DK_ABILITY_IMP_ATTACK,
    DK_ABILITY_IMP_CONVERT
};

/** Wizard abilities */
enum {
    DK_ABILITY_WIZARD_ATTACK,
    DK_ABILITY_WIZARD_FIREBALL
};

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////

/** Must be in same order as unit definitions in unit type enum */
static float move_speeds[] = {
    [DK_UNIT_IMP] = 0.6f,
    [DK_UNIT_WIZARD] = 0.4f
};

/** Cooldowns for different abilities */
static unsigned int cooldowns[][DK_UNITS_MAX_ABILITIES] = {
    [DK_UNIT_IMP] =
    {
        [DK_ABILITY_IMP_ATTACK] = 8,
        [DK_ABILITY_IMP_CONVERT] = 12
    },
    [DK_UNIT_WIZARD] =
    {
        [DK_ABILITY_WIZARD_ATTACK] = 16,
        [DK_ABILITY_WIZARD_FIREBALL] = 32
    }
};

/** Damage of different abilities */
static unsigned int damage[][DK_UNITS_MAX_ABILITIES] = {
    [DK_UNIT_IMP] =
    {
        [DK_ABILITY_IMP_ATTACK] = 10,
        [DK_ABILITY_IMP_CONVERT] = 10
    },
    [DK_UNIT_WIZARD] =
    {
        [DK_ABILITY_WIZARD_ATTACK] = 20,
        [DK_ABILITY_WIZARD_FIREBALL] = 30
    }
};

///////////////////////////////////////////////////////////////////////////////
// AI Stuff
///////////////////////////////////////////////////////////////////////////////

typedef enum {
    /** AI does nothing */
    DK_AI_NONE,

    /** AI is idling (randomly roaming nearby area) */
    DK_AI_IDLE,

    /** AI is moving to a target position */
    DK_AI_MOVE,

    /** AI is attacking a target unit */
    DK_AI_ATTACK,

    /** Imp is chipping away at a block */
    DK_AI_IMP_DIG,

    /** Imp is conquering a block */
    DK_AI_IMP_CONVERT,

    /** AI is working on something (training, research, crafting) */
    DK_AI_WORK
} AI_Job;

typedef struct {
    unsigned int delay;
    unsigned int wander_delay;
} AIJob_Idle;

typedef struct {
    /** The path the unit currently follows (if moving) */
    AStar_Waypoint path[DK_AI_PATH_MAX];

    /** The total depth of the path (number of nodes) */
    unsigned int path_depth;

    /** The current node of the path */
    unsigned int path_index;
} AIJob_Move;

typedef struct {
    /** The actual job this task represents */
    DK_Job* job;
} AIJob_DigConvert;

typedef struct {
    /** Enemy the unit currently attacks */
    unsigned int enemy;

} AIJob_Attack;

/** Get size of largest info struct */
#define SIZE0 sizeof(AIJob_Idle)
#define SIZE1 sizeof(AIJob_Move)
#define SIZE2 sizeof(AIJob_DigConvert)
#define SIZE3 sizeof(AIJob_Attack)
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MAX_JOB_SIZE MAX(MAX(MAX(SIZE0, SIZE1), SIZE2), SIZE3)

typedef struct {
    /** The type of this AI entry */
    AI_Job state;

    /** Statically allocated data for this AI entry */
    char info[MAX_JOB_SIZE];
} AI_Node;

typedef struct DK_Unit {
    /** The player this unit belongs to */
    DK_Player owner;

    /** The type of the unit */
    DK_UnitType type;

    /** Current position of the unit */
    float x, y;

    /** Target position the unit is moving to */
    float tx, ty;

    /** The unit's movement speed */
    float ms;

    /** Cooldowns for unit abilities */
    unsigned int cooldowns[DK_UNITS_MAX_ABILITIES];

    /** Level of the unit, as a float to account for "experience" */
    float level;

    /** Depth of the AI state stack */
    unsigned int ai_count;

    /** AI info stack */
    AI_Node ai[DK_AI_JOB_STACK_MAX];
} DK_Unit;

///////////////////////////////////////////////////////////////////////////////
// Global variables
///////////////////////////////////////////////////////////////////////////////

/** Used for debug rendering of units and paths */
static GLUquadric* quadratic = 0;

/** Units in the game, must be ensured to be non-sparse */
static DK_Unit units[DK_PLAYER_COUNT * DK_UNITS_MAX_PER_PLAYER] = {0};

/** Total number of units */
static unsigned int total_unit_count = 0;

/** Number of units per player */
static unsigned int unit_count[DK_PLAYER_COUNT] = {0};

///////////////////////////////////////////////////////////////////////////////
// Internal update methods
///////////////////////////////////////////////////////////////////////////////

static void update_position(DK_Unit* unit) {
    // Get unit vector to target.
    const float dx = unit->tx - unit->x;
    const float dy = unit->ty - unit->y;
    if (fabs(dx) > 0.001f || fabs(dy) > 0.001f) {
        const float l = sqrt(dx * dx + dy * dy);

        if (l <= unit->ms) {
            // This step will get us to our target position.
            unit->x = unit->tx;
            unit->y = unit->ty;
        } else {
            // Still have some way to go, normalize deltas and apply movement speed
            // to update the position.
            /*
                        const float pdx = unit->px - unit->x;
                        const float pdy = unit->py - unit->y;
                        const float pl = sqrt(pdx * pdx + pdy * pdy);

                        const float s = l / pl; // scale s to go from 0 to 1
                        const float s2 = s + s;
                        const float s3 = s2 + s;
                        const float sp2 = s * s;
                        const float sp3 = s * s * s;
                        const float s2p3 = s2 * s2*s2;
                        const float s3p2 = s3 * s3;
                        const float h1 = s2p3 - s3p2 + 1; // calculate basis function 1
                        const float h2 = s3p2 - s2p3; // calculate basis function 2
                        const float h3 = sp3 - 2 * sp2 + s; // calculate basis function 3
                        const float h4 = sp3 - sp2; // calculate basis function 4
                        const float px = h1 * unit->px + h2 * unit->tx + h3 * unit->tx + h;
                        vector p = h1 * P1 + // multiply and sum all funtions
                                h2 * P2 + // together to build the interpolated
                                h3 * T1 + // point along the curve.
                                h4*T2;
             */

            const float factor = unit->ms / l;
            unit->x += dx * factor;
            unit->y += dy * factor;
        }
    } else {
        // Pretty much at target, set directly.
        unit->x = unit->tx;
        unit->y = unit->ty;
    }
}

static void update_ai(DK_Unit* unit) {
    if (unit->ai_count == 0) {
        // Switch to idle state if there is none set.
        unit->ai[0].state = DK_AI_IDLE;
        AIJob_Idle* idle = (AIJob_Idle*) unit->ai[0].info;
        idle->delay = DK_AI_IDLE_DELAY / 2 + (rand() * DK_AI_IDLE_DELAY / 2 / RAND_MAX);
        idle->wander_delay = DK_AI_WANDER_DELAY;

        unit->ai_count = 1;
    }
    const AI_Node* ai = &unit->ai[unit->ai_count - 1];
    switch (ai->state) {
        case DK_AI_IDLE:
        {
            AIJob_Idle* idle = (AIJob_Idle*) ai->info;
            if (idle->delay > 0) {
                // Still waiting, skip for now.
                --idle->delay;
                break;
            }
            idle->delay = DK_AI_IDLE_DELAY / 2 + (rand() * DK_AI_IDLE_DELAY / 2 / RAND_MAX);

            // Find the unit something to do.
            if (unit->type == DK_UNIT_IMP) {
                // Look for the closest workplace.
                float best_distance = FLT_MAX;
                float best_penalty = 0;
                DK_Job* best_job = NULL;

                // Assuming we'll find work we'll want to add two jobs, one for
                // the actual work task, one for getting there.
                AI_Node* jobNode = &unit->ai[unit->ai_count];
                AI_Node* moveNode = &unit->ai[unit->ai_count + 1];
                AIJob_Move* move = (AIJob_Move*) moveNode->info;
                moveNode->state = DK_AI_MOVE;

                unsigned int job_count;
                DK_Job** jobs = DK_jobs(unit->owner, &job_count);
                for (; job_count > 0; --job_count) {
                    // Current workplace we're checking.
                    DK_Job* job = jobs[job_count - 1];

                    // Test direct distance; if that's longer than our best we
                    // can safely skip this one.
                    const float dx = unit->x - job->x * DK_BLOCK_SIZE;
                    const float dy = unit->y - job->y * DK_BLOCK_SIZE;
                    float distance = sqrt(dx * dx + dy * dy);
                    if (distance >= best_distance + best_penalty) {
                        continue;
                    }

                    // Find a path to it. Use temporary output data to avoid
                    // overriding existing path that may be shorter.
                    AStar_Waypoint path[DK_AI_PATH_MAX] = {0};
                    unsigned int depth = DK_AI_PATH_MAX;
                    if (DK_a_star(unit->x / DK_BLOCK_SIZE, unit->y / DK_BLOCK_SIZE, job->x, job->y, path, &depth, &distance)) {
                        // Scale to world units.
                        distance *= DK_BLOCK_SIZE;

                        // Factor in priorities.
                        float penalty = 0;
                        switch (job->type) {
                            case DK_JOB_DIG:
                                penalty = DK_JOB_DIG_PRIORITY;
                                break;
                            case DK_JOB_CONVERT:
                                if (DK_block_is_passable(job->block)) {
                                    penalty = DK_JOB_CONVERT_FLOOR_PRIORITY;
                                } else {
                                    penalty = DK_JOB_CONVERT_WALL_PRIORITY;
                                }
                                break;

                        }

                        // Got a path, check its length.
                        if (distance + penalty < best_distance + best_penalty) {
                            // Got a new best. Check if it's occupied, and if so
                            // only take it if our path is better than the
                            // direct distance to the occupant.
                            if (job->worker) {
                                const float cdx = job->worker->x - job->x * DK_BLOCK_SIZE;
                                const float cdy = job->worker->y - job->y * DK_BLOCK_SIZE;
                                const float current_distance = sqrt(cdx * cdx + cdy * cdy);
                                if (current_distance <= distance + DK_AI_ALREADY_WORKING_BONUS) {
                                    // The one that's on it is closer, ignore.
                                    continue;
                                }
                                // We're closer, kick the other one.
                                DK_unit_cancel_job(job->worker);
                            }

                            // Copy data.
                            best_distance = distance;
                            best_penalty = penalty;
                            best_job = job;
                            memcpy(move->path, path, depth * sizeof (AStar_Waypoint));
                            move->path_depth = depth;
                            switch (job->type) {
                                case DK_JOB_DIG:
                                    jobNode->state = DK_AI_IMP_DIG;
                                    break;
                                case DK_JOB_CONVERT:
                                    jobNode->state = DK_AI_IMP_CONVERT;
                                    break;
                            }
                            AIJob_DigConvert* info = (AIJob_DigConvert*) jobNode->info;
                            info->job = job;
                        }
                    }
                }

                if (best_job) {
                    // We found work, reserve it for ourselves and active the
                    // two jobs (work + move).
                    best_job->worker = unit;
                    unit->tx = move->path[0].x * DK_BLOCK_SIZE;
                    unit->ty = move->path[0].y * DK_BLOCK_SIZE;
                    move->path_index = 1;
                    unit->ai_count += 2;
                } else if (idle->wander_delay > 0) {
                    --idle->wander_delay;
                } else {
                    // Just walk around dumbly.
                    int i;
                    for (i = 0; i < DK_AI_WANDER_TRIES; ++i) {
                        float wx = unit->x + (rand() / (float) RAND_MAX - 0.5f) * DK_BLOCK_SIZE;
                        float wy = unit->y + (rand() / (float) RAND_MAX - 0.5f) * DK_BLOCK_SIZE;
                        // Make sure the coordinates are in bounds. This is
                        // expecially important for the interval of [-1, 0],
                        // because those get rounded to 0 when casting to int,
                        // resulting in a seemingly valid block.
                        if (wx < 0.2f) {
                            wx = 0.2f;
                        }
                        if (wx > DK_map_size * DK_BLOCK_SIZE - 0.2f) {
                            wx = DK_map_size * DK_BLOCK_SIZE - 0.2f;
                        }
                        if (wy < 0.2f) {
                            wy = 0.2f;
                        }
                        if (wy > DK_map_size * DK_BLOCK_SIZE - 0.2f) {
                            wy = DK_map_size * DK_BLOCK_SIZE - 0.2f;
                        }
                        if (DK_block_is_passable(DK_block_at((int) (wx / DK_BLOCK_SIZE), (int) (wy / DK_BLOCK_SIZE)))) {
                            // TODO: avoid getting too close to walls
                            unit->tx = wx;
                            unit->ty = wy;
                            break;
                        }
                    }

                    // Wait a bit before trying again.
                    idle->wander_delay = DK_AI_WANDER_DELAY;
                }
            }
            break;
        }
        case DK_AI_MOVE:
        {
            // AI is moving, check if we're there yet.
            if (fabs(unit->tx - unit->x) < 0.1f &&
                    fabs(unit->ty - unit->y) < 0.1f) {
                // Reached local checkpoint, see if we're at our final goal.
                AIJob_Move* info = (AIJob_Move*) ai->info;
                if (info->path_index == info->path_depth) {
                    // Yes, we reached our goal, pop the state.
                    --unit->ai_count;
                } else {
                    // Not yet, set the next waypoint.
                    unit->tx = info->path[info->path_index].x * DK_BLOCK_SIZE;
                    unit->ty = info->path[info->path_index].y * DK_BLOCK_SIZE;
                    ++info->path_index;
                }
            }
            break;
        }
        case DK_AI_IMP_DIG:
        {
            // Stop if the job got voided.
            AIJob_DigConvert* info = (AIJob_DigConvert*) ai->info;
            if (!info->job) {
                --unit->ai_count;
                break;
            }

            // Skip if on cooldown.
            if (unit->cooldowns[DK_ABILITY_IMP_ATTACK] > 0) {
                --unit->cooldowns[DK_ABILITY_IMP_ATTACK];
                break;
            }

            // Else attack the dirt! Update cooldown and apply damage.
            unit->cooldowns[DK_ABILITY_IMP_ATTACK] =
                    cooldowns[DK_UNIT_IMP][DK_ABILITY_IMP_ATTACK];
            if (DK_block_damage(info->job->block, damage[DK_UNIT_IMP][DK_ABILITY_IMP_ATTACK])) {
                // Block was destroyed! Job done.
                --unit->ai_count;
            }
            break;
        }
        case DK_AI_IMP_CONVERT:
        {
            // Stop if the job got voided.
            AIJob_DigConvert* info = (AIJob_DigConvert*) ai->info;
            if (!info->job) {
                --unit->ai_count;
                break;
            }

            // Skip if on cooldown.
            if (unit->cooldowns[DK_ABILITY_IMP_CONVERT] > 0) {
                --unit->cooldowns[DK_ABILITY_IMP_CONVERT];
                break;
            }

            // Else hoyoyo! Update cooldown and apply conversion.
            unit->cooldowns[DK_ABILITY_IMP_CONVERT] =
                    cooldowns[DK_UNIT_IMP][DK_ABILITY_IMP_CONVERT];
            if (DK_block_convert(info->job->block, damage[DK_UNIT_IMP][DK_ABILITY_IMP_CONVERT], unit->owner)) {
                // Block was destroyed! Job done.
                --unit->ai_count;
            }
            break;
        }
        default:
            // Unknown / invalid state, just pop it.
            --unit->ai_count;
            break;
    }
}

void DK_init_units() {
    int i;
    for (i = 0; i < DK_PLAYER_COUNT; ++i) {
        unit_count[i] = 0;
    }
    total_unit_count = 0;
}

void DK_update_units() {
    int i;
    for (i = 0; i < total_unit_count; ++i) {
        DK_Unit* unit = &units[i];

        update_position(unit);
        update_ai(unit);
    }
}

void DK_render_units() {
    if (!quadratic) {
        quadratic = gluNewQuadric();
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    int i;
    for (i = 0; i < total_unit_count; ++i) {
        const DK_Unit* unit = &units[i];
        switch (unit->type) {
            default:
                // Push name of the unit.
                glLoadName(i);
                glDisable(GL_LIGHTING);
                switch (unit->ai[unit->ai_count - 1].state) {
                    case DK_AI_MOVE:
                        glColor3f(0.9f, 0.9f, 0.9f);
                        break;
                    case DK_AI_IMP_DIG:
                        glColor3f(0.6f, 0.9f, 0.6f);
                        break;
                    case DK_AI_IMP_CONVERT:
                        glColor3f(0.6f, 0.6f, 0.9f);
                        break;
                    default:
                        glColor3f(0.6f, 0.6f, 0.6f);
                        break;
                }
                glPushMatrix();
                glTranslatef(unit->x, unit->y, 4);
                gluSphere(quadratic, DK_BLOCK_SIZE / 6.0f, 8, 8);
                glPopMatrix();
                glEnable(GL_LIGHTING);
                break;
        }

#if DK_D_DRAW_PATHS
        if (unit->ai[unit->ai_count - 1].state == DK_AI_MOVE) {
            glColor3f(0.8f, 0.8f, 0.9f);
            glLineWidth(1.52f);
            glDisable(GL_LIGHTING);
            glBegin(GL_LINES);

            glVertex3f(unit->x, unit->y, DK_D_DRAW_PATH_HEIGHT);
            glVertex3f(unit->tx, unit->ty, DK_D_DRAW_PATH_HEIGHT);

            AIJob_Move* info = (AIJob_Move*) unit->ai[unit->ai_count - 1].info;
            int j;
            for (j = info->path_index - 1; j < info->path_depth - 1; ++j) {
                glVertex3f(info->path[j].x * DK_BLOCK_SIZE, info->path[j].y * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT);
                glVertex3f(info->path[j + 1].x * DK_BLOCK_SIZE, info->path[j + 1].y * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT);
            }

            glEnd();
            glEnable(GL_LIGHTING);
        }
#endif
    }
}

unsigned int DK_add_unit(DK_Player player, DK_UnitType type, unsigned short x, unsigned short y) {
    if (total_unit_count > DK_PLAYER_COUNT * DK_UNITS_MAX_PER_PLAYER) {
        // TODO
        return 0;
    }

    // Check if the block is valid.
    if (!DK_block_is_passable(DK_block_at(x, y))) {
        // TODO
        return 0;
    }

    DK_Unit* unit = &units[total_unit_count];
    unit->type = type;
    unit->owner = player;
    unit->x = (x + 0.5f) * DK_BLOCK_SIZE;
    unit->y = (y + 0.5f) * DK_BLOCK_SIZE;
    unit->tx = unit->x;
    unit->ty = unit->y;
    unit->ms = move_speeds[type];

    ++total_unit_count;
    ++unit_count[player];
    
    return 1;
}

void DK_unit_cancel_job(DK_Unit* unit) {
    int backtrack = 1;
    while (unit->ai_count > backtrack) {
        AI_Node* ai = &unit->ai[unit->ai_count - backtrack];
        if (ai->state == DK_AI_IMP_DIG || ai->state == DK_AI_IMP_CONVERT) {
            unit->ai_count -= backtrack - 1;
            // Found the job we want to cancel.
            AIJob_DigConvert* info = (AIJob_DigConvert*) ai->info;
            if (info->job) {
                // It wasn't canceled yet.
                info->job->worker = 0;
                info->job = 0;
                // Do *not* stop! Looks more natural and will avoid units
                // jittering in the middle of nowhere because all their jobs
                // get stolen :P
                //unit->tx = unit->x;
                //unit->ty = unit->y;
            }
            return;
        }
        ++backtrack;
    }
}
