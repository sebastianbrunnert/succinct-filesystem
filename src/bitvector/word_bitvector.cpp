/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <vector>
#include <stdexcept>
#include "bitvector.hpp"

/**
 * Improved implementation of the BitVector interface.
 * 
 * This implemenation stores bits in a vector of machine words and implements the BitVector operations using SIMD operations on these words.
 * Performance:
 * - size, set and access: O(1)
 * - rank0, rank1, select0 and select1: O(n) (but much faster than the array-based implementation due to SIMD)
 * - insert, remove, remove_range: O(n)
 */
class WordBitVectorStrategy : public BitVector {
private:
    std::vector<size_t> words;
    std::size_t num_bits;

public:
    WordBitVectorStrategy(size_t n) : num_bits(n), words((n + sizeof(size_t) * 8 - 1) / (sizeof(size_t) * 8), 0) {};

    void set(size_t position, bool value) override {
        if (position >= num_bits) throw std::out_of_range("position out of range");
        words[position / 64] |= (1ull << (position % 64));
    }

    bool access(size_t position) const override {
        if (position >= num_bits) throw std::out_of_range("position out of range");
        return (words[position / 64] >> (position % 64)) & 1;
    }

    size_t size() const override {
        return num_bits;
    }

    size_t rank1(size_t position) const override {
        if (position >= num_bits) throw std::out_of_range("position out of range");
        size_t count = 0;
        size_t full_words = position / 64;
        size_t remaining_bits = position % 64;

        for(size_t i = 0; i < full_words; i++) {
            count += __builtin_popcountll(words[i]);
        }

        if (remaining_bits > 0) {
            count += __builtin_popcountll(words[full_words] & ((1ull << (remaining_bits + 1)) - 1));
        }

        return count;
    }

    size_t rank0(size_t position) const override {
        return position + 1 - rank1(position);
    }

    size_t select0(size_t n) const override {
        if (n == 0) throw std::out_of_range("n must be greater than zero");
        throw std::out_of_range("n exceeds number of 1-bits");
    }

    size_t select1(size_t n) const override {
        if (n == 0) throw std::out_of_range("n must be greater than zero");
        throw std::out_of_range("n exceeds number of 1-bits");
    }

    void insert(size_t position, bool value) override {

    }

    void remove(size_t position) override {

    }

    void remove_range(size_t position, size_t length) override {

    }
};

template <>
BitVector* create_bitvector<WordBitVectorStrategy>(std::size_t n) {
    return new WordBitVectorStrategy(n);
}