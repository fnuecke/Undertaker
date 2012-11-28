#include <assert.h>
#include <malloc.h>
#include <memory.h>

#include "bitset.h"

BitSet BS_New(unsigned int size) {
    return calloc(size / 8 + 1, sizeof (char));
}

void BS_Delete(BitSet bitset) {
    if (bitset) {
        free(bitset);
    }
}

void BS_Set(BitSet bitset, unsigned int idx) {
    bitset[idx >> 3] |= (1 << (idx & 7));
}

void BS_Unset(BitSet bitset, unsigned int idx) {
    bitset[idx >> 3] &= ~(1 << (idx & 7));
}

int BS_Test(const BitSet bitset, unsigned int idx) {
    return (bitset[idx >> 3] & (1 << (idx & 7))) != 0;
}

void BS_Reset(BitSet bitset, unsigned int size) {
    memset(bitset, 0, size / 8 + 1);
}
