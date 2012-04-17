#include <malloc.h>
#include <memory.h>

#include "bitset.h"

char* BS_alloc(unsigned int size) {
    return calloc(size / 8 + 1, sizeof (char));
}

void BS_free(char* bitset) {
    free(bitset);
}

void BS_set(char* bitset, unsigned int idx) {
    bitset[idx >> 3] |= (1 << (idx & 7));
}

void BS_unset(char* bitset, unsigned int idx) {
    bitset[idx >> 3] &= ~(1 << (idx & 7));
}

int BS_test(const char* bitset, unsigned int idx) {
    return (bitset[idx >> 3] & (1 << (idx & 7))) != 0;
}

void BS_reset(char* bitset, unsigned int size) {
    memset(bitset, 0, size / 8 + 1);
}