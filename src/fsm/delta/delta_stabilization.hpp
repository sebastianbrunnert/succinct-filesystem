/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <cstddef>
#include <vector>
#include <map>

class DeltaStabilization {
private:

    struct DeltaOperation {
        bool is_insert;
        size_t inode; // FLOUDS numbers are 0-based, FUSE inodes are 1-based, so this is the FLOUDS inode number
    };

    std::vector<DeltaOperation> operations;
    
    // Track base FLOUDS position for each stable inode
    // This is updated during compaction
    std::map<size_t, size_t> stable_to_base_flouds;
    
    // Track which FLOUDS positions correspond to which stable inodes
    std::map<size_t, size_t> flouds_to_stable;


public:

    void record_insert(size_t inode) {
        operations.push_back({true, inode});
        
        // Update reverse mapping: positions >= inode shift up
        std::map<size_t, size_t> updated_mapping;
        for (const auto& entry : flouds_to_stable) {
            if (entry.first >= inode) {
                updated_mapping[entry.first + 1] = entry.second;
            } else {
                updated_mapping[entry.first] = entry.second;
            }
        }
        flouds_to_stable = updated_mapping;
        
        compact_if_needed();
    }

    void record_remove(size_t inode, size_t stable_inode) {
        operations.push_back({false, inode});
        
        // Remove the deleted inode's mappings
        stable_to_base_flouds.erase(stable_inode);
        flouds_to_stable.erase(inode);
        
        // Update reverse mapping: positions > inode shift down
        std::map<size_t, size_t> updated_mapping;
        for (const auto& entry : flouds_to_stable) {
            if (entry.first > inode) {
                updated_mapping[entry.first - 1] = entry.second;
            } else {
                updated_mapping[entry.first] = entry.second;
            }
        }
        flouds_to_stable = updated_mapping;
        
        compact_if_needed();
    }
    
    // Call this when a new node is created to assign it a stable inode
    size_t assign_stable_inode(size_t flouds_position) {
        // Find the next available stable inode (1-based)
        size_t next_stable = 1;
        for (const auto& entry : stable_to_base_flouds) {
            if (entry.first >= next_stable) {
                next_stable = entry.first + 1;
            }
        }
        
        // Store base position (current position since no prior deltas affect this new node)
        stable_to_base_flouds[next_stable] = flouds_position;
        flouds_to_stable[flouds_position] = next_stable;
        
        return next_stable;
    }
    
    // Call this when mounting an existing filesystem to initialize mappings
    // Assumes stable inode N → FLOUDS position N-1 initially (1-based to 0-based)
    void initialize_from_flouds_size(size_t num_nodes) {
        stable_to_base_flouds.clear();
        flouds_to_stable.clear();
        operations.clear();
        
        for (size_t i = 0; i < num_nodes; i++) {
            size_t stable = i + 1; // 1-based stable inodes
            stable_to_base_flouds[stable] = i;
            flouds_to_stable[i] = stable;
        }
    }
    
private:
    void compact_if_needed() {
        // Compact every 100 operations to prevent unbounded growth
        /*
        if (operations.size() >= 100) {
            compact();
        }
        */
    }
    
    void compact() {
        // Apply all deltas to base positions
        std::map<size_t, size_t> new_base_mapping;
        for (const auto& entry : stable_to_base_flouds) {
            size_t current_pos = stable_inode_to_flouds_inode(entry.first);
            new_base_mapping[entry.first] = current_pos;
        }
        
        stable_to_base_flouds = new_base_mapping;
        operations.clear();
    }

public:

    size_t stable_inode_to_flouds_inode(size_t stable_inode) {
        // Look up the base position for this stable inode
        auto it = stable_to_base_flouds.find(stable_inode);
        if (it == stable_to_base_flouds.end()) {
            return SIZE_MAX; // Invalid stable inode
        }
        
        size_t flouds_inode = it->second;

        // Apply all delta operations to get current position
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
        // Look up in the reverse mapping (which is kept up to date)
        auto it = flouds_to_stable.find(flouds_inode);
        if (it != flouds_to_stable.end()) {
            return it->second;
        }
        return SIZE_MAX; // Not found
    }

};