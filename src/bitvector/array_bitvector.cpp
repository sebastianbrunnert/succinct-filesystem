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
 * Plain array-based implementation of the BitVector interface.
 * 
 * This implementation stores bits in a std::vector<bool> and provides straightforward implementations of all BitVector operations.
 * It is not optimized for space or speed, but is primrily intended for testing.
 */
class ArrayBitVectorStrategy : public BitVector {
private:
    std::vector<bool> bits;

public:
    ArrayBitVectorStrategy(size_t n) : bits(n, false) {}

    void set(size_t position, bool value) override {
        if (position >= bits.size()) throw std::out_of_range("position out of range");
        bits[position] = value;
    }

    bool access(size_t position) const override {
        if (position >= bits.size()) throw std::out_of_range("position out of range");
        return bits[position];
    }

    size_t size() const override {
        return bits.size();
    }

    size_t rank1(size_t position) const override {
        if (position >= bits.size()) throw std::out_of_range("position out of range");
        size_t count = 0;
        for (size_t i = 0; i <= position; i++) {
            if (bits[i]) count++;
        }
        return count;
    }

    size_t rank0(size_t position) const override {
        return position + 1 - rank1(position);
    }

    size_t select1(size_t n) const override {
        if (n == 0) throw std::out_of_range("n must be greater than zero");
        size_t count = 0;
        for (size_t i = 0; i < bits.size(); i++) {
            if (bits[i]) ++count;
            if (count == n) return i;
        }
        throw std::out_of_range("n exceeds number of 1-bits");
    }

    size_t select0(size_t n) const override {
        if (n == 0) throw std::out_of_range("n must be greater than zero");
        size_t count = 0;
        for (size_t i = 0; i < bits.size(); i++) {
            if (!bits[i]) ++count;
            if (count == n) return i;
        }
        throw std::out_of_range("n exceeds number of 0-bits");
    }

    void insert(size_t position, bool value) override {
        if (position > bits.size()) throw std::out_of_range("position out of range");
        bits.insert(bits.begin() + position, value);
    }

    void remove(size_t position) override {
        if (position >= bits.size()) throw std::out_of_range("position out of range");
        bits.erase(bits.begin() + position);
    }

    void serialize(char* buffer, size_t* offset) override {
        size_t size = bits.size();
        memcpy(buffer + *offset, &size, sizeof(size_t));
        *offset += sizeof(size_t);
        for (size_t i = 0; i < bits.size(); i++) {
            buffer[*offset + i / 8] |= (bits[i] ? 1 : 0) << (7 - i % 8);
        }
        *offset += (bits.size() + 7) / 8;
    }

    void deserialize(const char* buffer, size_t* offset) override {
        size_t size;
        memcpy(&size, buffer + *offset, sizeof(size_t));
        *offset += sizeof(size_t);
        bits.resize(size);
        for (size_t i = 0; i < size; i++) {
            bits[i] = (buffer[*offset + i / 8] >> (7 - i % 8)) & 1;
        }
        *offset += (size + 7) / 8;
    }

    size_t get_serialized_size() override {
        return sizeof(size_t) + (bits.size() + 7) / 8;
    }
};

template <>
BitVector* create_bitvector<ArrayBitVectorStrategy>(std::size_t n) {
    return new ArrayBitVectorStrategy(n);
}