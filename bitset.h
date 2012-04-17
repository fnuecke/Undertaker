/* 
 * File:   bitset.h
 * Author: fnuecke
 *
 * Created on April 17, 2012, 3:36 PM
 */

#ifndef BITSET_H
#define	BITSET_H

#ifdef	__cplusplus
extern "C" {
#endif

/** Allocate a new bitset that can represent the specified number of bits */
char* BS_alloc(unsigned int size);

/** Free a bitset */
void BS_free(char* bitset);

/** Set the bit at the specified index in the specified bitset */
void BS_set(char* bitset, unsigned int idx);

/** Unset the bit at the specified index in the specified bitset */
void BS_unset(char* bitset, unsigned int idx);

/** Test if the bit at the specified index is set in the specified bitset */
int BS_test(const char* bitset, unsigned int idx);

/** Reset a bitset (set all bits to 0), given the size of the bitset */
void BS_reset(char* bitset, unsigned int size);

#ifdef	__cplusplus
}
#endif

#endif	/* BITSET_H */

