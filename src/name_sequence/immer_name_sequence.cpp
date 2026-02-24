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

    size_t get_serialized_size() override {
        size_t total_size = sizeof(size_t);
        for (const std::string& name : names) {
            total_size += sizeof(size_t);
            total_size += name.size();
        }
        return total_size;
    }

    void serialize(char* buffer, size_t* offset) override {
        size_t num_names = names.size();
        memcpy(buffer + *offset, &num_names, sizeof(size_t));
        *offset += sizeof(size_t);
        for (const std::string& name : names) {
            size_t name_length = name.size();
            memcpy(buffer + *offset, &name_length, sizeof(size_t));
            *offset += sizeof(size_t);
            memcpy(buffer + *offset, name.data(), name_length);
            *offset += name_length;
        }
    }

    void deserialize(const char* buffer, size_t* offset) override {
        size_t num_names;
        memcpy(&num_names, buffer + *offset, sizeof(size_t));
        *offset += sizeof(size_t);
        names = immer::flex_vector<std::string>();
        for (size_t i = 0; i < num_names; i++) {
            size_t name_length;
            memcpy(&name_length, buffer + *offset, sizeof(size_t));
            *offset += sizeof(size_t);
            std::string name(buffer + *offset, name_length);
            *offset += name_length;
            names = names.push_back(name);
        }
    }
};

template <>
NameSequence* create_name_sequence<ImmerNameSequenceStrategy>() {
    return new ImmerNameSequenceStrategy();
}