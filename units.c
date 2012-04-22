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
    AStar_Waypoint path[DK_AI_PATH_MAX];

    /** The total depth of the path (number of nodes) */
    unsigned int path_depth;

    /** The current node of the path */
    unsigned int path_index;

    /** Enemy the unit currently attacks */
    unsigned int enemy;
} AI_Node;

struct DK_Unit {
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
                DK_Job** jobs = DK_jobs(unit->owner, &job_count);

                // Assuming we'll find work we'll want to add two jobs, one for
                // the actual work task, one for getting there.
                AI_Node* jobNode = &unit->ai[unit->ai_count];
                AI_Node* moveNode = &unit->ai[unit->ai_count + 1];
                moveNode->state = DK_AI_MOVE;

                for (; job_count > 0; --job_count) {
                    // Current workplace we're checking.
                    DK_Job* job = jobs[job_count - 1];

                    // For path finding results.
                    AStar_Waypoint path[DK_AI_PATH_MAX];
                    unsigned int depth = DK_AI_PATH_MAX;

                    // Test direct distance; if that's longer than our best we
                    // can safely skip this one.
                    const float dx = unit->x - job->x;
                    const float dy = unit->y - job->y;
                    float distance = sqrt(dx * dx + dy * dy);
                    if (distance >= best_distance + best_penalty) {
                        continue;
                    }

                    // Find a path to it. Use temporary output data to avoid
                    // overriding existing path that may be shorter.
                    if (DK_a_star(unit, job->x, job->y, path, &depth, &distance)) {
                        float penalty = 0;

                        // Factor in priorities.
                        switch (job->type) {
                            case DK_JOB_DIG:
                                penalty = DK_JOB_DIG_PRIORITY;
                                break;
                            case DK_JOB_CONVERT:
                                if (DK_block_is_open(job->block)) {
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
                                const float cdx = job->worker->x - job->x;
                                const float cdy = job->worker->y - job->y;
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
                            memcpy(moveNode->path, path, depth * sizeof (AStar_Waypoint));
                            moveNode->path_depth = depth;
                            switch (job->type) {
                                case DK_JOB_DIG:
                                    jobNode->state = DK_AI_IMP_DIG;
                                    break;
                                case DK_JOB_CONVERT:
                                    jobNode->state = DK_AI_IMP_CONVERT;
                                    break;
                            }
                            jobNode->job = job;
                        }
                    }
                }

                if (best_job) {
                    // We found work, reserve it for ourselves and active the
                    // two jobs (work + move).
                    best_job->worker = unit;
                    unit->tx = moveNode->path[0].x;
                    unit->ty = moveNode->path[0].y;
                    moveNode->path_index = 1;
                    unit->ai_count += 2;
                } else {
                    if (ai->wander_delay > 0) {
                        --ai->wander_delay;
                    } else {
                        // Just walk around dumbly.
                        int i;
                        for (i = 0; i < DK_AI_WANDER_TRIES; ++i) {
                            float wx = unit->x + (rand() / (float) RAND_MAX - 0.5f);
                            float wy = unit->y + (rand() / (float) RAND_MAX - 0.5f);
                            // Make sure the coordinates are in bounds. This is
                            // expecially important for the interval of [-1, 0],
                            // because those get rounded to 0 when casting to int,
                            // resulting in a seemingly valid block.
                            if (wx < 0.2f) {
                                wx = 0.2f;
                            }
                            if (wx > DK_map_size - 0.2f) {
                                wx = DK_map_size - 0.2f;
                            }
                            if (wy < 0.2f) {
                                wy = 0.2f;
                            }
                            if (wy > DK_map_size - 0.2f) {
                                wy = DK_map_size - 0.2f;
                            }
                            if (DK_block_is_passable(DK_block_at((int) wx, (int) wy), unit)) {
                                // TODO: avoid getting too close to walls
                                unit->tx = wx;
                                unit->ty = wy;
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
            // AI is moving, check if we're there yet.
            if (fabs(unit->tx - unit->x) < 0.01f &&
                    fabs(unit->ty - unit->y) < 0.01f) {
                // Reached local checkpoint, see if we're at our final goal.
                if (ai->path_index == ai->path_depth) {
                    // Yes, we reached our goal, pop the state.
                    --unit->ai_count;
                } else {
                    // Not yet, set the next waypoint.
                    unit->tx = ai->path[ai->path_index].x;
                    unit->ty = ai->path[ai->path_index].y;
                    ++ai->path_index;
                }
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
            if (DK_block_damage(ai->job->block, damage[DK_UNIT_IMP][DK_ABILITY_IMP_ATTACK])) {
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
            if (DK_block_convert(ai->job->block, damage[DK_UNIT_IMP][DK_ABILITY_IMP_CONVERT], unit->owner)) {
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

void DK_init_units(void) {
    int i;
    for (i = 0; i < DK_PLAYER_COUNT; ++i) {
        unit_count[i] = 0;
    }
    total_unit_count = 0;
}

void DK_update_units(void) {
    unsigned int i;
    for (i = 0; i < total_unit_count; ++i) {
        DK_Unit* unit = &units[i];

        update_position(unit);
        update_ai(unit);
    }
}

void DK_render_units(void) {
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
                glTranslatef(unit->x * DK_BLOCK_SIZE, unit->y * DK_BLOCK_SIZE, 4);
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
                    glVertex3f(unit->x * DK_BLOCK_SIZE, unit->y * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT);
                    glVertex3f(unit->tx * DK_BLOCK_SIZE, unit->ty * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT);

                    for (j = ai->path_index - 1; j < ai->path_depth - 1; ++j) {
                        glVertex3f(ai->path[j].x * DK_BLOCK_SIZE, ai->path[j].y * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT);
                        glVertex3f(ai->path[j + 1].x * DK_BLOCK_SIZE, ai->path[j + 1].y * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT);
                    }
                }
                glEnd();
                glPopAttrib();
            }
        }
    }
}

unsigned int DK_add_unit(DK_Player player, DK_UnitType type, unsigned short x, unsigned short y) {
    if (total_unit_count > DK_PLAYER_COUNT * DK_UNITS_MAX_PER_PLAYER) {
        // TODO
        return 0;
    } else if (!DK_block_is_open(DK_block_at(x, y))) {
        // TODO
        return 0;
    } else {
        DK_Unit* unit = &units[total_unit_count];
        unit->type = type;
        unit->owner = player;
        unit->x = (x + 0.5f);
        unit->y = (y + 0.5f);
        unit->tx = unit->x;
        unit->ty = unit->y;
        unit->ms = move_speeds[type];

        ++total_unit_count;
        ++unit_count[player];
    }
    return 1;
}

void DK_unit_cancel_job(DK_Unit* unit) {
    unsigned int backtrack = 1;
    while (unit->ai_count > backtrack) {
        AI_Node* ai = &unit->ai[unit->ai_count - backtrack];
        if (ai->state == DK_AI_IMP_DIG || ai->state == DK_AI_IMP_CONVERT) {
            // Found the job we want to cancel.
            if (ai->job) {
                // It wasn't canceled yet.
                ai->job->worker = 0;
                ai->job = 0;
                // Do *not* stop! Looks more natural and will avoid units
                // jittering in the middle of nowhere because all their jobs
                // get stolen :P
                //unit->tx = unit->x;
                //unit->ty = unit->y;
            }
            // Remember how much we had to pop.
            unit->ai_count -= backtrack - 1;
            return;
        }
        ++backtrack;
    }
}

int DK_unit_position(const DK_Unit* unit, float* x, float* y) {
    if (unit) {
        *x = unit->x;
        *y = unit->y;
        return 1;
    }
    return 0;
}

int DK_unit_immune_to_lava(const DK_Unit* unit) {
    return unit && unit->immune_to_lava;
}

DK_Player DK_unit_owner(const DK_Unit* unit) {
    return unit ? unit->owner : DK_PLAYER_NONE;
}