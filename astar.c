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

/** Re-used waypoint sets for A* search */
static AStar_Node* open = 0;
static AStar_Node* closed = 0;

/** Capacity of A* node sets; set to the initial capacities, will grow as necessary */
static unsigned int open_capacity = 0;
static unsigned int closed_capacity = 0;

/** Number of entries used in the open and closed set */
static unsigned int open_count = 0;
static unsigned int closed_count = 0;

///////////////////////////////////////////////////////////////////////////////
// Allocation
///////////////////////////////////////////////////////////////////////////////

/** Get a new node for the open queue, at the specified sort key */
static AStar_Node* get_open(float key) {
    // Ensure we have the capacity to add the node.
    if (open_count + 1 >= open_capacity) {
        open_capacity = open_capacity * 2 + 1;
        open = realloc(open, open_capacity * sizeof (AStar_Node));
    }

    // Find where to insert; we want to keep this list sorted, so that the entry
    // with the lowest score is first.
    unsigned int low = 0, high = open_count;
    while (high > low) {
        unsigned mid = (low + high) / 2;
        if (open[mid].fscore < key) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    // Move everything above one up, if necessary.
    if (low < open_count) {
        memmove(&open[low + 1], &open[low], (open_count - low) * sizeof (AStar_Node));
    }

    // Remember there's one more now.
    ++open_count;

    // Return it.
    return &open[low];
}

/** Move the first entry in the open queue to the closed set and return it */
static AStar_Node* pop_to_closed() {
    // Ensure we have the capacity to add the node.
    if (closed_count + 1 >= closed_capacity) {
        closed_capacity = closed_capacity * 2 + 1;
        closed = realloc(closed, closed_capacity * sizeof (AStar_Node));
    }

    // Copy it over.
    closed[closed_count] = open[0];

    // Mark as closed in our bitset.
    BS_set(a_star_closed_set, open[0].y * a_star_grid_size + open[0].x);

    // Shift open list to the left to remove first entry.
    --open_count;
    memmove(&open[0], &open[1], open_count * sizeof (AStar_Node));

    // Return the copied node.
    return &closed[closed_count++];
}

///////////////////////////////////////////////////////////////////////////////
// Utility methods
///////////////////////////////////////////////////////////////////////////////

/** Clamps an arbitrary coordinate to a valid one (in bounds) */
inline static unsigned int clamp(int coordinate) {
    if (coordinate < 0) {
        return 0;
    }
    if (coordinate >= a_star_grid_size) {
        return a_star_grid_size - 1;
    }
    return coordinate;
}

/** Computes actual path costs */
inline static float h(unsigned int from_x, unsigned int from_y, unsigned int to_x, unsigned int to_y) {
    const int dx = to_x - from_x;
    const int dy = to_y - from_y;
    return sqrtf(dx * dx + dy * dy);
}

/** Computes the heuristic */
inline static float f(unsigned int node_x, unsigned int node_y, unsigned int goal_x, unsigned int goal_y) {
    const unsigned int dx = abs(node_x - goal_x);
    const unsigned int dy = abs(node_y - goal_y);
    return M_SQRT2 * (dx > dy ? dx : dy);
}

/** Converts local coordinates to map ones */
inline static float to_map_space(unsigned int node_coordinate) {
    return (node_coordinate / (float) DK_ASTAR_GRANULARITY + (0.5f / DK_ASTAR_GRANULARITY));
}

/** Checks if a block is passable, using local coordinates */
inline static int is_passable(unsigned int node_x, unsigned int node_y) {
    return DK_block_is_passable(DK_block_at(node_x / DK_ASTAR_GRANULARITY, node_y / DK_ASTAR_GRANULARITY));
}

/** Tests if a neighbor should be skipped due to it already having a better score */
static int should_skip(unsigned int x, unsigned int y, float gscore) {
    // Try to find the neighbor.
    unsigned int i;
    for (i = 0; i < open_count; ++i) {
        if (open[i].x == x && open[i].y == y) {
            // Neighbor is already in open list.
            if (open[i].gscore <= gscore) {
                // Skip it, it has a better score.
                return 1;
            }
            // Otherwise we remove it from the list so that it may be
            // re-inserted.
            --open_count;
            memmove(&open[i], &open[i + 1], (open_count - i) * sizeof (AStar_Node));
            return 0;
        }
    }
    return 0;
}

/**
 * Performs a jump point search.
 * 
 * @param jx the x coordinate we jumped to, if successful.
 * @param jx the y coordinate we jumped to, if successful.
 * @param dx movement in x direction.
 * @param dy movement in y direction.
 * @param sx start point of search, also used to return the point we jump to.
 * @param sy start point of search, also used to return the point we jump to.
 * @param gx goal x coordinate (so we don't jump past it).
 * @param gy goal y coordinate (so we don't jump past it).
 * @return whether the jump was successful or not.
 */
static int jps(int* jx, int* jy, int dx, int dy, int sx, int sy, int gx, int gy) {
    // Don't go out of bounds.
    if (sx < 0 || sx >= a_star_grid_size ||
            sy < 0 || sy >= a_star_grid_size) {
        return 0;
    }

    // If we already handled this one, skip it.
    if (BS_test(a_star_closed_set, sy * a_star_grid_size + sx)) {
        return 0;
    }

    // Only continue if this block is passable and the two diagonal ones are.
    // Otherwise we hit an obstacle and thus failed.
    if (!is_passable(sx, sy) ||
            (!is_passable(sx, sy - dy) && !is_passable(sx - dx, sy))) {
        return 0;
    }

    // We have potential to succeed, so save our local coordinates.
    if (jx) {
        *jx = sx;
    }
    if (jy) {
        *jy = sy;
    }

    // Have we reached the goal?
    if (sx == gx && sy == gy) {
        return 1;
    }

    // Do we have to evaluate neighbors here and end our jump?
    if (
            // If we move along the x axis...
            (dx &&
            // ... and there's an obstacle above, blocking a passable tile...
            ((!is_passable(sx, sy - 1) && is_passable(sx + dx, sy - 1)) ||
            // ... or below us, blocking a passable tile...
            (!is_passable(sx, sy + 1) && is_passable(sx + dx, sy + 1))) ||
            // ... or we're moving along the y axis...
            (dy &&
            // ... and there's an obstacle to the left, blocking a passable tile...
            ((!is_passable(sx - 1, sy) && is_passable(sx - 1, sy + dy)) ||
            // ... or to the right of us, blocking a passable tile...
            (!is_passable(sx + 1, sy) && is_passable(sx + 1, sy + dy)))))) {
        // ... then we have to inspect this tile, so we end our jump.
        return 1;
    }

    // Moving diagonally?
    if (dx && dy) {
        // Yes, then try the straight ones first.
        if (jps(NULL, NULL, dx, 0, sx + dx, sy, gx, gy)) {
            return 1;
        }
        if (jps(NULL, NULL, 0, dy, sx, sy + dy, gx, gy)) {
            return 1;
        }
    }

    // Not invalidated yet, remember this position and move on ahead.
    return jps(jx, jy, dx, dy, sx + dx, sy + dy, gx, gy);
}

/**
 * Tests if we have reached our goal and writes out result data if yes.
 * @param path the buffer to write the path to.
 * @param depth the length of the buffer, set to the used length.
 * @param length the actual length of the found path, in map space.
 * @param node the current node.
 * @param local_goal_x the goal x coordinate in A* space.
 * @param local_goal_y the goal y coordinate in A* space.
 * @param start_x the start x coordinate in map space.
 * @param start_y the start y coordinate in map space.
 * @param goal_x the goal x coordinate in map space.
 * @param goal_y the goal y coordinate in map space.
 * @return whether the goal was reached or not.
 */
static int is_goal(AStar_Waypoint* path, unsigned int* depth, float* length,
        const AStar_Node* node,
        unsigned int local_goal_x, unsigned int local_goal_y,
        float start_x, float start_y,
        float goal_x, float goal_y) {
    // Test whether node coordinates are goal coordinates.
    if (node->x == local_goal_x && node->y == local_goal_y) {
        // Done, follow the path back to the beginning to return the best
        // neighboring node.

        // Return the length of the returned array. This isn't 100% accurate,
        // because we replace the start and end points with the search start
        // and end coordinates, but it's close enough.
        *length = node->gscore / DK_ASTAR_GRANULARITY;

        // Follow the path until only as many nodes as we can fit into the
        // specified buffer remain.
        while (node->steps > *depth && node->came_from) {
            node = &closed[node->came_from - 1];
        }

        // Then set the capacity to what we actually need.
        *depth = node->steps + 1;

        // Push the remaining nodes' coordinates (in reverse walk order to
        // make it forward work order).
        int i;
        for (i = *depth - 1; i >= 0; --i) {
            AStar_Waypoint* waypoint = &path[i];
            if (!node->came_from) {
                // This is the start node, set it to the actual start
                // coordinates, instead of the ones of the waypoint.
                waypoint->x = start_x;
                waypoint->y = start_y;
            } else {
                if (waypoint->x == local_goal_x &&
                        waypoint->y == local_goal_y) {
                    // Use the actual goal coordinates instead of those of the
                    // actual waypoint for the goal.
                    waypoint->x = goal_x;
                    waypoint->y = goal_y;
                } else {
                    // Normal waypoint, convert it to map space coordinates.
                    waypoint->x = to_map_space(node->x);
                    waypoint->y = to_map_space(node->y);
                }

                // Continue on with the next node.
                node = &closed[node->came_from - 1];
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

int DK_a_star(float start_x, float start_y, float goal_x, float goal_y, AStar_Waypoint* path, unsigned int* depth, float* length) {
    // Check if the start and target position are valid (passable).
    if (!DK_block_is_passable(DK_block_at((int) start_x, (int) start_y)) ||
            !DK_block_is_passable(DK_block_at((int) goal_x, (int) goal_y))) {
        return 0;
    }

    // Reset number of entries used in the open and closed set.
    open_count = 0;
    closed_count = 0;

    // Clear closed set bitset representation.
    BS_reset(a_star_closed_set, a_star_grid_size * a_star_grid_size);

    // Get goal in local coordinates.
    const unsigned int gx = (int) (goal_x * DK_ASTAR_GRANULARITY);
    const unsigned int gy = (int) (goal_y * DK_ASTAR_GRANULARITY);

    // Initialize the first open node to the one we're starting from.
    {
        const int sx = (int) (start_x * DK_ASTAR_GRANULARITY);
        const int sy = (int) (start_y * DK_ASTAR_GRANULARITY);
        const int dx = sx - gx;
        const int dy = sy - gy;

        AStar_Node* start = get_open(0);
        start->x = sx;
        start->y = sy;
        start->gscore = 0.0f;
        start->fscore = f(0, 0, dx, dy);
        start->came_from = 0;
        start->steps = 0;
    }

    // Do the actual search.
    while (open_count > 0) {
        // Copy first (best) open entry to closed list.
        const AStar_Node* current = pop_to_closed();

        // Check if we're there yet.
        if (is_goal(path, depth, length, current, gx, gy, start_x, start_y, goal_x, goal_y)) {
            return 1;
        }

        // Check our neighbors. Determine which neighbors we actually need to
        // check based on the direction we came from. Per default enable all.
        unsigned int begin_x = clamp(current->x - 1),
                begin_y = clamp(current->y - 1),
                end_x = clamp(current->x + 1),
                end_y = clamp(current->y + 1);

        // Check if we have a direction, i.e. we came from somewhere. The
        // exception is the start node, where we'll have to check all neighbors.
        if (current->came_from) {
            // Compute the direction as an integer of {-1, 0, 1} for both axii.
            const int dx = current->x - closed[current->came_from - 1].x,
                    dy = current->y - closed[current->came_from - 1].y;

            // If we're only moving to the right we don't have to check left. If
            // we're also moving vertically, though, we need to check for an
            // obstacle (forced neighbors), which we do via the passable check.
            // This works analogous for all other movement directions.
            if (dx >= 0 && is_passable(current->x - 1, current->y)) {
                // Don't have to check to the left.
                begin_x = current->x;
            } else if (dx <= 0 && is_passable(current->x + 1, current->y)) {
                // Don't have to check to the right.
                end_x = current->x;
            }
            if (dy >= 0 && is_passable(current->x, current->y - 1)) {
                // Don't have to check up.
                begin_y = current->y;
            } else if (dy <= 0 && is_passable(current->x, current->y + 1)) {
                // Don't have to check down.
                end_y = current->y;
            }
        }

        // Now work through all our selected neighbors.
        unsigned int neighbor_x, neighbor_y;
        for (neighbor_x = begin_x; neighbor_x <= end_x; ++neighbor_x) {
            for (neighbor_y = begin_y; neighbor_y <= end_y; ++neighbor_y) {
                // Skip self.
                if (neighbor_x == current->x && neighbor_y == current->y) {
                    continue;
                }

                // The actual node to be inspected.
                unsigned int x = neighbor_x, y = neighbor_y;

#if DK_ASTAR_JPS
                // Try this direction using jump point search.
                if (!jps(&x, &y, x - current->x, y - current->y, x, y, gx, gy)) {
                    // Failed, try next neighbor.
                    continue;
                }

                // We might have jumped to the goal, check for that.
                if (is_goal(path, depth, length, current, gx, gy, start_x, start_y, goal_x, goal_y)) {
                    return 1;
                }
#else
                // If we already handled this one, skip it.
                if (BS_test(a_star_closed_set, y * a_star_grid_size + x)) {
                    continue;
                }

                // Only if this block is passable and the two diagonal ones are.
                if (!is_passable(x, y) ||
                        (!is_passable(x, current->y) && !is_passable(current->x, y))) {
                    continue;
                }
#endif

                // Determine score - score to current node plus that to this
                // neighbor. This is the actual traveled distance (Euclidean
                // distance -- we do an actual computation here, because we
                // might have skipped some tiles).
                const float gscore = current->gscore +
                        h(x, y, current->x, current->y);

                // See if we can find that neighbor in the open set and should
                // skip it, because there already exists a better path to it.
                if (should_skip(x, y, gscore)) {
                    continue;
                }

                // Compute the heuristic cost for a path with this waypoint.
                // The factor in the end is used for tie breaking.
                const float fscore = (gscore + f(x, y, gx, gy)) * 1.001f;

                // Create new entry.
                AStar_Node* node = get_open(fscore);

                // Store position of that node. Used for "contains" checks.
                node->x = x;
                node->y = y;

                // Remember where we came from and how deep the path is.
                node->came_from = closed_count;
                node->steps = current->steps + 1;

                // Also keep the scores, g for continuous computation, f for
                // sorting.
                node->gscore = gscore;
                node->fscore = fscore;
            }
        }
    }

    return 0;
}
