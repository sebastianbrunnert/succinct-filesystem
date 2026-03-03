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
    }

    void record_remove(size_t inode) {
        operations.push_back({false, inode});
    }

    size_t stable_inode_to_flouds_inode(size_t stable_inode) {
        size_t flouds_inode = stable_inode - 1;
        for (const auto& op : operations) {
            if (op.is_insert && op.inode <= flouds_inode) {
                flouds_inode++;  // insert before/at us: shift right
            } else if (!op.is_insert && op.inode < flouds_inode) {
                flouds_inode--;  // remove before us: shift left
            }
        }
        return flouds_inode;
    }

    size_t flouds_inode_to_stable_inode(size_t flouds_inode) {
        size_t x = flouds_inode;
        // Undo ops in reverse order
        for (int i = (int)operations.size() - 1; i >= 0; i--) {
            const auto& op = operations[i];
            if (op.is_insert && op.inode <= x) {
                x--;  // undo the ++
            } else if (!op.is_insert && op.inode < x) {
                x++;  // undo the --
            }
        }
        return x + 1;
    }

};