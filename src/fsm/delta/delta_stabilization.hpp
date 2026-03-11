/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <vector>

/**
 * This class implements the delta stabilization mechanism for FLOUDS. 
 * It keeps track of all insert and remove operations. When converting between stable inodes (used by FUSE) and FLOUDS inodes, it uses the log of operation to adjust accordingly. 
 * It also encodes the position within the log.
 */
class DeltaStabilization {
private:

    /**
     * This struct represents an operation (insert or remove) that has been performed on the FLOUDS filesystem.
     */
    struct DeltaOperation {
        bool is_insert;
        // The inode in the FLOUDS structure
        uint64_t inode;
    };

    // Log of operations
    std::vector<DeltaOperation> operations;

public:

    /**
     * This function records an insert operation for a given FLOUDS inode.
     */
    void record_insert(uint64_t inode) {
        operations.push_back({true, inode});
    }

    /**
     * This function records a remove operation for a given FLOUDS inode.
     */
    void record_remove(uint64_t inode) {
        operations.push_back({false, inode});
    }

    /**
     * This function converts a stable inode (used by FUSE) to a FLOUDS inode.
     * 
     * @param stable_inode The stable inode number that FUSE uses.
     * @return The corresponding FLOUDS inode number.
     */
    uint64_t stable_inode_to_flouds_inode(uint64_t stable_inode) {
        #ifdef DELTA_STABILIZATION
        if (stable_inode == 1) {
            return 0;
        }

        uint64_t log_position = stable_inode >> 48;
        uint64_t flouds_position = stable_inode & ((1ULL << 48) - 1) - 1;

        // Calculate what happened between log_position and now and adjust accordingly
        for (size_t i = log_position; i < operations.size(); i++) {
            if (operations[i].is_insert && operations[i].inode <= flouds_position) {
                flouds_position++;
            } else if (!operations[i].is_insert && operations[i].inode < flouds_position) {
                flouds_position--;
            }
        }

        return flouds_position;
        #else 
        return stable_inode - 1;
        #endif
    }

    /**
     * This function converts a FLOUDS inode to a stable inode.
     * 
     * @param flouds_inode The inode number used internally by FLOUDS.
     * @return The corresponding stable inode number that FUSE should use.
     */
    uint64_t flouds_inode_to_stable_inode(uint64_t flouds_inode) {
        #ifdef DELTA_STABILIZATION
        uint64_t log_position = operations.size();
        uint64_t inode_number = flouds_inode + 1;

        return (log_position << 48) | inode_number;
        #else 
        return flouds_inode + 1;
        #endif
    }

};