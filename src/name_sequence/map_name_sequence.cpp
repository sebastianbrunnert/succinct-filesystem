/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "name_sequence.hpp"
#include <map>

/**
 * A name sequence implementation using a map to store names at specific positions.
 * This allows O(log n) access and insertion.
 */
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

    size_t get_serialized_size() override {
        size_t total_size = sizeof(size_t);
        for (const auto& pair : names) {
            total_size += sizeof(size_t);
            total_size += sizeof(size_t);
            total_size += pair.second.size();
        }
        return total_size;
    }

    void serialize(char* buffer, size_t* offset) override {
        size_t num_names = names.size();
        memcpy(buffer + *offset, &num_names, sizeof(size_t));
        *offset += sizeof(size_t);
        for (const auto& pair : names) {
            size_t position = pair.first;
            const std::string& name = pair.second;
            size_t name_length = name.size();
            memcpy(buffer + *offset, &position, sizeof(size_t));
            *offset += sizeof(size_t);
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
        names.clear();
        for (size_t i = 0; i < num_names; i++) {
            size_t position;
            memcpy(&position, buffer + *offset, sizeof(size_t));
            *offset += sizeof(size_t);
            size_t name_length;
            memcpy(&name_length, buffer + *offset, sizeof(size_t));
            *offset += sizeof(size_t);
            std::string name(buffer + *offset, name_length);
            *offset += name_length;
            names[position] = name;
        }
    }
};

template <>
NameSequence* create_name_sequence<MapNameSequenceStrategy>() {
    return new MapNameSequenceStrategy();
}