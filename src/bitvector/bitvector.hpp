/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <cstddef>
#include <stdexcept>
#include <iostream>

/**
 * This class represents a dynamic 0-based bit sequence that can grow in size as needed.
 * This is a common data structure used for FLOUDS.
 */
class BitVector {
public:
    /**
     * Virtual destructor
     */
    virtual ~BitVector() = default;

    /**
     * Sets the bit at the specified position to the given value.
     * 
     * @param position The 0-based position of the bit vector to set.
     * @param value The value to set the bit to.
     * @throws std::out_of_range if the position exceeds the size of the bit vector.
     */
    virtual void set(size_t position, bool value) = 0;

    /**
     * Accesses the value of the bit at the specified position.
     * 
     * @param position The 0-based position of the bit vector to get.
     * @return The value of the bit at the specified position.
     * @throws std::out_of_range if the position exceeds the size of the bit vector.
     */
    virtual bool access(size_t position) const = 0;

    /**
     * Gets the current size of the bit vector.
     * 
     * @return The number of bits in the bit vector.
     */
    virtual size_t size() const = 0;

    /**
     * Gets the number of 0-bits in the bit vector up to and including the specified position.
     * 
     * @param position The position up to which to count the number of 0-bits.
     * @return The number of 0-bits in the bit vector up to and including the specified position.
     * @throws std::out_of_range if the position exceeds the size of the bit vector.
     */
    virtual size_t rank0(size_t position) const = 0;

    /**
     * Gets the number of 1-bits in the bit vector up to and including the specified position.
     * 
     * @param position The position up to which to count the number of 1-bits.
     * @return The number of 1-bits in the bit vector up to and including the specified position.
     * @throws std::out_of_range if the position exceeds the size of the bit vector.
     */
    virtual size_t rank1(size_t position) const = 0;

    /**
     * Gets the position of the n-th 0-bit in the bit vector.
     * 
     * @param n
     * @return The position of the nth 0-bit in the bit vector.
     * @throws std::out_of_range if n is zero or exceeds the number of 0-bits in the bit vector.
     */
    virtual size_t select0(size_t n) const = 0;

    /**
     * Gets the position of the n-th 1-bit in the bit vector.
     * 
     * @param n
     * @return The position of the nth 1-bit in the bit vector.
     * @throws std::out_of_range if n is zero or exceeds the number of 1-bits in the bit vector.
     */    
    virtual size_t select1(size_t n) const = 0;

    /**
     * Inserts a bit with the given value at the specified position, shifting all following bits to the right.
     * 
     * @param position The 0-based position at which to insert the new bit.
     * @param value The value of the bit to insert.
     * @throws std::out_of_range if the position exceeds the size of the bit vector plus one.
     */
    virtual void insert(size_t position, bool value) = 0;

    /**
     * Removes the bit at the specified position, shifting all following bits to the left.
     * 
     * @param position The 0-based position of the bit to remove.
     * @throws std::out_of_range if the position exceeds the size of the bit vector.
     */
    virtual void remove(size_t position) = 0;

    /**
     * Helper function to see the bits for debugging.
     */
    friend std::ostream& operator<<(std::ostream& os, const BitVector& bv) {
        for (size_t i = 0; i < bv.size(); i++) {
            os << (bv.access(i) ? '1' : '0');
        }
        return os;
    }

};

// Factory function to create a BitVector instance based on a specified strategy
template <typename BitVectorStrategy> BitVector* create_bitvector(size_t n);

// Different strategies for implementing the interface
class ArrayBitVectorStrategy;
template <> BitVector* create_bitvector<ArrayBitVectorStrategy>(size_t n);

class WordBitVectorStrategy;
template <> BitVector* create_bitvector<WordBitVectorStrategy>(size_t n);

class SaskeliBitVectorStrategy;
template <> BitVector* create_bitvector<SaskeliBitVectorStrategy>(size_t n);

class AdaptiveDynamicBitVectorStrategy;
template <> BitVector* create_bitvector<AdaptiveDynamicBitVectorStrategy>(size_t n);