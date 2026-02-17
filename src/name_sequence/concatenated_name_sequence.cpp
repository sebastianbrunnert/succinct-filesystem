/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "name_sequence.hpp"
#include "../bitvector/bitvector.hpp"

class ConcatenatedNameSequenceStrategy : public NameSequence {
private:
    std::string concatenated_names;
    BitVector* boundaries;
public:
    ConcatenatedNameSequenceStrategy() {
        concatenated_names = "";
        boundaries = create_bitvector<WordBitVectorStrategy>(0);
    }

    ~ConcatenatedNameSequenceStrategy() {
        delete boundaries;
    }

    void set(size_t position, const std::string& name) override {
        if (position >= size()) throw std::out_of_range("position out of range");
        remove(position);
        insert(position, name);
    }

    std::string access(size_t position) const override {
        size_t start = boundaries->select1(position + 1);
        size_t end;
        if (position == size() - 1) {
            end = concatenated_names.size();
        } else {
            end = boundaries->select1(position + 2);
        }
        return concatenated_names.substr(start, end - start);
    }

    size_t size() const override {
        if (boundaries->size() == 0) return 0;
        return boundaries->rank1(boundaries->size() - 1);
    }

    void insert(size_t position, const std::string& name) override {
        if (position > size()) throw std::out_of_range("position out of range");
        size_t char_pos;
        if (position == 0) {
            char_pos = 0;
        } else if (position == size()) {
            char_pos = concatenated_names.size();
        } else {
            char_pos = boundaries->select1(position + 1);
        }
        concatenated_names.insert(char_pos, name);
        for (size_t i = 0; i < name.length(); i++) {
            boundaries->insert(char_pos + i, i == 0);
        }    
    }

    void remove(size_t position) override {
        if (position >= size()) throw std::out_of_range("position out of range");
        size_t start = boundaries->select1(position + 1);
        size_t end;
        if (position == size() - 1) {
            end = concatenated_names.size();
        } else {
            end = boundaries->select1(position + 2);
        }
        size_t length = end - start;
        concatenated_names.erase(start, length);
        for (size_t i = 0; i < length; i++) {
            boundaries->remove(start);
        }
    }
};

template <>
NameSequence* create_name_sequence<ConcatenatedNameSequenceStrategy>() {
    return new ConcatenatedNameSequenceStrategy();
}