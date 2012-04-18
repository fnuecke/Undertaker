#include <malloc.h>
#include <memory.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <assert.h>

#include "bitset.h"
#include "config.h"
#include "units.h"
#include "map.h"

///////////////////////////////////////////////////////////////////////////////
// Types and constants
///////////////////////////////////////////////////////////////////////////////

/** Must be in same order as unit definitions in unit type enum */
static float unit_speeds[] = {
    0.2f,
    0.1f
};

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
} DK_AIState;

typedef struct {
    float x, y;
} waypoint;

typedef struct {
    /** Current AI state */
    DK_AIState state;

    /** Current target position when moving (final coordinate) */
    float tx, ty;

    /** Enemy the unit currently attacks */
    unsigned int enemy;

    /** Coordinate of the block an imp digs or conquers */
    int bx, by;

    /** The path the unit currently follows (if moving) */
    waypoint path[32];

    /** The remaining length of the path */
    unsigned int path_length;

    //TODO: workplace
} DK_AIInfo;

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

    /** AI info */
    DK_AIInfo ai;
} DK_Unit;

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
} node;

///////////////////////////////////////////////////////////////////////////////
// Global variables
///////////////////////////////////////////////////////////////////////////////

/** Used for debug rendering of units and paths */
static GLUquadric* quadratic = 0;

/** Units in the game, must be ensured to be non-sparse */
static DK_Unit units[DK_PLAYER_COUNT * DK_UNITS_MAX_PER_PLAYER];

/** Total number of active units */
static unsigned int total_units = 0;

/** Number of units per player */
static unsigned int num_units[DK_PLAYER_COUNT] = {0};

/** Slots on the map that are*/
static char* work_occupied[DK_PLAYER_COUNT] = {0};

/** Number of cells in the grid used for our A* algorithm */
static unsigned int a_star_grid_size = 0;

/** Bitset for quick lookup of closed set in A* */
static char* a_star_closed = 0;

/** Capacity of A* node sets; set to the initial capacities, will grow as necessary */
static unsigned int capacity_open = 32, capacity_closed = 64;

/** Re-used waypoint sets for A* search */
static node* open = 0;
static node* closed = 0;

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

static float a_star_h(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1) {
    return (x0 == x1 || y0 == y1) ? 1 : M_SQRT2;
}

static float a_star_f(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1) {
    const unsigned int dx = abs(x0 - x1);
    const unsigned int dy = abs(y0 - y1);
    return M_SQRT2 * (dx > dy ? dx : dy);
}

static float a_star_to_global(unsigned int coordinate) {
    return (coordinate / (float) DK_ASTAR_GRANULARITY + (0.5f / DK_ASTAR_GRANULARITY)) * DK_BLOCK_SIZE;
}

static int a_star(const DK_Unit* unit, float tx, float ty, waypoint* path, unsigned int max_length) {

    // Check if the target position is valid (passable).
    if (!DK_block_is_passable(DK_block_at(
            (int) (tx / DK_BLOCK_SIZE), (int) (ty / DK_BLOCK_SIZE)))) {
        return 0;
    }

    // Number of entries used in the open and closed set.
    unsigned int num_open = 1, num_closed = 0;

    // Allocate for the first time, if necessary.
    if (!open) {
        open = calloc(capacity_open, sizeof (node));
    }
    if (!closed) {
        closed = calloc(capacity_closed, sizeof (node));
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
    BS_reset(a_star_closed, a_star_grid_size * a_star_grid_size);

    while (num_open > 0) {
        // Copy first (best) open entry to closed list, make sure we have the
        // space.
        if (num_closed >= capacity_closed - 1) {
            capacity_closed = capacity_closed * 2 + 1;
            closed = realloc(closed, capacity_closed * sizeof (node));
        }
        closed[num_closed] = open[0];

        // Remember the current node.
        const node* current = &closed[num_closed];

        // Check if we're there yet.
        if (current->x == goal_x && current->y == goal_y) {
            // Done, follow the path back to the beginning to return the best
            // neighboring node.

            // Follow the path until only as many nodes as we can fit into the
            // specified array remain.
            while (current->steps > max_length && current->came_from) {
                current = &closed[current->came_from - 1];
            }

            // In case we don't need the full capacity...
            max_length = current->steps;

            // Push the remaining nodes' coordinates (in reverse walk order to
            // make it forward work order).
            int i;
            for (i = max_length - 1; i >= 0; --i) {
                path[i].x = a_star_to_global(current->x);
                path[i].y = a_star_to_global(current->y);

                if (!current->came_from) {
                    break;
                } else {
                    current = &closed[current->came_from - 1];
                }
            }

            // Return the length of the returned array.
            return max_length;
        }

        // Remember there's one more entry in the closed list now.
        ++num_closed;

        // Shift open list to the left to remove first entry.
        if (--num_open > 0) {
            memmove(&open[0], &open[1], num_open * sizeof (node));
        }

        // Mark as closed in our bitset.
        BS_set(a_star_closed, current->y * a_star_grid_size + current->x);

        // Find the neighbor with the best score.
        int x, y;
        for (x = current->x - 1; x <= current->x + 1; ++x) {
            // Don't go out of bounds.
            if (x < 0 || x >= a_star_grid_size) {
                continue;
            }
            for (y = current->y - 1; y <= current->y + 1; ++y) {
                // Don't go out of bounds.
                if (y < 0 || y >= a_star_grid_size) {
                    continue;
                }

                // If we already handled this one, skip it.
                if (BS_test(a_star_closed, y * a_star_grid_size + x)) {
                    continue;
                }

                // Only if this block is passable.
                if (!DK_block_is_passable(DK_block_at(x / DK_ASTAR_GRANULARITY, y / DK_ASTAR_GRANULARITY))) {
                    continue;
                }

                // Determine score - score to current plus that to neighbor.
                // If the neighbor is on the same x/y field it's 1, else it's
                // diagonal -> sqrt(2).
                const float gscore = current->gscore +
                        a_star_h(x, y, current->x, current->y);

                // See if we can find that neighbor in the open set.
                node* neighbor = 0;
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
                    memmove(neighbor, neighbor + 1, (num_open - idx) * sizeof (node));

                    // Remember we removed it.
                    --num_open;
                }

                // Create new entry.
                if (num_open >= capacity_open - 1) {
                    capacity_open = capacity_open * 2 + 1;
                    open = realloc(open, capacity_open * sizeof (node));
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
                    memmove(&open[low + 1], &open[low], (num_open - low) * sizeof (node));
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
    switch (unit->ai.state) {
        case DK_AI_IDLE:
            // Find the unit something to do.
            if (unit->type == DK_UNIT_IMP) {
                // Look for work.
                
            }
            break;
        case DK_AI_MOVE:
            // AI is moving, check if we're there yet.
            if (fabs(unit->tx - unit->x) < 0.1f &&
                    fabs(unit->ty - unit->y) < 0.1f) {
                // Reached local checkpoint, see if we're at our final goal.
                if (fabs(unit->ai.tx - unit->tx) < 0.1f &&
                        fabs(unit->ai.ty - unit->ty) < 0.1f) {
                    // This was the final checkpoint, switch back to idling.
                    unit->ai.state = DK_AI_IDLE;
                } else {
                    // Not there yet, figure out best way to final position, set
                    // next checkpoint.

                }
            }
            break;
        default:
            // Unknown / invalid state, switch to idle and stop.
            unit->ai.state = DK_AI_IDLE;
            unit->tx = unit->x;
            unit->ty = unit->y;
            break;
    }
}

void DK_init_units() {
    BS_free(a_star_closed);
    a_star_grid_size = DK_map_size * DK_ASTAR_GRANULARITY;
    a_star_closed = BS_alloc(a_star_grid_size * a_star_grid_size);
    int i;
    for (i = 0; i < DK_PLAYER_COUNT - 1; ++i) {
        BS_free(work_occupied[i]);
        work_occupied[i] = BS_alloc(a_star_grid_size * a_star_grid_size);
    }
}

void DK_update_units() {
    int i;
    for (i = 0; i < total_units; ++i) {
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
    for (i = 0; i < total_units; ++i) {
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
        waypoint path[64];
        unsigned int length = a_star(unit, tx, ty, path, 64);

        if (length > 0) {
            glColor3f(1.0f, 1.0f, 1.0f);
            glLineWidth(3);
            glDisable(GL_LIGHTING);
            glBegin(GL_LINES);

            glVertex3f(unit->x, unit->y, DK_D_PATH_HEIGHT);
            glVertex3f(path[0].x, path[0].y, DK_D_PATH_HEIGHT);

            for (i = 0; i < length - 1; ++i) {
                glVertex3f(path[i].x, path[i].y, DK_D_PATH_HEIGHT);
                glVertex3f(path[i + 1].x, path[i + 1].y, DK_D_PATH_HEIGHT);
            }

            glVertex3f(path[i].x, path[i].y, DK_D_PATH_HEIGHT);
            glVertex3f(tx, ty, DK_D_PATH_HEIGHT);

            glEnd();
            glEnable(GL_LIGHTING);
        }
#endif
    }
}

unsigned int DK_add_unit(DK_Player player, DK_UnitType type, unsigned short x, unsigned short y) {
    if (total_units > DK_PLAYER_COUNT * DK_UNITS_MAX_PER_PLAYER) {
        // TODO
        return;
    }

    // Check if the block is valid.
    if (!DK_block_is_passable(DK_block_at(x, y))) {
        // TODO
        return;
    }

    DK_Unit* unit = &units[total_units++];
    unit->type = type;
    unit->ai.state = DK_AI_IDLE;
    unit->x = (x + 0.5f) * DK_BLOCK_SIZE;
    unit->y = (y + 0.5f) * DK_BLOCK_SIZE;
    unit->tx = unit->x;
    unit->ty = unit->y;
    unit->ms = unit_speeds[type];
}