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

    /**
     * Data representation for a bit set.
     */
    typedef char* BitSet;

    /**
     * Allocate a new bit set that can represent the specified number of bits.
     * @param size the size of the bit set, in bits.
     * @return a new bit set with the specified capacity.
     */
    BitSet BS_New(unsigned int size);

    /**
     * Free the memory occupied by the specified bit set.
     * @param bitset the bit set to free.
     */
    void BS_Delete(BitSet bitset);

    /**
     * Set the bit at the specified index in the specified bit set.
     * @param bitset the bit set to set the bit in.
     * @param idx the index at which to set a bit (i.e. the number of the bit).
     */
    void BS_Set(BitSet bitset, unsigned int idx);

    /**
     * Unset the bit at the specified index in the specified bit set.
     * @param bitset the bit set to unset the bit in.
     * @param idx the index at which to unset a bit (i.e. the number of the bit).
     */
    void BS_Unset(BitSet bitset, unsigned int idx);

    /**
     * Test if the bit at the specified index is set in the specified bit set.
     * @param bitset the bit set to test the bit in.
     * @return whether the bit is set (1) or not (0).
     */
    int BS_Test(const BitSet bitset, unsigned int idx);

    /**
     * Reset a bit set (set all bits to 0), given the size of the bit set.
     * @param bitset the bit set to reset.
     * @param size the size of the bit set.
     */
    void BS_Reset(BitSet bitset, unsigned int size);

#ifdef	__cplusplus
}
#endif

#endif	/* BITSET_H */

