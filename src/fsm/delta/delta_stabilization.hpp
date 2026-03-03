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

    struct DeltaOperation {
        bool is_insert;
        size_t inode; // FLOUDS numbers are 0-based, FUSE inodes are 1-based, so this is the FLOUDS inode number
    };

    std::vector<DeltaOperation> operations;


public:

    void record_insert(size_t inode) {
        operations.push_back({true, inode});
        compact_if_needed();
    }

    void record_remove(size_t inode) {
        operations.push_back({false, inode});
        compact_if_needed();
    }
    
private:
    void compact_if_needed() {
        // Compact every 100 operations to prevent unbounded growth
        if (operations.size() >= 100) {
            operations.clear();
        }
    }

public:

    size_t stable_inode_to_flouds_inode(size_t stable_inode) {
        size_t flouds_inode = stable_inode - 1;

        for (const auto& op : operations) {
            if (op.is_insert && op.inode <= flouds_inode) {
                flouds_inode++;  // Insert shifts positions RIGHT (up)
            } else if (!op.is_insert && op.inode < flouds_inode) {
                flouds_inode--;  // Delete shifts positions LEFT (down)
            }
        }

        return flouds_inode;
    }

    size_t flouds_inode_to_stable_inode(size_t flouds_inode) {
        size_t stable_inode = flouds_inode;

        for (const auto& op : operations) {
            if (op.is_insert && op.inode <= flouds_inode) {
                stable_inode--;  // Reverse: subtract inserts that happened
            } else if (!op.is_insert && op.inode < flouds_inode) {
                stable_inode++;  // Reverse: add back deleted positions
            }
        }

        return stable_inode + 1;
    }

};