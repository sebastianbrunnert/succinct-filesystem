/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "name_sequence.hpp"
#include <map>

class MapNameSequenceStrategy : public NameSequence {
private:
    std::map<size_t, std::string> names;

public:
    MapNameSequenceStrategy() : names() {}

    void set(size_t position, const std::string& name) override {
        if (position >= names.size()) throw std::out_of_range("position out of range");
        names[position] = name;
    }

    std::string access(size_t position) const override {
        auto it = names.find(position);
        if (it == names.end()) throw std::out_of_range("position out of range");
        return it->second;
    }

    size_t size() const override {
        return names.size();
    }

    void insert(size_t position, const std::string& name) override {
        if (position > names.size()) throw std::out_of_range("position out of range");
        for (size_t i = names.size(); i > position; i--) {
            auto it = names.find(i - 1);
            if (it != names.end()) {
                names[i] = it->second;
            } else {
                names.erase(i);
            }
        }
        names[position] = name;
    }

    void remove(size_t position) override {
        if (position >= names.size()) throw std::out_of_range("position out of range");
        for (size_t i = position; i < names.size() - 1; i++) {
            auto it = names.find(i + 1);
            if (it != names.end()) {
                names[i] = it->second;
            } else {
                names.erase(i);
            }
        }
        names.erase(names.size() - 1);
    }
};

template <>
NameSequence* create_name_sequence<MapNameSequenceStrategy>() {
    return new MapNameSequenceStrategy();
}