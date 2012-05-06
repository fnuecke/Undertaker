#include "quadtree.h"

#include <malloc.h>

typedef struct QuadEntry {
    void* object;
    vec2 position;
    float radius;
} QuadEntry;

enum {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

typedef struct QuadNode QuadNode;

struct QuadNode {
    QuadEntry** entries;
    unsigned int entryCapacity;
    unsigned int entryCount;
    QuadNode * childNodes[4];
};

static void deleteNode(QuadNode* node) {
    if (!node) {
        return;
    }

    for (unsigned int i = 0; i < node->entryCount; ++i) {
        free(node->entries[i]);
    }
    free(node->entries);
    node->entryCapacity = 0;
    node->entryCount = 0;

    deleteNode(node->childNodes[TopLeft]);
    node->childNodes[TopLeft] = 0;
    deleteNode(node->childNodes[TopRight]);
    node->childNodes[TopRight] = 0;
    deleteNode(node->childNodes[BottomLeft]);
    node->childNodes[BottomLeft] = 0;
    deleteNode(node->childNodes[BottomRight]);
    node->childNodes[BottomRight] = 0;

    free(node);
}

struct QuadTree {
    QuadNode* root;
    float size;
};

QuadTree* QT_New(unsigned int size) {
    QuadTree* tree = calloc(1, sizeof (QuadTree));
    tree->root = NULL;
    tree->size = size;
    return tree;
}

void QT_Delete(QuadTree* tree) {
    if (tree) {
        deleteNode(tree->root);
        free(tree);
    }
}

void QT_Add(QuadTree* tree, const vec2* position, float radius, const void* object) {

}

void QT_Remove(QuadTree* tree, const void* object) {

}

int QT_Query(QuadTree* tree, const vec2* position, float radius, void** list, unsigned int size) {
    return 0;
}
