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
 * This implemenation stores bits in a vector of machine words and implements the BitVector operations using SIMD operations on these words.
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
        size_t full_words = (position + 1) / 64;
        size_t remaining_bits = (position + 1) % 64;

        for(size_t i = 0; i < full_words; i++) {
            count += __builtin_popcountll(words[i]);
        }

        if (remaining_bits > 0) {
            count += __builtin_popcountll(words[full_words] & ((1ull << remaining_bits) - 1));
        }

        return count;
    }

    size_t rank0(size_t position) const override {
        return position + 1 - rank1(position);
    }

    size_t select0(size_t n) const override {
        if (n == 0) throw std::out_of_range("n must be greater than zero");

        size_t count = 0;
        for(size_t i = 0; i < words.size(); i++) {
            size_t bits_in_word = 64;
            size_t masked_word = words[i];
            if(i == words.size()-1) {
                bits_in_word = num_bits % 64;
                if(bits_in_word == 0) bits_in_word = 64;
                if (bits_in_word < 64) {
                    masked_word &= (1ull << bits_in_word) - 1;
                }
            }

            size_t word_count = bits_in_word - __builtin_popcountll(masked_word);

            if (count + word_count >= n) {
                size_t remaining = n - count;
                for(size_t j = 0; j < bits_in_word; j++) {
                    if (((masked_word >> j) & 1) == 0) {
                        remaining--;
                        if (remaining == 0) {
                            return i * 64 + j;
                        }
                    }
                }
            }

            count += word_count;
        }

        throw std::out_of_range("n exceeds number of 1-bits");
    }

    size_t select1(size_t n) const override {
        if (n == 0) throw std::out_of_range("n must be greater than zero");

        size_t count = 0;
        for(size_t i = 0; i < words.size(); i++) {
            size_t bits_in_word = 64;
            size_t masked_word = words[i];
            if(i == words.size()-1) {
                bits_in_word = num_bits % 64;
                if(bits_in_word == 0) bits_in_word = 64;
                if (bits_in_word < 64) {
                    masked_word &= (1ull << bits_in_word) - 1;
                }
            }

            size_t word_count = __builtin_popcountll(masked_word);

            if (count + word_count >= n) {
                size_t remaining = n - count;
                for(size_t j = 0; j < bits_in_word; j++) {
                    if (((masked_word >> j) & 1) == 1) {
                        remaining--;
                        if (remaining == 0) {
                            return i * 64 + j;
                        }
                    }
                }
            }

            count += word_count;
        }

        throw std::out_of_range("n exceeds number of 1-bits");
    }

    void insert(size_t position, bool value) override {
        if (position > num_bits) throw std::out_of_range("position out of range");

        num_bits++;
        if (num_bits % 64 == 1) {
            words.push_back(0);
        }
        
        size_t word_index = position / 64;
        size_t bit_index = position % 64;

        for(size_t i = words.size() - 1; i > word_index; i--) {
            words[i] = (words[i] << 1) | (words[i-1] >> 63);
        }
        
        size_t mask = (1ull << bit_index) - 1;
        size_t high_bits = words[word_index] & ~mask;
        size_t low_bits = words[word_index] & mask;        
        words[word_index] = low_bits | (high_bits << 1);
        
        if (value) {
            words[word_index] |= (1ull << bit_index);
        }
    }

    void remove(size_t position) override {
        if (position >= num_bits) throw std::out_of_range("position out of range");

        size_t word_index = position / 64;
        size_t bit_index = position % 64;

        size_t mask = (1ull << bit_index) - 1;
        words[word_index] = (words[word_index] & mask) | ((words[word_index] >> 1) & ~mask);
        for (size_t i = word_index; i < words.size() - 1; i++) {
            words[i] |= (words[i + 1] << 63);
            words[i + 1] >>= 1;
        }

        num_bits--;
        if (num_bits % 64 == 0 && num_bits > 0) {
            words.pop_back();
        }
    }

    void serialize(char* buffer, size_t* offset) override {
        size_t size = this->size();
        memcpy(buffer + *offset, &size, sizeof(size_t));
        *offset += sizeof(size_t);
        size_t num_words = (size + 63) / 64;  // Match deserialize logic
        for (size_t i = 0; i < num_words; i++) {
            memcpy(buffer + *offset, &words[i], sizeof(size_t));
            *offset += sizeof(size_t);
        }
    }

    void deserialize(const char* buffer, size_t* offset) override {
        size_t size;
        memcpy(&size, buffer + *offset, sizeof(size_t));
        *offset += sizeof(size_t);
        num_bits = size;
        words.resize((size + 63) / 64);
        for (size_t i = 0; i < words.size(); i++) {
            memcpy(&words[i], buffer + *offset, sizeof(size_t));
            *offset += sizeof(size_t);
        }
    }

    size_t get_serialized_size() override {
        size_t num_words = (num_bits + 63) / 64;  // Match serialize/deserialize logic
        return sizeof(size_t) + num_words * sizeof(size_t);
    }
};

template <>
BitVector* create_bitvector<WordBitVectorStrategy>(std::size_t n) {
    return new WordBitVectorStrategy(n);
}