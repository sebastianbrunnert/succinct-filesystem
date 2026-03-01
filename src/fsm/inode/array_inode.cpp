/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "inode.hpp"
#include <vector>
#include <cstring>

class ArrayInodeManagerStrategy : public InodeManager {
private:
    std::vector<Inode> inodes;
public:
    ArrayInodeManagerStrategy(AllocationManager* allocation_manager) : InodeManager(allocation_manager) {}

    Inode* get_inode(size_t inode) override {
        return &inodes[inode];
    }

    Inode* insert_inode(size_t inode) override {
        inodes.insert(inodes.begin() + inode, Inode{});
        return &inodes[inode];
    }

    void remove_inode(size_t inode) override {
        inodes.erase(inodes.begin() + inode);
    }

    void serialize(char* buffer, size_t* offset) override {
        size_t num_inodes = inodes.size();
        std::memcpy(buffer + *offset, &num_inodes, sizeof(size_t));
        *offset += sizeof(size_t);
        for (const Inode& inode : inodes) {
            std::memcpy(buffer + *offset, &inode, sizeof(Inode));
            *offset += sizeof(Inode);
        }
    }

    void deserialize(const char* buffer, size_t* offset) override {
        size_t num_inodes;
        std::memcpy(&num_inodes, buffer + *offset, sizeof(size_t));
        *offset += sizeof(size_t);
        inodes.resize(num_inodes);
        for (size_t i = 0; i < num_inodes; i++) {
            std::memcpy(&inodes[i], buffer + *offset, sizeof(Inode));
            *offset += sizeof(Inode);
        }
    }

    size_t get_serialized_size() override {
        return sizeof(size_t) + inodes.size() * sizeof(Inode);
    }
};

template <>
InodeManager* create_inode_manager<ArrayInodeManagerStrategy>(AllocationManager* allocation_manager) {
    return new ArrayInodeManagerStrategy(allocation_manager);
}