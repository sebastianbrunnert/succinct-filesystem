/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "../bitvector/bitvector.hpp"
#include <vector>
#include <cstdint>

/**
 * This class implements a wavelet tree for an alphabet of size 4 using three bit vectors.
 */
template <typename BitVectorStrategy>
class TwoBitWaveletTree {
private:
    // Root bit vector splits symbols 0-1 and 2-3
    BitVector* root_bv;
    // Left bit vector for symbols 0 and 1
    BitVector* left_bv;
    // Right bit vector for symbols 2 and 3
    BitVector* right_bv;

public:
    /**
     * Constructs a TwoBitWaveletTree with the given bit vectors.
     * 
     * @param root The bit vector representing the root of the wavelet tree. Size must be equal to the sum of the sizes of the left and right bit vectors.
     * @param left The bit vector representing the left child of the wavelet tree.
     * @param right The bit vector representing the right child of the wavelet tree.
     */
    TwoBitWaveletTree(BitVector* root, BitVector* left, BitVector* right)
        : root_bv(root), left_bv(left), right_bv(right) {}

    
    /**
     * Destructor to clean up the bit vectors.
     */
    ~TwoBitWaveletTree() {
        delete root_bv;
        delete left_bv;
        delete right_bv;
    }

    /**
     * Sets the symbol at the specified position to the given value.
     * 
     * @param position The 0-based position in the wavelet tree to set.
     * @param symbol The symbol to set at the specified position. Must be in the range [0, 3].
     * @throws std::out_of_range if the position exceeds the size of the wavelet tree.
     */
    void set(size_t position, uint8_t symbol) {
        if (position >= size()) throw std::out_of_range("position out of range");
        root_bv->set(position, symbol >= 2);
        if (symbol < 2) {
            left_bv->set(position, symbol == 1);
        } else {
            right_bv->set(position, symbol == 3);
        }
    }

    /**
     * Accesses the symbol at the specified position.
     * 
     * @param position The 0-based position in the wavelet tree to access.
     * @return The symbol at the specified position.
     * @throws std::out_of_range if the position exceeds the size of the wavelet tree.
     */
    uint8_t access(size_t position) const {
        if (position >= size()) throw std::out_of_range("position out of range");
        bool root_bit = root_bv->access(position);
        if (!root_bit) {
            return left_bv->access(root_bv->rank0(position) - 1) ? 1 : 0;
        } else {
            return right_bv->access(root_bv->rank1(position) - 1) ? 3 : 2;
        }
    }

    /**
     * Gets the current size of the wavelet tree.
     * 
     * @return The number of symbols in the wavelet tree.
     */
    size_t size() const {
        return root_bv->size();
    }

    /**
     * Gets the number of occurrences of the specified symbol in the wavelet tree up to and including the specified position.
     * 
     * @param position The position up to which to count the occurrences of the symbol.
     * @param symbol The symbol for which to count the occurrences.
     * @return The number of the specified symbol in the wavelet tree up to and including the specified position.
     * @throws std::out_of_range if the position exceeds the size of the wavelet tree.
     */
    size_t rank(size_t position, uint8_t symbol) const {
        if (position >= size()) throw std::out_of_range("position out of range");
        bool root_bit = symbol >= 2;
        if (!root_bit) {
            if (symbol == 0) {
                return root_bv->rank0(position) - left_bv->rank1(root_bv->rank0(position) - 1);
            } else {
                return root_bv->rank0(position) - left_bv->rank0(root_bv->rank0(position) - 1);
            }
        } else {
            if (symbol == 2) {
                return root_bv->rank1(position) - right_bv->rank1(root_bv->rank1(position) - 1);
            } else {
                return root_bv->rank1(position) - right_bv->rank0(root_bv->rank1(position) - 1);
            }
        }
    }

    /**
     * Gets the position of the n-th occurrence of the specified symbol in the wavelet tree.
     * 
     * @param n The occurrence number of the symbol to find.
     * @param symbol The symbol for which to find the n-th occurrence.
     * @return The position of the n-th occurrence of the specified symbol in the wavelet tree.
     * @throws std::out_of_range if n is zero or exceeds the number of occurrences of the symbol in the wavelet tree.
     */
    size_t select(size_t n, uint8_t symbol) const {
        if (n == 0) throw std::out_of_range("n must be greater than zero");
        bool root_bit = symbol >= 2;
        if (!root_bit) {
            if (symbol == 0) {
                size_t root_pos = root_bv->select0(n + left_bv->rank1(root_bv->size() - 1));
                return root_pos - left_bv->rank1(root_pos);
            } else {
                size_t root_pos = root_bv->select0(n + left_bv->rank0(root_bv->size() - 1));
                return root_pos - left_bv-> rank0(root_pos);
            }
        } else {
            if (symbol == 2) {
                size_t root_pos = root_bv->select1(n + right_bv->rank1(root_bv->size() - 1));
                return root_pos - right_bv->rank1(root_pos);
            } else {
                size_t root_pos = root_bv->select1(n + right_bv->rank0(root_bv->size() - 1));
                return root_pos - right_bv->rank0(root_pos);
            }
        }
    }

    /**
     * Inserts a symbol at the specified position.
     * 
     * @param position The 0-based position at which to insert the new symbol.
     * @param symbol The symbol to insert at the specified position. Must be in the range [0, 3].
     * @throws std::out_of_range if the position exceeds the size of the wavelet tree plus one.
     */
    void insert(size_t position, uint8_t symbol) {
        if (position > size()) throw std::out_of_range("position out of range");
        root_bv->insert(position, symbol >= 2);
        if (symbol < 2) {
            left_bv->insert(position, symbol == 1);
            right_bv->insert(position, false);
        } else {
            left_bv->insert(position, false);
            right_bv->insert(position, symbol == 3);
        }
    }

    /**
     * Removes the symbol at the specified position.
     * 
     * @param position The 0-based position of the symbol to remove.
     * @throws std::out_of_range if the position exceeds the size of the wavelet tree.
     */
    void remove(size_t position) {
        if (position >= size()) throw std::out_of_range("position out of range");
        bool root_bit = root_bv->access(position);
        root_bv->remove(position);
        if (!root_bit) {
            left_bv->remove(position);
            right_bv->remove(position);
        } else {
            left_bv->remove(position);
            right_bv->remove(position);
        }
    }
};

/**
 * Factory function to create a TwoBitWaveletTree instance from an array of symbols.
 * 
 * @param BitVectorStrategy The strategy to use for the BitVector implementation.
 * @param data The array of symbols to build the wavelet tree from. Each symbol must be in the range [0, 3].
 * @param n The number of symbols in the data array.
 * @return A pointer to a new TwoBitWaveletTree instance built from the given data.
 * @throws std::out_of_range if any symbol in the data array is out of range.
 */
template <typename BitVectorStrategy>
TwoBitWaveletTree<BitVectorStrategy>* create_two_bit_wavelet_tree(uint8_t data[], size_t n) {
    BitVector* root_bv = create_bitvector<BitVectorStrategy>(n);
    size_t right = 0;
    for (size_t i = 0; i < n; i++) {
        if (data[i] >= 2) {
            root_bv->set(i, true);
            right++;
        }
    }
    BitVector* left_bv = create_bitvector<BitVectorStrategy>(n - right);
    BitVector* right_bv = create_bitvector<BitVectorStrategy>(right);
    size_t left_pos = 0, right_pos = 0;
    for (size_t i = 0; i < n; i++) {
        if (data[i] < 2) {
            if (data[i] == 1) {
                left_bv->set(left_pos, true);
            }
            left_pos++;
        } else {
            if (data[i] == 3) {
                right_bv->set(right_pos, true);
            }
            right_pos++;
        }
    }

    TwoBitWaveletTree<BitVectorStrategy>* tree = new TwoBitWaveletTree<BitVectorStrategy>(root_bv, left_bv, right_bv);

    return tree;
}