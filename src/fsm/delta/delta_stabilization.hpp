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
        operations.push_back({true, inode});
    }

    void record_remove(uint64_t inode) {
        operations.push_back({false, inode});
    }

    uint64_t stable_inode_to_flouds_inode(uint64_t stable_inode) {
        uint64_t log_position = stable_inode >> 48;
        uint64_t inode_number = stable_inode & ((1ULL << 48) - 1);

        /*
        // Calculate what happend in between the log_position and now and adjust the inode_number accordingly
        for (size_t i = log_position; i < operations.size(); i++) {
            if (operations[i].is_insert && operations[i].inode <= inode_number) {
                inode_number++;
            } else if (!operations[i].is_insert && operations[i].inode < inode_number) {
                inode_number--;
            }
        }
        */

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
        return flouds_inode + 1;                
        */
    }

};