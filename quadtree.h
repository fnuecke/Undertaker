/* 
 * Author: fnuecke
 *
 * Created on April 30, 2012, 1:34 AM
 */

#ifndef QUADTREE_H
#define	QUADTREE_H

#include "vmath.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /** Structure representing a QuadTree */
    typedef struct QuadTree QuadTree;

    /**
     * Allocates memory for a new QuadTree.
     * @param size the size of the tree on the x/y axis. It can only contain
     * objects that are within the [(0,0),(size,size)] bounds.
     * @return the allocated tree.
     */
    QuadTree* QT_New(unsigned int size);

    /**
     * Frees all memory occupied by a QuadTree.
     * @param tree the tree to delete.
     */
    void QT_Delete(QuadTree* tree);

    /**
     * Add an object at the specified position. If the object is already in the
     * tree it will update its position.
     * @param tree the tree to add the object to.
     * @param position the position of the object.
     * @param object
     */
    void QT_Add(QuadTree* tree, const vec2* position, float radius, const void* object);

    /**
     * Remove an object from the tree.
     * @param tree the tree to remove the object from.
     * @param object the object to remove.
     */
    void QT_Remove(QuadTree* tree, const void* object);

    /**
     * Performs a range query on the specified tree.
     * @param tree the tree to perform the query on.
     * @param position the center of the query.
     * @param radius the radius of the query.
     * @param list the list to store found objects in.
     * @param size the capacity of the result list.
     * @return the number of found elements.
     */
    int QT_Query(QuadTree* tree, const vec2* position, float radius, void** list, unsigned int size);

#ifdef	__cplusplus
}
#endif

#endif
