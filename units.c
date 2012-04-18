#include <malloc.h>
#include <memory.h>
#include <math.h>
#include <float.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <assert.h>

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
// Pathfinding
///////////////////////////////////////////////////////////////////////////////

typedef struct {
    float x, y;
} AStar_Waypoint;

/** Node used in A* search */
typedef struct {
    /** Coordinates of the node (used for searching neighbors) */
    unsigned int x, y;

    /** The cost to travel to this node from the start position */
    float gscore;

    /** The heuristic score of the total path to the goal including this node */
    float fscore;

    /** The path length, i.e. number of parent nodes */
    unsigned int steps;

    /** The node that came before this one in the path it is part of */
    unsigned int came_from;
} AStar_Node;

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

/** Number of cells in the grid used for our A* algorithm */
static unsigned int a_star_grid_size = 0;

/** Bitset for quick lookup of closed set in A* */
static char* a_star_closed_set = 0;

/** Capacity of A* node sets; set to the initial capacities, will grow as necessary */
static unsigned int capacity_open = 32;
static unsigned int capacity_closed = 64;

/** Re-used waypoint sets for A* search */
static AStar_Node* open = 0;
static AStar_Node* closed = 0;

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

///////////////////////////////////////////////////////////////////////////////
// A* search
///////////////////////////////////////////////////////////////////////////////

inline static float a_star_h(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1) {
    const int dx = x1 - x0;
    const int dy = y1 - y0;
    return sqrtf(dx * dx + dy * dy);
    return (x0 == x1 || y0 == y1) ? 1 : M_SQRT2;
}

inline static float a_star_f(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1) {
    const unsigned int dx = abs(x0 - x1);
    const unsigned int dy = abs(y0 - y1);
    return M_SQRT2 * (dx > dy ? dx : dy);
}

inline static float a_star_to_global(unsigned int coordinate) {
    return (coordinate / (float) DK_ASTAR_GRANULARITY + (0.5f / DK_ASTAR_GRANULARITY)) * DK_BLOCK_SIZE;
}

inline static int a_star_is_passable(unsigned int x, unsigned int y) {
    return DK_block_is_passable(DK_block_at(x / DK_ASTAR_GRANULARITY, y / DK_ASTAR_GRANULARITY));
}

static int a_star_jump(int dx, int dy, int* sx, int* sy, int gx, int gy) {
    // Don't go out of bounds.
    if (*sx < 0 || *sx >= a_star_grid_size ||
            *sy < 0 || *sy >= a_star_grid_size) {
        return 0;
    }

    // If we already handled this one, skip it.
    if (BS_test(a_star_closed_set, *sy * a_star_grid_size + *sx)) {
        return 0;
    }

    // Only if this block is passable and the two diagonal ones are.
    if (!a_star_is_passable(*sx, *sy) ||
            (!a_star_is_passable(*sx, *sy - dy) && !a_star_is_passable(*sx - dx, *sy))) {
        return 0;
    }

    // Have we reached the goal?
    if (*sx == gx && *sy == gy) {
        return 1;
    }

    // Do we have to evaluate neighbors here and end our jump?
    if ((dx &&
            ((!a_star_is_passable(*sx, *sy - 1) && a_star_is_passable(*sx + dx, *sy - 1)) ||
            (!a_star_is_passable(*sx, *sy + 1) && a_star_is_passable(*sx + dx, *sy + 1))) ||
            (dy &&
            ((!a_star_is_passable(*sx - 1, *sy) && a_star_is_passable(*sx - 1, *sy + dy)) ||
            (!a_star_is_passable(*sx + 1, *sy) && a_star_is_passable(*sx + 1, *sy + dy)))))) {
        // Yes, there's an obstacle somewhere.
        return 1;
    }

    // Moving diagonally?
    if (dx != 0 && dy != 0) {
        // Yes, then try the straight ones first.
        int tx = *sx + dx, ty = *sy;
        if (a_star_jump(dx, 0, &tx, &ty, gx, gy)) {
            return 1;
        }
        tx = *sx;
        ty = *sy + dy;
        if (a_star_jump(0, dy, &tx, &ty, gx, gy)) {
            return 1;
        }
    }

    // Not invalidated yet, move on ahead.
    *sx += dx;
    *sy += dy;
    return a_star_jump(dx, dy, sx, sy, gx, gy);
}

static int a_star_test_goal(const AStar_Node* n, int gx, int gy, AStar_Waypoint* path, unsigned int* depth, float* length) {
    if (n->x == gx && n->y == gy) {
        // Done, follow the path back to the beginning to return the best
        // neighboring node.

        // Return the length of the returned array.
        *length = n->gscore;

        // Follow the path until only as many nodes as we can fit into the
        // specified array remain.
        while (n->steps > *depth && n->came_from) {
            n = &closed[n->came_from - 1];
        }

        // In case we don't need the full capacity...
        *depth = n->steps;

        // Push the remaining nodes' coordinates (in reverse walk order to
        // make it forward work order).
        int i;
        for (i = *depth - 1; i >= 0; --i) {
            path[i].x = a_star_to_global(n->x);
            path[i].y = a_star_to_global(n->y);

            if (!n->came_from) {
                break;
            } else {
                n = &closed[n->came_from - 1];
            }
        }

        return 1;
    }
    return 0;
}

static int a_star(const DK_Unit* unit, float tx, float ty, AStar_Waypoint* path, unsigned int* depth, float* length) {

    // Check if the target position is valid (passable).
    if (!DK_block_is_passable(DK_block_at(
            (int) (tx / DK_BLOCK_SIZE), (int) (ty / DK_BLOCK_SIZE)))) {
        return 0;
    }

    // Number of entries used in the open and closed set.
    unsigned int num_open = 1, num_closed = 0;

    // Allocate for the first time, if necessary.
    if (!open) {
        open = calloc(capacity_open, sizeof (AStar_Node));
    }
    if (!closed) {
        closed = calloc(capacity_closed, sizeof (AStar_Node));
    }

    const unsigned int goal_x = (int) (tx * DK_ASTAR_GRANULARITY / DK_BLOCK_SIZE);
    const unsigned int goal_y = (int) (ty * DK_ASTAR_GRANULARITY / DK_BLOCK_SIZE);

    // Initialize the first open node to the one we're starting from.
    {
        open[0].x = (int) (unit->x * DK_ASTAR_GRANULARITY / DK_BLOCK_SIZE);
        open[0].y = (int) (unit->y * DK_ASTAR_GRANULARITY / DK_BLOCK_SIZE);
        open[0].gscore = 0.0f;
        const int dx = open[0].x - goal_x;
        const int dy = open[0].y - goal_y;
        open[0].fscore = a_star_f(0, 0, dx, dy);
        open[0].came_from = 0;
        open[0].steps = 1;
    }

    // Clear closed set bitset representation.
    BS_reset(a_star_closed_set, a_star_grid_size * a_star_grid_size);

    while (num_open > 0) {
        // Copy first (best) open entry to closed list, make sure we have the
        // space.
        if (num_closed >= capacity_closed - 1) {
            capacity_closed = capacity_closed * 2 + 1;
            closed = realloc(closed, capacity_closed * sizeof (AStar_Node));
        }
        closed[num_closed] = open[0];

        // Remember the current node.
        const AStar_Node* current = &closed[num_closed];

        // Check if we're there yet.
        if (a_star_test_goal(current, goal_x, goal_y, path, depth, length)) {
            return 1;
        }

        // Remember there's one more entry in the closed list now.
        ++num_closed;

        // Shift open list to the left to remove first entry.
        if (--num_open > 0) {
            memmove(&open[0], &open[1], num_open * sizeof (AStar_Node));
        }

        // Mark as closed in our bitset.
        BS_set(a_star_closed_set, current->y * a_star_grid_size + current->x);

        // Build our start / end indexes.
        int start_x = current->x - 1, start_y = current->y - 1,
                end_x = current->x + 1, end_y = current->y + 1;
        // Check if we have a direction.
        if (current->came_from) {
            const int dx = current->x - closed[current->came_from - 1].x,
                    dy = current->y - closed[current->came_from - 1].y;
            if (dx >= 0 && a_star_is_passable(current->x - 1, current->y)) {
                start_x = current->x;
            } else if (dx <= 0 && a_star_is_passable(current->x + 1, current->y)) {
                end_x = current->x;
            }
            if (dy >= 0 && a_star_is_passable(current->x, current->y - 1)) {
                start_y = current->y;
            } else if (dy <= 0 && a_star_is_passable(current->x, current->y + 1)) {
                end_y = current->y;
            }
        }
        int lx, ly;
        for (lx = start_x; lx <= end_x; ++lx) {
            // Don't go out of bounds.
            if (lx < 0 || lx >= a_star_grid_size) {
                continue;
            }
            for (ly = start_y; ly <= end_y; ++ly) {

#if DK_D_DRAW_PATHS
                glColor3f(0.0f, 0.0f, 1.0f);
                glDisable(GL_LIGHTING);
                glPushMatrix();
                glTranslatef(a_star_to_global(lx), a_star_to_global(ly), DK_D_PATH_HEIGHT / 2.0f);
                gluSphere(quadratic, 0.5f, 8, 8);
                glPopMatrix();
                glEnable(GL_LIGHTING);
#endif

                // Activate hyperdrive aaaaand... JUMP!
                int x = lx, y = ly;
#if DK_ASTAR_JPS
                if (!a_star_jump(x - current->x, y - current->y, &x, &y, goal_x, goal_y)) {
                    // Failed, try next neighbor.
                    continue;
                }

                // We might have jumped to the goal, check for that.
                if (a_star_test_goal(current, goal_x, goal_y, path, depth, length)) {
                    return 1;
                }
#else
                // Don't go out of bounds.
                if (y < 0 || y >= a_star_grid_size) {
                    continue;
                }

                // If we already handled this one, skip it.
                if (BS_test(a_star_closed_set, y * a_star_grid_size + x)) {
                    continue;
                }

                // Only if this block is passable and the two diagonal ones are.
                if (!a_star_is_passable(x, y) ||
                        (!a_star_is_passable(x, current->y) && !a_star_is_passable(current->x, y))) {
                    continue;
                }
#endif

                // Determine score - score to current plus that to neighbor.
                // If the neighbor is on the same x/y field it's 1, else it's
                // diagonal -> sqrt(2).
                const float gscore = current->gscore +
                        a_star_h(x, y, current->x, current->y);

                // See if we can find that neighbor in the open set.
                AStar_Node* neighbor = 0;
                unsigned int i;
                for (i = 0; i < num_open; ++i) {
                    if (open[i].x == x && open[i].y == y) {
                        neighbor = &open[i];
                        break;
                    }
                }

                // If it's already known and even has a better score, skip it.
                if (neighbor && neighbor->gscore <= gscore) {
                    continue;
                }

                // Compute the heuristic cost for a path with this waypoint.
                const float fscore = (gscore + a_star_f(x, y, goal_x, goal_y))
                        * 1.001f; // Tie breaking.

#if DK_D_DRAW_PATHS
                glColor3f(1.0f, 0.0f, 0.0f);
                glDisable(GL_LIGHTING);
                glPushMatrix();
                glTranslatef(a_star_to_global(x), a_star_to_global(y), DK_D_PATH_HEIGHT);
                gluSphere(quadratic, 0.5f, 8, 8);
                glPopMatrix();
                glEnable(GL_LIGHTING);
#endif

                if (neighbor) {
                    // Already in the list, remove and insert again to update
                    // its fscore based position in the list.
                    const unsigned int idx = neighbor - open;
                    memmove(neighbor, neighbor + 1, (num_open - idx) * sizeof (AStar_Node));

                    // Remember we removed it.
                    --num_open;
                }

                // Create new entry.
                if (num_open >= capacity_open - 1) {
                    capacity_open = capacity_open * 2 + 1;
                    open = realloc(open, capacity_open * sizeof (AStar_Node));
                }

                // Find where to insert; we want to keep this list sorted,
                // so that the entry with the lowest score is first.
                unsigned int low = 0, high = num_open;
                while (high > low) {
                    unsigned mid = (low + high) / 2;
                    if (open[mid].fscore < fscore) {
                        low = mid + 1;
                    } else {
                        high = mid;
                    }
                }

                // Move everything above one up, if necessary.
                if (low < num_open) {
                    memmove(&open[low + 1], &open[low], (num_open - low) * sizeof (AStar_Node));
                }

                // Remember it.
                neighbor = &open[low];
                neighbor->x = x;
                neighbor->y = y;

                // Remember there's one more now.
                ++num_open;

                // Initialize or update neighbor.
                neighbor->came_from = num_closed;
                neighbor->gscore = gscore;
                neighbor->fscore = fscore;
                neighbor->steps = current->steps + 1;
            }
        }
    }

    return 0;
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
                void* task = unit->ai[unit->ai_count].info;
                AIJob_Move* move = (AIJob_Move*) unit->ai[unit->ai_count + 1].info;

                int i;
                for (i = 0; i < workplace_count[unit->owner]; ++i) {
                    // Current workplace we're checking.
                    AI_Work* work = &workplaces[unit->owner][i];

                    // Find a path to it. Use temporary output data to avoid
                    // overriding existing path that may be shorter.
                    AStar_Waypoint path[sizeof (move->path)];
                    unsigned int depth;
                    float length;
                    if (a_star(unit, work->x, work->y, path, &depth, &length)) {
                        // Got a path, check its length.
                        if (length < best_length) {
                            // Got a new best. Copy data.
                            best_length = length;
                            best_work = work;
                            memcpy(move->path, path, depth * sizeof (AStar_Waypoint));
                            move->node_count = depth;
                        }
                    }
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
    BS_free(a_star_closed_set);
    a_star_grid_size = DK_map_size * DK_ASTAR_GRANULARITY;
    a_star_closed_set = BS_alloc(a_star_grid_size * a_star_grid_size);
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
        float tx = 11.5f * DK_BLOCK_SIZE;
        float ty = 9.99f * DK_BLOCK_SIZE;
        AStar_Waypoint path[DK_AI_PATH_MAX];
        unsigned int depth = sizeof (path);
        float length;

        if (a_star(unit, tx, ty, path, &depth, &length)) {
            glColor3f(1.0f, 1.0f, 1.0f);
            glLineWidth(3);
            glDisable(GL_LIGHTING);
            glBegin(GL_LINES);

            glVertex3f(unit->x, unit->y, DK_D_PATH_HEIGHT);
            glVertex3f(path[0].x, path[0].y, DK_D_PATH_HEIGHT);

            int j;
            for (j = 0; j < depth - 1; ++j) {
                glVertex3f(path[j].x, path[j].y, DK_D_PATH_HEIGHT);
                glVertex3f(path[j + 1].x, path[j + 1].y, DK_D_PATH_HEIGHT);
            }

            glVertex3f(path[j].x, path[j].y, DK_D_PATH_HEIGHT);
            glVertex3f(tx, ty, DK_D_PATH_HEIGHT);

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