#include <malloc.h>
#include <math.h>
#include <memory.h>

#include "astar.h"
#include "bitset.h"
#include "config.h"
#include "map.h"

///////////////////////////////////////////////////////////////////////////////
// Types
///////////////////////////////////////////////////////////////////////////////

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
// Globals
///////////////////////////////////////////////////////////////////////////////

/** Number of cells in the grid used for our A* algorithm */
static unsigned int a_star_grid_size = 0;

/** Bitset for quick lookup of closed set in A* */
static char* a_star_closed_set = 0;

/** Capacity of A* node sets; set to the initial capacities, will grow as necessary */
static unsigned int open_capacity = 32;
static unsigned int closed_capacity = 64;

/** Re-used waypoint sets for A* search */
static AStar_Node* open = 0;
static AStar_Node* closed = 0;

///////////////////////////////////////////////////////////////////////////////
// Utility methods
///////////////////////////////////////////////////////////////////////////////

/** Computes actual path costs */
inline static float a_star_h(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1) {
    const int dx = x1 - x0;
    const int dy = y1 - y0;
    return sqrtf(dx * dx + dy * dy);
    return (x0 == x1 || y0 == y1) ? 1 : M_SQRT2;
}

/** Computes the heuristic */
inline static float a_star_f(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1) {
    const unsigned int dx = abs(x0 - x1);
    const unsigned int dy = abs(y0 - y1);
    return M_SQRT2 * (dx > dy ? dx : dy);
}

/** Converts local coordinates to openGL global ones */
inline static float a_star_to_global(unsigned int coordinate) {
    return (coordinate / (float) DK_ASTAR_GRANULARITY + (0.5f / DK_ASTAR_GRANULARITY));
}

/** Checks if a block is passable, using local coordinates */
inline static int a_star_is_passable(unsigned int x, unsigned int y) {
    return DK_block_is_passable(DK_block_at(x / DK_ASTAR_GRANULARITY, y / DK_ASTAR_GRANULARITY));
}

/** Performs a jump point search */
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

/** Tests if we have reached our goal and writes out result data if yes */
static int a_star_test_goal(const AStar_Node* n, int gx, int gy, float tx, float ty, AStar_Waypoint* path, unsigned int* depth, float* length) {
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
            if (path[i].x == gx && path[i].y == gy) {
                path[i].x = tx;
                path[i].y = ty;
            } else {
                path[i].x = a_star_to_global(n->x);
                path[i].y = a_star_to_global(n->y);
            }

            if (!n->came_from) {
                break;
            } else {
                n = &closed[n->came_from - 1];
            }
        }

        // Return success.
        return 1;
    }

    // Return failure.
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

void DK_init_a_star() {
    BS_free(a_star_closed_set);
    a_star_grid_size = DK_map_size * DK_ASTAR_GRANULARITY;
    a_star_closed_set = BS_alloc(a_star_grid_size * a_star_grid_size);
}

int DK_a_star(float sx, float sy, float tx, float ty, AStar_Waypoint* path, unsigned int* depth, float* length) {

    // Check if the target position is valid (passable).
    if (!DK_block_is_passable(DK_block_at((int) tx, (int) ty))) {
        return 0;
    }

    // Number of entries used in the open and closed set.
    unsigned int open_count = 1, closed_count = 0;

    // Allocate for the first time, if necessary.
    if (!open) {
        open = calloc(open_capacity, sizeof (AStar_Node));
    }
    if (!closed) {
        closed = calloc(closed_capacity, sizeof (AStar_Node));
    }

    const unsigned int gx = (int) (tx * DK_ASTAR_GRANULARITY);
    const unsigned int gy = (int) (ty * DK_ASTAR_GRANULARITY);

    // Initialize the first open node to the one we're starting from.
    {
        open[0].x = (int) (sx * DK_ASTAR_GRANULARITY);
        open[0].y = (int) (sy * DK_ASTAR_GRANULARITY);
        open[0].gscore = 0.0f;
        const int dx = open[0].x - gx;
        const int dy = open[0].y - gy;
        open[0].fscore = a_star_f(0, 0, dx, dy);
        open[0].came_from = 0;
        open[0].steps = 1;
    }

    // Clear closed set bitset representation.
    BS_reset(a_star_closed_set, a_star_grid_size * a_star_grid_size);

    while (open_count > 0) {
        // Copy first (best) open entry to closed list, make sure we have the
        // space.
        if (closed_count >= closed_capacity - 1) {
            closed_capacity = closed_capacity * 2 + 1;
            closed = realloc(closed, closed_capacity * sizeof (AStar_Node));
        }
        closed[closed_count] = open[0];

        // Remember the current node.
        const AStar_Node* current = &closed[closed_count];

        // Check if we're there yet.
        if (a_star_test_goal(current, gx, gy, tx, ty, path, depth, length)) {
            return 1;
        }

        // Remember there's one more entry in the closed list now.
        ++closed_count;

        // Shift open list to the left to remove first entry.
        if (--open_count > 0) {
            memmove(&open[0], &open[1], open_count * sizeof (AStar_Node));
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

                /*
                #if DK_D_DRAW_PATHS
                                glColor3f(0.0f, 0.0f, 1.0f);
                                glDisable(GL_LIGHTING);
                                glPushMatrix();
                                glTranslatef(a_star_to_global(lx), a_star_to_global(ly), DK_D_PATH_HEIGHT / 2.0f);
                                gluSphere(quadratic, 0.5f, 8, 8);
                                glPopMatrix();
                                glEnable(GL_LIGHTING);
                #endif
                 */

                // Activate hyperdrive aaaaand... JUMP!
                int x = lx, y = ly;
#if DK_ASTAR_JPS
                if (!a_star_jump(x - current->x, y - current->y, &x, &y, gx, gy)) {
                    // Failed, try next neighbor.
                    continue;
                }

                // We might have jumped to the goal, check for that.
                if (a_star_test_goal(current, gx, gy, tx, ty, path, depth, length)) {
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
                for (i = 0; i < open_count; ++i) {
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
                const float fscore = (gscore + a_star_f(x, y, gx, gy))
                        * 1.001f; // Tie breaking.

                /*
                #if DK_D_DRAW_PATHS
                                glColor3f(1.0f, 0.0f, 0.0f);
                                glDisable(GL_LIGHTING);
                                glPushMatrix();
                                glTranslatef(a_star_to_global(x), a_star_to_global(y), DK_D_PATH_HEIGHT);
                                gluSphere(quadratic, 0.5f, 8, 8);
                                glPopMatrix();
                                glEnable(GL_LIGHTING);
                #endif
                 */

                if (neighbor) {
                    // Already in the list, remove and insert again to update
                    // its fscore based position in the list.
                    const unsigned int idx = neighbor - open;
                    memmove(neighbor, neighbor + 1, (open_count - idx) * sizeof (AStar_Node));

                    // Remember we removed it.
                    --open_count;
                }

                // Create new entry.
                if (open_count >= open_capacity - 1) {
                    open_capacity = open_capacity * 2 + 1;
                    open = realloc(open, open_capacity * sizeof (AStar_Node));
                }

                // Find where to insert; we want to keep this list sorted,
                // so that the entry with the lowest score is first.
                unsigned int low = 0, high = open_count;
                while (high > low) {
                    unsigned mid = (low + high) / 2;
                    if (open[mid].fscore < fscore) {
                        low = mid + 1;
                    } else {
                        high = mid;
                    }
                }

                // Move everything above one up, if necessary.
                if (low < open_count) {
                    memmove(&open[low + 1], &open[low], (open_count - low) * sizeof (AStar_Node));
                }

                // Remember it.
                neighbor = &open[low];
                neighbor->x = x;
                neighbor->y = y;

                // Remember there's one more now.
                ++open_count;

                // Initialize or update neighbor.
                neighbor->came_from = closed_count;
                neighbor->gscore = gscore;
                neighbor->fscore = fscore;
                neighbor->steps = current->steps + 1;
            }
        }
    }

    return 0;
}
