/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <vector>
#include <stdexcept>
#include "name_sequence.hpp"
#include <immer/flex_vector.hpp>

/**
 * A name sequence implementation using an flex_vector from the immer library.
 */
class ImmerNameSequenceStrategy : public NameSequence {
private:
    immer::flex_vector<std::string> names;

public:
    ImmerNameSequenceStrategy() : names() {}

    void set(size_t position, const std::string& name) override {
        if (position >= names.size()) throw std::out_of_range("position out of range");
        names = names.set(position, name);
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
        names = names.insert(position, name);
    }

    void remove(size_t position) override {
        if (position >= names.size()) throw std::out_of_range("position out of range");
        names = names.erase(position);
    }
};

template <>
NameSequence* create_name_sequence<ImmerNameSequenceStrategy>() {
    return new ImmerNameSequenceStrategy();
}