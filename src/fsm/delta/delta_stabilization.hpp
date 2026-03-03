/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <cstddef>

class DeltaStabilization {
private:

    class DeltaOperation {
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
        return stable_inode - 1;
    }

    size_t flouds_inode_to_stable_inode(size_t flouds_inode) {
        return flouds_inode + 1;
    }

};