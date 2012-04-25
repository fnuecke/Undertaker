#include <float.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>

#include <GL/glew.h>

#include "astar.h"
#include "bitset.h"
#include "config.h"
#include "map.h"
#include "jobs.h"
#include "vmath.h"

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
    [DK_UNIT_IMP] = 0.03f,
    [DK_UNIT_WIZARD] = 0.02f
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
    /** The type of this AI entry */
    AI_Job state;

    /** Delay for idling (before next check if there's something to do) */
    unsigned int delay;

    /** Delay (in number of idle updates) before wandering around again */
    unsigned int wander_delay;

    /** The job the unit currently performs */
    DK_Job* job;

    /** The path the unit currently follows (if moving) */
    DK_AStarWaypoint path[DK_AI_PATH_MAX + 2];

    /** The total depth of the path (number of nodes) */
    unsigned int path_depth;

    /** The current node of the path */
    unsigned int path_index;

    /** Distance to next node in the path */
    float path_distance;

    /** Distance already traveled to the next node */
    float path_traveled;

    /** Enemy the unit currently attacks */
    unsigned int enemy;
} AI_Node;

struct DK_Unit {
    /** The player this unit belongs to */
    DK_Player owner;

    /** The type of the unit */
    DK_UnitType type;

    /** Current position of the unit */
    vec2 position;

    /** Cooldowns for unit abilities */
    unsigned int cooldowns[DK_UNITS_MAX_ABILITIES];

    /** Level of the unit, as a float to account for "experience" */
    float level;

    /** Flag whether this unit is immune to lava */
    int immune_to_lava;

    /** Depth of the AI state stack */
    unsigned int ai_count;

    /** AI info stack */
    AI_Node ai[DK_AI_JOB_STACK_MAX];
};

///////////////////////////////////////////////////////////////////////////////
// Global variables
///////////////////////////////////////////////////////////////////////////////

/** Used for debug rendering of units and paths */
static GLUquadric* quadratic = 0;

/** Units in the game, must be ensured to be non-sparse */
static DK_Unit units[DK_PLAYER_COUNT * DK_UNITS_MAX_PER_PLAYER];

/** Total number of units */
static unsigned int total_unit_count = 0;

/** Number of units per player */
static unsigned int unit_count[DK_PLAYER_COUNT] = {0};

///////////////////////////////////////////////////////////////////////////////
// Internal update methods
///////////////////////////////////////////////////////////////////////////////

static float catmull_rom(float p0, float p1, float p2, float p3, float t) {
    const float m1 = DK_AI_CATMULL_ROM_T * (p2 - p0);
    const float m2 = DK_AI_CATMULL_ROM_T * (p3 - p1);
    return (2 * p1 - 2 * p2 + m1 + m2) * t * t * t + (-3 * p1 + 3 * p2 - 2 * m1 - m2) * t * t + m1 * t + p1;
}

static void update_ai(DK_Unit* unit) {
    AI_Node* ai;
    if (unit->ai_count == 0) {
        // Switch to idle state if there is none set.
        ai = &unit->ai[0];
        ai->state = DK_AI_IDLE;
        ai->delay = DK_AI_IDLE_DELAY / 2 + (rand() * DK_AI_IDLE_DELAY / 2 / RAND_MAX);
        ai->wander_delay = DK_AI_WANDER_DELAY;

        unit->ai_count = 1;
    }
    ai = &unit->ai[unit->ai_count - 1];
    switch (ai->state) {
        case DK_AI_IDLE:
        {
            if (ai->delay > 0) {
                // Still waiting, skip for now.
                --ai->delay;
                break;
            }

            // Find the unit something to do.
            if (unit->type == DK_UNIT_IMP) {
                // Look for the closest workplace.
                float best_distance = FLT_MAX;
                float best_penalty = 0;
                DK_Job* best_job = NULL;
                unsigned int job_count;
                DK_Job** jobs = DK_GetJobs(unit->owner, &job_count);

                // Assuming we'll find work we'll want to add two jobs, one for
                // the actual work task, one for getting there.
                AI_Node* jobNode = &unit->ai[unit->ai_count];
                AI_Node* moveNode = &unit->ai[unit->ai_count + 1];
                moveNode->state = DK_AI_MOVE;

                for (; job_count > 0; --job_count) {
                    // Current workplace we're checking.
                    DK_Job* job = jobs[job_count - 1];

                    // For path finding results.
                    DK_AStarWaypoint path[DK_AI_PATH_MAX];
                    unsigned int depth = DK_AI_PATH_MAX;

                    // Test direct distance; if that's longer than our best we
                    // can safely skip this one.
                    const float dx = unit->position.v[0] - job->position.v[0];
                    const float dy = unit->position.v[1] - job->position.v[1];
                    float distance = sqrt(dx * dx + dy * dy);
                    if (distance >= best_distance + best_penalty) {
                        continue;
                    }

                    // Find a path to it. Use temporary output data to avoid
                    // overriding existing path that may be shorter.
                    if (DK_AStar(unit, &job->position, path, &depth, &distance)) {
                        float penalty = 0;

                        // Factor in priorities.
                        switch (job->type) {
                            case DK_JOB_DIG:
                                penalty = DK_JOB_DIG_PRIORITY;
                                break;
                            case DK_JOB_CONVERT:
                                if (DK_IsBlockOpen(job->block)) {
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
                                const float cdx = job->worker->position.v[0] - job->position.v[0];
                                const float cdy = job->worker->position.v[1] - job->position.v[1];
                                const float current_distance = sqrt(cdx * cdx + cdy * cdy);
                                if (current_distance <= distance + DK_AI_ALREADY_WORKING_BONUS) {
                                    // The one that's on it is closer, ignore.
                                    continue;
                                }
                                // We're closer, kick the other one.
                                DK_CancelJob(job->worker);
                            }

                            // Copy data.
                            best_distance = distance;
                            best_penalty = penalty;
                            best_job = job;
                            memcpy(&moveNode->path[1], path, depth * sizeof (DK_AStarWaypoint));
                            moveNode->path_depth = depth;
                            // Generate endpoints for catmull-rom spline; just
                            // extend the path in the direction of the last two
                            // nodes before that end.
                            {
                                const float dlx = moveNode->path[1].x - moveNode->path[2].x;
                                const float dly = moveNode->path[1].y - moveNode->path[2].y;
                                const float l = sqrtf(dlx * dlx + dly * dly);
                                moveNode->path[0].x = moveNode->path[1].x + dlx / l;
                                moveNode->path[0].y = moveNode->path[1].y + dly / l;
                            }
                            {
                                const float dlx = moveNode->path[depth].x - moveNode->path[depth - 1].x;
                                const float dly = moveNode->path[depth].y - moveNode->path[depth - 1].y;
                                const float l = sqrtf(dlx * dlx + dly * dly);
                                moveNode->path[depth + 1].x = moveNode->path[depth].x + dlx / l;
                                moveNode->path[depth + 1].y = moveNode->path[depth].y + dly / l;
                            }

                            // Save the job type.
                            switch (job->type) {
                                case DK_JOB_DIG:
                                    jobNode->state = DK_AI_IMP_DIG;
                                    break;
                                case DK_JOB_CONVERT:
                                    jobNode->state = DK_AI_IMP_CONVERT;
                                    break;
                            }

                            // And the job.
                            jobNode->job = job;
                        }
                    }
                }

                if (best_job) {
                    // We found work, reserve it for ourselves and active the
                    // two jobs (work + move).
                    best_job->worker = unit;

                    // Trigger computation of the next segment by setting the
                    // distance to zero.
                    moveNode->path_index = 1;
                    moveNode->path_distance = 0;
                    moveNode->path_traveled = 0;

                    // Remember we extended the stack.
                    unit->ai_count += 2;
                } else {
                    if (ai->wander_delay > 0) {
                        --ai->wander_delay;
                    } else {
                        // Just walk around dumbly.
                        int i;
                        for (i = 0; i < DK_AI_WANDER_TRIES; ++i) {
                            float wx = unit->position.v[0] + (rand() / (float) RAND_MAX - 0.5f);
                            float wy = unit->position.v[1] + (rand() / (float) RAND_MAX - 0.5f);
                            // Make sure the coordinates are in bounds. This is
                            // expecially important for the interval of [-1, 0],
                            // because those get rounded to 0 when casting to int,
                            // resulting in a seemingly valid block.
                            if (wx < 0.2f) {
                                wx = 0.2f;
                            }
                            if (wx > DK_GetMapSize() - 0.2f) {
                                wx = DK_GetMapSize() - 0.2f;
                            }
                            if (wy < 0.2f) {
                                wy = 0.2f;
                            }
                            if (wy > DK_GetMapSize() - 0.2f) {
                                wy = DK_GetMapSize() - 0.2f;
                            }
                            if (DK_IsBlockPassable(DK_GetBlockAt((int) wx, (int) wy), unit)) {
                                // OK, move.
                                moveNode = &unit->ai[unit->ai_count++];
                                moveNode->state = DK_AI_MOVE;
                                moveNode->path[0].x = 2 * unit->position.v[0] - wx;
                                moveNode->path[0].y = 2 * unit->position.v[1] - wy;
                                moveNode->path[1].x = unit->position.v[0];
                                moveNode->path[1].y = unit->position.v[1];
                                moveNode->path[2].x = wx;
                                moveNode->path[2].y = wy;
                                moveNode->path[3].x = 2 * wx - unit->position.v[0];
                                moveNode->path[3].y = 2 * wy - unit->position.v[1];
                                moveNode->path_depth = 2;
                                moveNode->path_index = 1;
                                moveNode->path_distance = 0;
                                moveNode->path_traveled = 0;
                                break;
                            }
                        }

                        // Wait a bit before trying again.
                        ai->wander_delay = DK_AI_WANDER_DELAY;
                    }

                    // Update delay. Don't update it if we found a job, to allow
                    // looking for a job again, directly after finishing the last.
                    ai->delay = DK_AI_IDLE_DELAY / 2 + (rand() * DK_AI_IDLE_DELAY / 2 / RAND_MAX);
                }
            }
            break;
        }
        case DK_AI_MOVE:
        {
            // AI is moving. Apply movement.
            ai->path_traveled += move_speeds[unit->type];

            // Test if we reached the waypoint.
            if (ai->path_traveled > ai->path_distance) {
                // Yes, try to advance to the next one.
                ++ai->path_index;
                if (ai->path_index > ai->path_depth) {
                    // Reached final node, we're done.
                    unit->position.v[0] = ai->path[ai->path_index - 1].x;
                    unit->position.v[1] = ai->path[ai->path_index - 1].y;
                    --unit->ai_count;
                    break;
                } else {
                    // Subtract length of previous to carry surplus movement.
                    ai->path_traveled -= ai->path_distance;

                    // Update to new distance.
                    if (DK_AI_PATH_INTERPOLATE) {
                        int e;
                        float x, y, dx, dy;
                        float lx = ai->path[ai->path_index - 1].x;
                        float ly = ai->path[ai->path_index - 1].y;
                        ai->path_distance = 0;
                        for (e = 1; e <= DK_AI_PATH_INTERPOLATION; ++e) {
                            const float t = e / (float) DK_AI_PATH_INTERPOLATION;
                            x = catmull_rom(ai->path[ai->path_index - 2].x, ai->path[ai->path_index - 1].x, ai->path[ai->path_index].x, ai->path[ai->path_index + 1].x, t);
                            y = catmull_rom(ai->path[ai->path_index - 2].y, ai->path[ai->path_index - 1].y, ai->path[ai->path_index].y, ai->path[ai->path_index + 1].y, t);
                            dx = x - lx;
                            dy = y - ly;
                            lx = x;
                            ly = y;
                            ai->path_distance += sqrtf(dx * dx + dy * dy);
                        }
                    } else {
                        const float dx = ai->path[ai->path_index].x - ai->path[ai->path_index - 1].x;
                        const float dy = ai->path[ai->path_index].y - ai->path[ai->path_index - 1].y;
                        ai->path_distance = sqrtf(dx * dx + dy * dy);
                    }
                }
            }
            // Compute actual position of the unit.
            {
                const float t = ai->path_traveled / ai->path_distance;
                unit->position.v[0] = catmull_rom(ai->path[ai->path_index - 2].x, ai->path[ai->path_index - 1].x, ai->path[ai->path_index].x, ai->path[ai->path_index + 1].x, t);
                unit->position.v[1] = catmull_rom(ai->path[ai->path_index - 2].y, ai->path[ai->path_index - 1].y, ai->path[ai->path_index].y, ai->path[ai->path_index + 1].y, t);
            }
            break;
        }
        case DK_AI_IMP_DIG:
        {
            // Stop if the job got voided.
            if (!ai->job) {
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
            if (DK_DamageBlock(ai->job->block, damage[DK_UNIT_IMP][DK_ABILITY_IMP_ATTACK])) {
                // Block was destroyed! Job done.
                --unit->ai_count;
            }
            break;
        }
        case DK_AI_IMP_CONVERT:
        {
            // Stop if the job got voided.
            if (!ai->job) {
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
            if (DK_ConvertBlock(ai->job->block, damage[DK_UNIT_IMP][DK_ABILITY_IMP_CONVERT], unit->owner)) {
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

void DK_InitUnits(void) {
    int i;
    for (i = 0; i < DK_PLAYER_COUNT; ++i) {
        unit_count[i] = 0;
    }
    total_unit_count = 0;
}

void DK_UpdateUnits(void) {
    unsigned int i;
    for (i = 0; i < total_unit_count; ++i) {
        DK_Unit* unit = &units[i];

        update_ai(unit);
    }
}

void DK_RenderUnits(void) {
    unsigned int i, j;

    if (!quadratic) {
        quadratic = gluNewQuadric();
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    for (i = 0; i < total_unit_count; ++i) {
        DK_Unit* unit = &units[i];
        AI_Node* ai = &unit->ai[unit->ai_count - 1];
        switch (unit->type) {
            default:
                // Push name of the unit.
                glLoadName(i);
                glDisable(GL_LIGHTING);
                switch (ai->state) {
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
                glTranslatef(unit->position.v[0] * DK_BLOCK_SIZE, unit->position.v[1] * DK_BLOCK_SIZE, 4);
                gluSphere(quadratic, DK_BLOCK_SIZE / 6.0f, 8, 8);
                glPopMatrix();
                glEnable(GL_LIGHTING);
                break;
        }

        if (DK_d_draw_paths) {
            if (ai->state == DK_AI_MOVE) {
                glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);

                glColor3f(0.8f, 0.8f, 0.9f);
                glLineWidth(1.25f);
                glDisable(GL_LIGHTING);
                glBegin(GL_LINES);
                {
                    glVertex3f(ai->path[1].x * DK_BLOCK_SIZE, ai->path[1].y * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT);
                    for (j = 2; j <= ai->path_depth; ++j) {
                        // Somewhere in the middle, smooth the path.
                        int k;
                        for (k = 1; k < 20; ++k) {
                            const float t = k / 20.0f;
                            const float x = catmull_rom(ai->path[j - 2].x, ai->path[j - 1].x, ai->path[j].x, ai->path[j + 1].x, t);
                            const float y = catmull_rom(ai->path[j - 2].y, ai->path[j - 1].y, ai->path[j].y, ai->path[j + 1].y, t);
                            glVertex3f(x * DK_BLOCK_SIZE, y * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT);
                            glVertex3f(x * DK_BLOCK_SIZE, y * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT);
                        }
                    }
                    glVertex3f(ai->path[j - 1].x * DK_BLOCK_SIZE, ai->path[j - 1].y * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT);
                }
                glEnd();

                glColor3f(0.8f, 0.4f, 0.4f);
                for (j = 1; j <= ai->path_depth; ++j) {
                    glPushMatrix();
                    glTranslatef(ai->path[j].x * DK_BLOCK_SIZE, ai->path[j].y * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT);
                    gluSphere(quadratic, 0.5f, 8, 8);
                    glPopMatrix();
                }
                glPopAttrib();
            }
        }
    }
}

int DK_AddUnit(DK_Player player, DK_UnitType type, unsigned short x, unsigned short y) {
    if (total_unit_count > DK_PLAYER_COUNT * DK_UNITS_MAX_PER_PLAYER) {
        // TODO
        return 0;
    } else if (!DK_IsBlockOpen(DK_GetBlockAt(x, y))) {
        // TODO
        return 0;
    } else {
        DK_Unit* unit = &units[total_unit_count];
        unit->type = type;
        unit->owner = player;
        unit->position.v[0] = (x + 0.5f);
        unit->position.v[1] = (y + 0.5f);

        ++total_unit_count;
        ++unit_count[player];
    }
    return 1;
}

void DK_CancelJob(DK_Unit* unit) {
    unsigned int backtrack = 1;
    while (unit->ai_count > backtrack) {
        AI_Node* ai = &unit->ai[unit->ai_count - backtrack];
        if (ai->state == DK_AI_IMP_DIG || ai->state == DK_AI_IMP_CONVERT) {
            // Found the job we want to cancel.
            if (ai->job) {
                // It wasn't canceled yet.
                ai->job->worker = 0;
                ai->job = 0;
                // TODO don't interrupt other tasks higher on the ai stack
            }
            // Remember how much we had to pop.
            unit->ai_count -= backtrack - 1;
            return;
        }
        ++backtrack;
    }
}

const vec2* DK_GetUnitPosition(const DK_Unit* unit) {
    if (unit) {
        return &unit->position;
    }
    return NULL;
}

int DK_IsUnitImmuneToLava(const DK_Unit* unit) {
    return unit && unit->immune_to_lava;
}

DK_Player DK_GetUnitOwner(const DK_Unit* unit) {
    return unit ? unit->owner : DK_PLAYER_NONE;
}