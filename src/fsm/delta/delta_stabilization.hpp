/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <vector>

class DeltaStabilization {
private:

    struct DeltaOperation {
        bool is_insert;
        uint64_t inode;
    };

    std::vector<DeltaOperation> operations;

public:

    void record_insert(uint64_t inode) {
        operations.push_back({true, stable_inode_to_flouds_inode(inode)});
    }

    void record_remove(uint64_t inode) {
        operations.push_back({false, stable_inode_to_flouds_inode(inode)});
    }

    uint64_t stable_inode_to_flouds_inode(uint64_t stable_inode) {
        uint64_t log_position = stable_inode >> 48;
        uint64_t inode_number = stable_inode & (1ULL << 48) - 1;

        return inode_number - 1;

        /*
        WITHOUT CACHING IT WOULD BE:
        return stable_inode - 1;        
        */
    }

    uint64_t flouds_inode_to_stable_inode(uint64_t flouds_inode) {
        uint64_t log_position = operations.size();
        uint64_t inode_number = flouds_inode + 1;

        return (log_position << 48) | inode_number;

        /*
        WITHOUT CACHING IT WOULD BE:
        return flouds_inode + 1;        
        */
    }

};