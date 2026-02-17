/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <vector>
#include <stdexcept>
#include "name_sequence.hpp"

/**
 * Plain array-based implementation of the NameSequence interface.
 * 
 * This implementation stores names in a std::vector<std::string> and provides straightforward implementations of all NameSequence operations.
 */
class ArrayNameSequenceStrategy : public NameSequence {
private:
    std::vector<std::string> names;

public:
    ArrayNameSequenceStrategy() : names() {}

    void set(size_t position, const std::string& name) override {
        if (position >= names.size()) throw std::out_of_range("position out of range");
        names[position] = name;
    }

    std::string access(size_t position) const override {
        if (position >= names.size()) throw std::out_of_range("position out of range");
        return names[position];
    }

    size_t size() const override {
        return names.size();
    }

    void insert(size_t position, const std::string& name) override {
        if (position > names.size()) throw std::out_of_range("position out of range");
        names.insert(names.begin() + position, name);
    }

    void remove(size_t position) override {
        if (position >= names.size()) throw std::out_of_range("position out of range");
        names.erase(names.begin() + position);  
    }
};

template <>
NameSequence* create_name_sequence<ArrayNameSequenceStrategy>() {
    return new ArrayNameSequenceStrategy();
}