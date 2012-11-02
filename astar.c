#include <assert.h>
#include <malloc.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>

#include "astar.h"
#include "bitset.h"

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
} PathNode;

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

/** Root of two */
static const float SQRT2 = 1.41421356237309504880f;

/** Number of cells in the grid used for our A* algorithm */
static int gGridSize = 0;
static int gGridCapacity = 0;

/** Bitset for quick lookup of closed set in A* */
static char* gClosedSetLookup = 0;

/** Re-used waypoint sets for A* search */
static PathNode* gOpenSet = 0;
static PathNode* gClosedSet = 0;

/** Capacity of A* node sets */
static unsigned int gOpenSetCapacity = 0;
static unsigned int gClosedSetCapacity = 0;

/** Number of entries used in the open and closed set */
static unsigned int gOpenSetCount = 0;
static unsigned int gClosedSetCount = 0;

/** The unit we're currently checking for */
static int(*gIsPassable)(float x, float y);

///////////////////////////////////////////////////////////////////////////////
// Allocation
///////////////////////////////////////////////////////////////////////////////

/** Get a new node for the open queue, at the specified sort key */
static PathNode* newOpenNode(float key) {
    unsigned int low = 0, high = gOpenSetCount;

    // Ensure we have the capacity to add the node.
    if (gOpenSetCount + 1 >= gOpenSetCapacity) {
        gOpenSetCapacity = gOpenSetCapacity * 2 + 8;
        if (!(gOpenSet = realloc(gOpenSet, gOpenSetCapacity * sizeof (PathNode)))) {
            fprintf(stderr, "Out of memory while resizing A* open queue.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Find where to insert; we want to keep this list sorted, so that the entry
    // with the lowest score is first.
    while (high > low) {
        unsigned mid = (low + high) / 2;
        if (gOpenSet[mid].fscore < key) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    // Move everything above one up, if necessary.
    if (low < gOpenSetCount) {
        memmove(&gOpenSet[low + 1], &gOpenSet[low],
                (gOpenSetCount - low) * sizeof (PathNode));
    }

    // Remember there's one more now.
    ++gOpenSetCount;

    // Return it.
    return &gOpenSet[low];
}

/** Move the first entry in the open queue to the closed set and return it */
static PathNode* popOpenNodeToClosedSet(void) {
    // Ensure we have the capacity to add the node.
    if (gClosedSetCount + 1 >= gClosedSetCapacity) {
        gClosedSetCapacity = gClosedSetCapacity * 2 + 8;
        if (!(gClosedSet = realloc(gClosedSet, gClosedSetCapacity * sizeof (PathNode)))) {
            fprintf(stderr, "Out of memory while resizing A* closed set.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Copy it over.
    gClosedSet[gClosedSetCount] = gOpenSet[0];

    // Mark as closed in our bitset.
    BS_Set(gClosedSetLookup, gOpenSet[0].y * gGridSize + gOpenSet[0].x);

    // Shift open list to the left to remove first entry.
    --gOpenSetCount;
    memmove(&gOpenSet[0], &gOpenSet[1], gOpenSetCount * sizeof (PathNode));

    // Return the copied node.
    return &gClosedSet[gClosedSetCount++];
}

///////////////////////////////////////////////////////////////////////////////
// Utility methods
///////////////////////////////////////////////////////////////////////////////

/** Clamps an arbitrary coordinate to a valid one (in bounds) */
inline static unsigned int clamp(int coordinate) {
    if (coordinate < 0) {
        return 0;
    }
    if (coordinate >= gGridSize) {
        return gGridSize - 1;
    }
    return coordinate;
}

/** Computes actual path costs */
inline static float h(unsigned int fromX, unsigned int fromY,
                      unsigned int toX, unsigned int toY) {
    const int dx = toX - fromX;
    const int dy = toY - fromY;
    return sqrtf(dx * dx + dy * dy);
}

/** Computes the heuristic */
inline static float f(unsigned int x, unsigned int y,
                      unsigned int goalX, unsigned int goalY) {
    const unsigned int dx = abs(x - goalX);
    const unsigned int dy = abs(y - goalY);
    return SQRT2 * (dx > dy ? dx : dy);
}

/** Converts local coordinates to global ones */
inline static float toGlobal(unsigned int coordinate) {
    return (coordinate + 0.5f) / (float) ASTAR_GRANULARITY;
}

/** Converts global coordinates to local ones */
inline static float toLocal(float coordinate) {
    return (unsigned int) floorf(coordinate * ASTAR_GRANULARITY);
}

/** Checks if a block is passable, using local coordinates */
inline static int isPassable(unsigned int x, unsigned int y) {
    return gIsPassable(toGlobal(x), toGlobal(y));
}

/** Tests if a neighbor should be skipped due to it already having a better score */
static int shouldSkip(unsigned int x, unsigned int y, float gscore) {
    // Try to find the neighbor.
    for (unsigned int i = 0; i < gOpenSetCount; ++i) {
        if (gOpenSet[i].x == x && gOpenSet[i].y == y) {
            // Neighbor is already in open list.
            if (gOpenSet[i].gscore <= gscore) {
                // Skip it, it has a better score.
                return 1;
            }
            // Otherwise we remove it from the list so that it may be
            // re-inserted.
            --gOpenSetCount;
            memmove(&gOpenSet[i], &gOpenSet[i + 1], (gOpenSetCount - i) * sizeof (PathNode));
            return 0;
        }
    }
    return 0;
}

#if ASTAR_JPS

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
static int jumpPointSearch(int* jx, int* jy, int dx, int dy,
                           int sx, int sy, unsigned int gx, unsigned int gy) {
    // Don't go out of bounds.
    if (sx < 0 || sx >= gGridSize ||
        sy < 0 || sy >= gGridSize) {
        return 0;
    }

    // If we already handled this one, skip it.
    if (BS_Test(gClosedSetLookup, sy * gGridSize + sx)) {
        return 0;
    }

    // Only continue if this block is passable and the two diagonal ones are.
    // Otherwise we hit an obstacle and thus failed.
    if (!isPassable(sx, sy) ||
        (!isPassable(sx, sy - dy) && !isPassable(sx - dx, sy))) {
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
    if ((unsigned int) sx == gx && (unsigned int) sy == gy) {
        return 1;
    }

    // Do we have to evaluate neighbors here and end our jump?
    if (
        // If we move along the x axis...
        ((dx &&
        // ... and there's an obstacle above, blocking a passable tile...
        ((!isPassable(sx, sy - 1) && isPassable(sx + dx, sy - 1)) ||
        // ... or below us, blocking a passable tile...
        (!isPassable(sx, sy + 1) && isPassable(sx + dx, sy + 1)))) ||
        // ... or we're moving along the y axis...
        (dy &&
        // ... and there's an obstacle to the left, blocking a passable tile...
        ((!isPassable(sx - 1, sy) && isPassable(sx - 1, sy + dy)) ||
        // ... or to the right of us, blocking a passable tile...
        (!isPassable(sx + 1, sy) && isPassable(sx + 1, sy + dy)))))) {
        // ... then we have to inspect this tile, so we end our jump.
        return 1;
    }

    // Moving diagonally?
    if (dx && dy) {
        // Yes, then try the straight ones first.
        if (jumpPointSearch(NULL, NULL, dx, 0, sx + dx, sy, gx, gy)) {
            return 1;
        }
        if (jumpPointSearch(NULL, NULL, 0, dy, sx, sy + dy, gx, gy)) {
            return 1;
        }
    }

    // Not invalidated yet, remember this position and move on ahead.
    return jumpPointSearch(jx, jy, dx, dy, sx + dx, sy + dy, gx, gy);
}

#endif

/** Tests whether two nodes are visible to each other */
static int isInLineOfSight(const PathNode* a, const PathNode* b) {
    // We essentially draw a line from a to b and check if all the pixels we'd
    // set would be on passable tiles. Algorithm from wikipedia:
    // https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm#Simplification
    const int dx = abs(b->x - a->x);
    const int dy = abs(b->y - a->y);
    const int sx = (a->x < b->x) ? 1 : -1;
    const int sy = (a->y < b->y) ? 1 : -1;

    int err = dx - dy;
    unsigned int x = a->x, y = a->y;
    while (1) {
        if (!isPassable(x, y)) {
            return 0;
        }
        if (x == b->x && y == b->y) {
            return 1;
        }
        {
            const int e2 = 2 * err;
            if (e2 > -dy) {
                err = err - dy;
                x = x + sx;
            }
            if (e2 < dx) {
                err = err + dx;
                y = y + sy;
            }
        }
    }
    return 1;
}

/**
 * Cleans up a path ending in the specified node, removing all in-between nodes
 * that make the path unnecessarily long -- i.e. links nodes if they're within
 * each other's line of sight; where line of sight is determined by passability
 * of in-between blocks.
 */
static unsigned int prunePath(PathNode* tail) {
    // The capacity to what we actually need if we use the full path.
    unsigned int depth = tail->steps;

    // Done, follow the path back to the beginning to return the best
    // neighboring node.
    PathNode *n0, *n1, *n2;
    // Do some path cleaning: throw out nodes that are between two other
    // nodes that can "see" each other.
    n1 = tail;
    if (n1->came_from) {
        n0 = &gClosedSet[n1->came_from - 1];
        while (n0->came_from) {
            n2 = n1;
            n1 = n0;
            n0 = &gClosedSet[n0->came_from - 1]; // = n(-1)

            // See if we can skip the middle one.
            if (isInLineOfSight(n0, n2)) {
                // Yes, take out the middle-man.
                n2->came_from = n1->came_from;
                n1 = n2;
                --depth;
            }
        }
    }

    // Force minimum length of the path for start and end node (can be one
    // if goal is in same cell as start otherwise).
    if (depth < 2) {
        return 2;
    }
    return depth;
}

/**
 * Computes the actual length (in map space) of a path ending in the specified
 * tail node, with the specified start and goal positions.
 */
static float computeLength(const PathNode* tail, const vec2* start, const vec2* goal) {
    // Accumulate length of the path.
    float length = 0;

    if (tail->came_from) {
        // Position of last waypoint (init with goal coordinates to get the
        // actual distance to the goal position, not the last node).
        float lx = goal->d.x;
        float ly = goal->d.y;

        // Skip last node, as it'll be replaced with the goal coordinates.
        const PathNode* n = &gClosedSet[tail->came_from - 1];

        // Run until we get to the start node (which we'll replace with the
        // start coordinates).
        while (n->came_from) {
            // Compute distance.
            const float nx = toGlobal(n->x);
            const float ny = toGlobal(n->y);
            const float dx = nx - lx;
            const float dy = ny - ly;
            length += sqrtf(dx * dx + dy * dy);

            // Store this position as the last one and move on to the next node.
            lx = nx;
            ly = ny;
            n = &gClosedSet[n->came_from - 1];
        }
        // For the last node, use the start coordinates.
        {
            const float dx = start->d.x - lx;
            const float dy = start->d.y - ly;
            length += sqrtf(dx * dx + dy * dy);
        }
    }

    return length;
}

/**
 * Converts the path ending in the specified tail node to a buffer of the
 * specified length.
 */
static unsigned int writePath(vec2* path, unsigned int depth,
                              const PathNode* tail, unsigned int realDepth,
                              const vec2* start, const vec2* goal) {
    // Follow the path until only as many nodes as we can fit into the
    // specified buffer remain.
    while (realDepth >= depth) {
        tail = &gClosedSet[tail->came_from - 1];
        --realDepth;
    }

    // Set the start node to the actual start coordinates, instead of the
    // ones of the waypoint.
    path[0].d.x = start->d.x;
    path[0].d.y = start->d.y;
    // Same for the last one.
    path[realDepth - 1].d.x = goal->d.x;
    path[realDepth - 1].d.y = goal->d.y;
    // And skip it, too.
    tail = &gClosedSet[tail->came_from - 1];

    // Push the remaining nodes' coordinates (in reverse walk order to
    // make it forward work order).
    for (int i = realDepth - 2; i > 0; --i, tail = &gClosedSet[tail->came_from - 1]) {
        vec2* waypoint = &path[i];
        waypoint->d.x = toGlobal(tail->x);
        waypoint->d.y = toGlobal(tail->y);
    }

    return realDepth;
}

/**
 * Tests if we have reached our goal and writes out result data if yes.
 * @param path the buffer to write the path to.
 * @param depth the length of the buffer, set to the used length.
 * @param length the actual length of the found path, in map space.
 * @param node the current node.
 * @param gx the goal x coordinate in A* space.
 * @param gy the goal y coordinate in A* space.
 * @param start the start coordinates in map space.
 * @param goal the goal coordinates in map space.
 * @return whether the goal was reached or not.
 */
static int isGoal(vec2* path, unsigned int* depth, float* length,
                  PathNode* node, unsigned int gx, unsigned int gy,
                  const vec2* start, const vec2* goal) {
    // Test whether node coordinates are goal coordinates.
    if (node->x == gx && node->y == gy) {
        // Prune the path based on line of sight (i.e. skip nodes that are only
        // making the path longer than necessary).
        unsigned int realDepth = prunePath(node);

        // Compute actual length of the path. We can't use the gscore because
        // we might have pruned some of the path, which might alter the distance
        // quite a bit.
        if (length) {
            *length = computeLength(node, start, goal);
        }

        // Make sure we can write something back.
        if (depth) {
            if (*depth > 1) {
                // Long enough for something path-y, write as much as we need or
                // can, whichever is less.
                *depth = writePath(path, *depth, node, realDepth, start, goal);
            } else {
                // Not enough space for a path.
                *depth = 0;
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

int AStar(const vec2* start, const vec2* goal,
          int(*passable)(float x, float y), unsigned int bounds,
          vec2* path, unsigned int* depth, float* length) {
    unsigned int gx, gy, begin_x, begin_y, end_x, end_y, neighbor_x, neighbor_y;
    int x, y;
    float gscore, fscore;
    PathNode *current, *node;

    // We need a passability method.
    if (!passable) {
        return 0;
    }

    // Check if the start and target position are valid (passable).
    if (!passable(start->v[0], start->v[1]) ||
        !passable(goal->v[0], goal->v[1])) {
        return 0;
    }

    // Remember passability check method.
    gIsPassable = passable;

    // Set grid bounds.
    gGridSize = bounds * ASTAR_GRANULARITY;

    // Ensure size of lookup table is sufficient.
    if (gGridSize > gGridCapacity) {
        gGridCapacity = gGridSize;
        BS_Delete(gClosedSetLookup);
        gClosedSetLookup = BS_New(gGridCapacity * gGridCapacity);
    }

    // Reset number of entries used in the open and closed set.
    gOpenSetCount = 0;
    gClosedSetCount = 0;

    // Clear closed set bitset representation.
    BS_Reset(gClosedSetLookup, gGridSize * gGridSize);

    // Get goal in local coordinates.
    gx = toLocal(goal->v[0]);
    gy = toLocal(goal->v[1]);

    // Initialize the first open node to the one we're starting from.
    current = newOpenNode(0);
    current->x = toLocal(start->v[0]);
    current->y = toLocal(start->v[1]);
    current->gscore = 0.0f;
    current->fscore = 0.0f;
    current->came_from = 0;
    current->steps = 1;

    // Do the actual search.
    while (gOpenSetCount > 0) {
        // Copy first (best) open entry to closed list.
        current = popOpenNodeToClosedSet();

        // Check if we're there yet.
        if (isGoal(path, depth, length, current, gx, gy, start, goal)) {
            return 1;
        }

        // Check our neighbors. Determine which neighbors we actually need to
        // check based on the direction we came from. Per default enable all.
        begin_x = clamp(current->x - 1);
        begin_y = clamp(current->y - 1);
        end_x = clamp(current->x + 1);
        end_y = clamp(current->y + 1);

        // Check if we have a direction, i.e. we came from somewhere. The
        // exception is the start node, where we'll have to check all neighbors.
        if (current->came_from) {
            // Compute the direction as an integer of {-1, 0, 1} for both axii.
            const int dx = current->x - gClosedSet[current->came_from - 1].x,
                    dy = current->y - gClosedSet[current->came_from - 1].y;

            // If we're only moving to the right we don't have to check left. If
            // we're also moving vertically, though, we need to check for an
            // obstacle (forced neighbors), which we do via the passable check.
            // This works analogous for all other movement directions.
            if (dx >= 0 && isPassable(current->x - 1, current->y)) {
                // Don't have to check to the left.
                begin_x = current->x;
            } else if (dx <= 0 && isPassable(current->x + 1, current->y)) {
                // Don't have to check to the right.
                end_x = current->x;
            }
            if (dy >= 0 && isPassable(current->x, current->y - 1)) {
                // Don't have to check up.
                begin_y = current->y;
            } else if (dy <= 0 && isPassable(current->x, current->y + 1)) {
                // Don't have to check down.
                end_y = current->y;
            }
        }

        // Now work through all our selected neighbors.
        for (neighbor_x = begin_x; neighbor_x <= end_x; ++neighbor_x) {
            for (neighbor_y = begin_y; neighbor_y <= end_y; ++neighbor_y) {
                // Skip self.
                if (neighbor_x == current->x && neighbor_y == current->y) {
                    continue;
                }

                // The actual node to be inspected.
                x = neighbor_x, y = neighbor_y;

#if ASTAR_JPS
                // Try this direction using jump point search.
                if (!jumpPointSearch(&x, &y, x - current->x, y - current->y, x, y, gx, gy)) {
                    // Failed, try next neighbor.
                    continue;
                }

                // We might have jumped to the goal, check for that.
                if (isGoal(path, depth, length, current, gx, gy, start, goal)) {
                    return 1;
                }
#else
                // If we already handled this one, skip it.
                if (BS_Test(gClosedSetLookup, y * gGridSize + x)) {
                    continue;
                }

                // Only if this block is passable and the two diagonal ones are.
                if (!isPassable(x, y) ||
                    (!isPassable(x, current->y) && !isPassable(current->x, y))) {
                    continue;
                }
#endif

                // Determine score - score to current node plus that to this
                // neighbor. This is the actual traveled distance (Euclidean
                // distance -- we do an actual computation here, because we
                // might have skipped some tiles).
                gscore = current->gscore +
                        h(x, y, current->x, current->y);

                // See if we can find that neighbor in the open set and should
                // skip it, because there already exists a better path to it.
                if (shouldSkip(x, y, gscore)) {
                    continue;
                }

                // Compute the heuristic cost for a path with this waypoint.
                // The factor in the end is used for tie breaking.
                fscore = (gscore + f(x, y, gx, gy)) * 1.001f;

                // Create new entry.
                node = newOpenNode(fscore);

                // Store position of that node. Used for "contains" checks.
                node->x = x;
                node->y = y;

                // Remember where we came from and how deep the path is.
                node->came_from = gClosedSetCount;
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
