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
        size_t inode;
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
        size_t new_flouds_inode = stable_inode;

        for (const auto& op : operations) {
            if (op.is_insert && op.inode <= new_flouds_inode) {
                new_flouds_inode--;
            } else if (!op.is_insert && op.inode < new_flouds_inode) {
                new_flouds_inode++;
            }
        }

        return new_flouds_inode - 1; // Convert to 0-based index for FLOUDS
    }

    size_t flouds_inode_to_stable_inode(size_t flouds_inode) {
        size_t stable_inode = flouds_inode;

        for (const auto& op : operations) {
            if (op.is_insert && op.inode <= flouds_inode) {
                stable_inode++;
            } else if (!op.is_insert && op.inode < flouds_inode) {
                stable_inode--;
            }
        }

        return stable_inode + 1; // Convert to 1-based index for FUSE
    }

};