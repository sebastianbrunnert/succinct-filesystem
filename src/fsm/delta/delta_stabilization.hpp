/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <cstddef>
#include <vector>

/**
 * Delta-based inode stabilization for FUSE caching.
 * 
 * FUSE inodes are encoded as: (version << 48) | original_flouds_position
 * - Upper 16 bits: version at creation time (immutable for each file)
 * - Lower 48 bits: original FLOUDS position when file was created
 * 
 * The delta log tracks inserts/deletes so we can convert between:
 * - Stable inode (version + original position) → current FLOUDS position
 * - Current FLOUDS position → stable inode (requires reverse lookup)
 * 
 * This enables FUSE caching because files keep the same inode number
 * even as their FLOUDS positions shift due to tree modifications.
 */
class DeltaStabilization {
private:
    uint64_t version = 1; // Current version, incremented on each modification
    
    struct DeltaOperation {
        bool is_insert;      // true = insert, false = delete
        size_t position;     // FLOUDS position where operation occurred
        uint64_t version;    // Version when this operation occurred
    };
    
    std::vector<DeltaOperation> operations;

public:
    /**
     * Record an insertion at a FLOUDS position.
     * Positions >= position shift right by 1.
     */
    void record_insert(size_t flouds_position) {
        version++;
        operations.push_back({true, flouds_position, version});
    }

    /**
     * Record a removal at a FLOUDS position.
     * Positions > position shift left by 1.
     */
    void record_remove(size_t flouds_position) {
        version++;
        operations.push_back({false, flouds_position, version});
    }

    /**
     * Convert stable FUSE inode to current FLOUDS position.
     * Applies all delta operations that occurred after the inode's creation.
     */
    size_t stable_inode_to_flouds_inode(uint64_t stable_inode) {
        uint64_t creation_version = (stable_inode >> 48);
        size_t position = stable_inode & 0xFFFFFFFFFFFF;
        
        // Apply deltas that occurred after this inode was created
        for (const auto& op : operations) {
            if (op.version <= creation_version) continue; // Skip ops before creation
            
            if (op.is_insert) {
                // Insert at k: positions >= k shift right
                if (position >= op.position) {
                    position++;
                }
            } else {
                // Delete at k: positions > k shift left
                if (position > op.position) {
                    position--;
                } else if (position == op.position) {
                    // This file was deleted! Return a sentinel (or handle specially)
                    return static_cast<size_t>(-1);
                }
            }
        }
        
        return position;
    }

    /**
     * Convert current FLOUDS position to stable FUSE inode.
     * Uses current version for newly seen files.
     */
    uint64_t flouds_inode_to_stable_inode(size_t flouds_position) {
        // Encode with current version as creation version
        return (version << 48) | flouds_position;
    }
    
    /**
     * Get current version number.
     */
    uint64_t get_version() const {
        return version;
    }
};