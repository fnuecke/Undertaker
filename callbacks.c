#include <malloc.h>

#include "callbacks.h"

struct Callbacks {
    callback* list;
    unsigned int length;
    unsigned int capacity;
};

Callbacks* CB_New(void) {
    return (Callbacks*) calloc(1, sizeof (Callbacks));
}

void CB_Delete(Callbacks* list) {
    if (list) {
        free(list->list);
        free(list);
    }
}

void CB_Add(Callbacks* list, callback method) {
    if (list && method) {
        if (list->length >= list->capacity) {
            list->capacity = list->capacity * 2 + 1;
            list->list = (callback*) realloc(list->list, list->capacity * sizeof (callback));
        }
        list->list[list->length] = method;
        ++list->length;
    }
}

void CB_Call(Callbacks* list) {
    if (list) {
        for (unsigned int i = 0; i < list->length; ++i) {
            list->list[i]();
        }
    }
}
