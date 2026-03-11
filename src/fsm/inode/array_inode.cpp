/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "inode.hpp"
#include <vector>
#include <cstring>
#include <sys/stat.h>

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
            // Write each inode manually (because folders dont need allocation_handle and size)
            std::memcpy(buffer + *offset, &inode.mode, sizeof(uint32_t));
            *offset += sizeof(uint32_t);
            std::memcpy(buffer + *offset, &inode.modification_time, sizeof(time_t));
            *offset += sizeof(time_t);
            std::memcpy(buffer + *offset, &inode.access_time, sizeof(time_t));
            *offset += sizeof(time_t);
            std::memcpy(buffer + *offset, &inode.creation_time, sizeof(time_t));
            *offset += sizeof(time_t);
            if (S_ISREG(inode.mode)) {
                std::memcpy(buffer + *offset, &inode.allocation_handle, sizeof(size_t));
                *offset += sizeof(size_t);
                std::memcpy(buffer + *offset, &inode.size, sizeof(size_t));
                *offset += sizeof(size_t);
            }
        }
    }

    void deserialize(const char* buffer, size_t* offset) override {
        size_t num_inodes;
        std::memcpy(&num_inodes, buffer + *offset, sizeof(size_t));
        *offset += sizeof(size_t);
        inodes.resize(num_inodes);
        for (size_t i = 0; i < num_inodes; i++) {
            Inode& inode = inodes[i];
            std::memcpy(&inode.mode, buffer + *offset, sizeof(uint32_t));
            *offset += sizeof(uint32_t);
            std::memcpy(&inode.modification_time, buffer + *offset, sizeof(time_t));
            *offset += sizeof(time_t);
            std::memcpy(&inode.access_time, buffer + *offset, sizeof(time_t));
            *offset += sizeof(time_t);
            std::memcpy(&inode.creation_time, buffer + *offset, sizeof(time_t));
            *offset += sizeof(time_t);
            if (S_ISREG(inode.mode)) {
                std::memcpy(&inode.allocation_handle, buffer + *offset, sizeof(size_t));
                *offset += sizeof(size_t);
                std::memcpy(&inode.size, buffer + *offset, sizeof(size_t));
                *offset += sizeof(size_t);
            }
        }
    }

    size_t get_serialized_size() override {
        size_t size = sizeof(size_t);
        for (const Inode& inode : inodes) {
            size += sizeof(uint32_t) + 3 * sizeof(time_t);
            if (S_ISREG(inode.mode)) {
                size += 2 * sizeof(size_t);
            }
        }
        return size;
    }
};

template <>
InodeManager* create_inode_manager<ArrayInodeManagerStrategy>(AllocationManager* allocation_manager) {
    return new ArrayInodeManagerStrategy(allocation_manager);
}