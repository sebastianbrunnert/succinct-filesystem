/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <cstddef>
#include <vector>

class DeltaStabilization {
private:
    uint64_t version = 1; // Current version, incremented on each modification
    
    struct DeltaOperation {
        bool is_insert;
        size_t position;
        uint64_t version;
    };
    
    std::vector<DeltaOperation> operations;

public:

    void record_insert(size_t inode) {
        version++;
        operations.push_back({true, inode, version});
    }

    void record_remove(size_t inode) {
        version++;
        operations.push_back({false, inode, version});
    }

    size_t stable_inode_to_flouds_inode(uint64_t stable_inode) {
        size_t flouds_inode = stable_inode & 0xFFFFFFFFFFFF; // Extract the lower 48 bits for the FLOUDS inode
        uint64_t op_version = stable_inode >> 48; // Extract the upper 16 bits for the version
        
        // Apply all operations that occurred after the given version
        for (const auto& op : operations) {
            if (op.version > op_version) {
                if (op.is_insert && op.position <= flouds_inode) {
                    flouds_inode++;
                } else if (!op.is_insert && op.position < flouds_inode) {
                    flouds_inode--;
                }
            }
        }
        
        return flouds_inode;
    }

    uint64_t flouds_inode_to_stable_inode(size_t flouds_inode) {
        return (version << 48) | flouds_inode;
    }
};