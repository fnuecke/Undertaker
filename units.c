#include <float.h>
#include <math.h>
#include <memory.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "astar.h"
#include "bitset.h"
#include "config.h"
#include "units.h"
#include "map.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////

/** Must be in same order as unit definitions in unit type enum */
static float unit_speeds[] = {
    0.2f,
    0.1f
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
    DK_AI_IMP_CONQUER,

    /** AI is working on something (training, research, crafting) */
    DK_AI_WORK
} AI_Job;

typedef struct {
    unsigned int delay;
} AIJob_Idle;

typedef struct {
    /** The path the unit currently follows (if moving) */
    AStar_Waypoint path[DK_AI_PATH_MAX];

    /** The remaining length of the path */
    unsigned int node_count;
} AIJob_Move;

typedef struct {
    /** Coordinate of the block an imp should dig up */
    unsigned int x, y;
} AIJob_Dig;

typedef struct {
    /** Coordinate of the block an imp should convert */
    unsigned int x, y;
} AIJob_Convert;

typedef struct {
    /** Enemy the unit currently attacks */
    unsigned int enemy;

} AIJob_Attack;

/** Get size of largest info struct */
#define SIZE0 sizeof(AIJob_Idle)
#define SIZE1 sizeof(AIJob_Move)
#define SIZE2 sizeof(AIJob_Dig)
#define SIZE3 sizeof(AIJob_Convert)
#define SIZE4 sizeof(AIJob_Attack)
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MAX_JOB_SIZE MAX(MAX(MAX(MAX(SIZE0, SIZE1), SIZE2), SIZE3), SIZE4)

typedef struct {
    /** The type of this AI entry */
    AI_Job state;

    /** Statically allocated data for this AI entry */
    char info[MAX_JOB_SIZE];
} AI_Node;

typedef struct {
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

    /** The unit's attack speed (the cooldown between attacks) */
    float as;

    /** Level of the unit, as a float to account for "experience" */
    float level;

    /** Depth of the AI state stack */
    unsigned int ai_count;

    /** AI info stack */
    AI_Node ai[DK_AI_JOB_STACK_MAX];
} DK_Unit;

/** A slot for imp work (digging, converting) */
typedef struct {
    /** Coordinates of the workplace in A* coordinates */
    unsigned int x, y;

    /** The AI job info needed to process this task */
    AI_Node job;

    /** The imp that wants to work here */
    DK_Unit* worker;
} AI_Work;

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

/**
 * Possible work slots on a map, per player; possibility for two players to
 * have imps working on the same block at the same time, but whatever.
 */
static AI_Work* workplaces[DK_PLAYER_COUNT] = {0};

/** Capacity of the workplace lists, and actual load */
static unsigned int workplace_capacity[DK_PLAYER_COUNT] = {64};
static unsigned int workplace_count[DK_PLAYER_COUNT] = {0};

///////////////////////////////////////////////////////////////////////////////
// Internal update methods
///////////////////////////////////////////////////////////////////////////////

static void update_position(DK_Unit* unit) {
    // Get unit vector to target.
    const float dx = unit->tx - unit->x;
    const float dy = unit->ty - unit->y;
    if (fabs(dx) > 0.001f || fabs(dy) > 0.001f) {
        const float l0 = dx * dx + dy * dy;

        if (l0 <= unit->ms) {
            // This step will get us to our target position.
            unit->x = unit->tx;
            unit->y = unit->ty;
        } else {
            // Still have some way to go, normalize deltas and apply movement speed
            // to update the position.
            const float factor = unit->ms / l0;
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
        ((AIJob_Idle*) unit->ai[0].info)->delay = DK_AI_IDLE_DELAY;
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

            // Find the unit something to do.
            if (unit->type == DK_UNIT_IMP) {
                // Look for the closest workplace.
                float best_length = FLT_MAX;
                AI_Work* best_work = NULL;

                // Assuming we'll find work we'll want to add two jobs, one for
                // the actual work task, one for getting there.
                AI_Node* task = &unit->ai[unit->ai_count];
                AIJob_Move* move = (AIJob_Move*) unit->ai[unit->ai_count + 1].info;

                int i;
                for (i = 0; i < workplace_count[unit->owner]; ++i) {
                    // Current workplace we're checking.
                    AI_Work* work = &workplaces[unit->owner][i];

                    // Skip jobs that are being worked on for now.
                    // TODO: override if we're closer.
                    if (work->worker) {
                        continue;
                    }

                    // Find a path to it. Use temporary output data to avoid
                    // overriding existing path that may be shorter.
                    AStar_Waypoint path[sizeof (move->path)];
                    unsigned int depth;
                    float length;
                    if (DK_a_star(unit->x / DK_BLOCK_SIZE, unit->y / DK_BLOCK_SIZE, work->x, work->y, path, &depth, &length)) {
                        // Got a path, check its length.
                        if (length < best_length) {
                            // Got a new best. Copy data.
                            best_length = length;
                            best_work = work;
                            memcpy(move->path, path, depth * sizeof (AStar_Waypoint));
                            move->node_count = depth;
                            memcpy(task, &work->job, sizeof (AI_Node));
                        }
                    }
                }

                if (best_work) {
                    // We found work, reserve it for ourselves and active the
                    // two jobs (work + move).
                    best_work->worker = unit;
                    unit->ai_count += 2;
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
                if (--info->node_count < 1) {
                    // Yes, we reached our goal, pop the state.
                    --unit->ai_count;
                } else {
                    // Not yet, set the next waypoint.
                    unit->tx = info->path[info->node_count - 1].x;
                    unit->ty = info->path[info->node_count - 1].y;
                }
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
    for (i = 0; i < DK_PLAYER_COUNT - 1; ++i) {
        workplace_count[i] = 0;
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
                glColor3f(0.8f, 0.8f, 1.0f);
                glPushMatrix();
                glTranslatef(unit->x, unit->y, 0);
                gluSphere(quadratic, DK_BLOCK_SIZE / 6.0f, 8, 8);
                glPopMatrix();
                break;
        }

#if DK_D_DRAW_PATHS
        float tx = 11.5f;
        float ty = 9.99f;
        AStar_Waypoint path[DK_AI_PATH_MAX];
        unsigned int depth = sizeof (path);
        float length;

        if (DK_a_star(unit->x / DK_BLOCK_SIZE, unit->y / DK_BLOCK_SIZE, tx, ty, path, &depth, &length)) {
            glColor3f(1.0f, 1.0f, 1.0f);
            glLineWidth(3);
            glDisable(GL_LIGHTING);
            glBegin(GL_LINES);

            glVertex3f(unit->x, unit->y, DK_D_PATH_HEIGHT);
            glVertex3f(path[0].x * DK_BLOCK_SIZE, path[0].y * DK_BLOCK_SIZE, DK_D_PATH_HEIGHT);

            int j;
            for (j = 0; j < depth - 1; ++j) {
                glVertex3f(path[j].x * DK_BLOCK_SIZE, path[j].y * DK_BLOCK_SIZE, DK_D_PATH_HEIGHT);
                glVertex3f(path[j + 1].x * DK_BLOCK_SIZE, path[j + 1].y * DK_BLOCK_SIZE, DK_D_PATH_HEIGHT);
            }

            glVertex3f(path[j].x * DK_BLOCK_SIZE, path[j].y * DK_BLOCK_SIZE, DK_D_PATH_HEIGHT);
            glVertex3f(tx * DK_BLOCK_SIZE, ty * DK_BLOCK_SIZE, DK_D_PATH_HEIGHT);

            glEnd();
            glEnable(GL_LIGHTING);
        }
#endif
    }
}

unsigned int DK_add_unit(DK_Player player, DK_UnitType type, unsigned short x, unsigned short y) {
    if (total_unit_count > DK_PLAYER_COUNT * DK_UNITS_MAX_PER_PLAYER) {
        // TODO
        return;
    }

    // Check if the block is valid.
    if (!DK_block_is_passable(DK_block_at(x, y))) {
        // TODO
        return;
    }

    DK_Unit* unit = &units[total_unit_count];
    unit->type = type;
    unit->x = (x + 0.5f) * DK_BLOCK_SIZE;
    unit->y = (y + 0.5f) * DK_BLOCK_SIZE;
    unit->tx = unit->x;
    unit->ty = unit->y;
    unit->ms = unit_speeds[type];

    ++total_unit_count;
    ++unit_count[player];
}