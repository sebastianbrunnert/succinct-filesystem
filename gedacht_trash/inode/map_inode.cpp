/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "inode.hpp"

#include <unordered_map>

class MapInodeManager : public InodeManager {
private:
    std::unordered_map<size_t, Inode> inode_table;
    std::unordered_map<size_t, size_t> flouds_node_to_inode;

public:

    MapInodeManager(AllocationManager* allocation_manager) : InodeManager(allocation_manager) {}

    size_t create_inode(size_t flouds_node_id) override {
        size_t inode_number = inode_table.size() + 1;
        Inode inode;
        inode.allocation_handle = 0;
        inode.size = 0;
        inode.mode = 0;
        inode.modification_time = std::time(nullptr);
        inode.access_time = std::time(nullptr);
        inode.creation_time = std::time(nullptr);
        inode_table[inode_number] = inode;
        flouds_node_to_inode[flouds_node_id] = inode_number;
        return inode_number;
    }

    void delete_inode(size_t inode) override {
        auto it = inode_table.find(inode);
        if (it != inode_table.end()) {
            size_t flouds_node_id = flouds_node_to_inode[inode];
            flouds_node_to_inode.erase(flouds_node_id);
            inode_table.erase(it);
        }
    }

    size_t get_flouds_node_id(size_t inode) override {
        auto it = flouds_node_to_inode.find(inode);
        if (it != flouds_node_to_inode.end()) {
            return it->second;
        }
        return 0; // Return 0 if not found
    }

    Inode get_inode(size_t inode) override {
        auto it = inode_table.find(inode);
        if (it != inode_table.end()) {
            return it->second;
        }
        Inode empty_inode;
        empty_inode.allocation_handle = 0;
        empty_inode.size = 0;
        empty_inode.mode = 0;
        empty_inode.modification_time = 0;
        empty_inode.access_time = 0;
        empty_inode.creation_time = 0;
        return empty_inode;
    }

};

