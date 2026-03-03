/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef STABLE_INODE_MANAGER_HPP
#define STABLE_INODE_MANAGER_HPP

#include <cstddef>
#include <map>
#include <vector>
#include <cstdint>

/**
 * Manages stable inode numbers that remain constant despite FLOUDS structure changes.
 * 
 * This implements delta-based inode stabilization: when nodes are inserted or deleted
 * in the FLOUDS structure, their positions shift, but we maintain stable external inode
 * numbers by tracking these changes in a delta history.
 * 
 * Key concepts:
 * - Stable inode: External identifier exposed to FUSE, never changes for a file/folder
 * - FLOUDS position: Internal position in BFS order, changes with inserts/deletes
 * - Delta history: Sequence of operations used to compute current position from base position
 */
class StableInodeManager {
public:
    enum DeltaOpType { INSERT, DELETE };
    
    struct DeltaOp {
        DeltaOpType type;
        size_t flouds_position;  // Position at time of operation (relative to prior deltas)
        
        DeltaOp(DeltaOpType t, size_t pos) : type(t), flouds_position(pos) {}
    };

private:
    // Maps stable inode → base FLOUDS position (before applying deltas)
    std::map<size_t, size_t> stable_to_base_flouds;
    
    // Maps current FLOUDS position → stable inode (updated after each delta)
    std::map<size_t, size_t> flouds_to_stable;
    
    // History of insert/delete operations since last compaction
    std::vector<DeltaOp> delta_history;
    
    // Next stable inode number to assign
    size_t next_stable_inode;
    
    // Maximum delta history size before automatic compaction
    static const size_t MAX_DELTA_HISTORY = 100;
    
    /**
     * Compute the delta for a given position based on an operation.
     * 
     * δᵢ(f) = +1 if opᵢ = (insert, fᵢ) and f ≥ fᵢ
     *         -1 if opᵢ = (delete, fᵢ) and f > fᵢ
     *          0 otherwise
     */
    int compute_delta(size_t position, const DeltaOp& op) const {
        if (op.type == INSERT && position >= op.flouds_position) {
            return 1;
        } else if (op.type == DELETE && position > op.flouds_position) {
            return -1;
        }
        return 0;
    }
    
    /**
     * Rebuild the reverse mapping after delta operations or compaction.
     */
    void rebuild_reverse_mapping() {
        flouds_to_stable.clear();
        for (const auto& entry : stable_to_base_flouds) {
            size_t current_pos = stable_to_current_flouds(entry.first);
            flouds_to_stable[current_pos] = entry.first;
        }
    }

public:
    StableInodeManager() : next_stable_inode(1) {
        // Reserve stable inode 0 for root (maps to FLOUDS position 0)
        stable_to_base_flouds[0] = 0;
        flouds_to_stable[0] = 0;
    }
    
    /**
     * Convert a stable inode number to its current FLOUDS position.
     * This applies all delta operations to the base position.
     * 
     * f_current = f_base + Σᵢ δᵢ(f^(i-1))
     * 
     * @param stable_inode The stable inode number from FUSE
     * @return Current FLOUDS position, or SIZE_MAX if not found
     */
    size_t stable_to_current_flouds(size_t stable_inode) const {
        auto it = stable_to_base_flouds.find(stable_inode);
        if (it == stable_to_base_flouds.end()) {
            return SIZE_MAX;
        }
        
        size_t position = it->second;
        
        // Apply each delta operation in sequence
        for (const auto& op : delta_history) {
            position += compute_delta(position, op);
        }
        
        return position;
    }
    
    /**
     * Convert a current FLOUDS position to its stable inode number.
     * 
     * @param flouds_position Current FLOUDS position
     * @return Stable inode number, or SIZE_MAX if not found
     */
    size_t current_flouds_to_stable(size_t flouds_position) const {
        auto it = flouds_to_stable.find(flouds_position);
        return (it != flouds_to_stable.end()) ? it->second : SIZE_MAX;
    }
    
    /**
     * Assign a new stable inode for a newly created node.
     * Called after inserting into FLOUDS structure.
     * 
     * @param flouds_position The FLOUDS position where the node was inserted
     * @return The new stable inode number
     */
    size_t assign_stable_inode(size_t flouds_position) {
        size_t stable = next_stable_inode++;
        
        // The base position is the current position (no prior deltas affect this new node)
        stable_to_base_flouds[stable] = flouds_position;
        flouds_to_stable[flouds_position] = stable;
        
        return stable;
    }
    
    /**
     * Record an insertion operation that occurred at the given position.
     * This shifts all subsequent FLOUDS positions by +1.
     * 
     * @param flouds_position Where the node was inserted (in pre-insert numbering)
     */
    void record_insert(size_t flouds_position) {
        delta_history.emplace_back(INSERT, flouds_position);
        
        // Update reverse mapping incrementally (avoid full rebuild for performance)
        std::map<size_t, size_t> updated_mapping;
        for (const auto& entry : flouds_to_stable) {
            size_t pos = entry.first;
            if (pos >= flouds_position) {
                updated_mapping[pos + 1] = entry.second;
            } else {
                updated_mapping[pos] = entry.second;
            }
        }
        flouds_to_stable = updated_mapping;
        
        check_compaction();
    }
    
    /**
     * Record a deletion operation that occurred at the given position.
     * This shifts all subsequent FLOUDS positions by -1.
     * 
     * @param flouds_position Where the node was deleted (in pre-delete numbering)
     * @param stable_inode The stable inode of the deleted node (to remove mapping)
     */
    void record_delete(size_t flouds_position, size_t stable_inode) {
        delta_history.emplace_back(DELETE, flouds_position);
        
        // Remove the deleted node's mappings
        stable_to_base_flouds.erase(stable_inode);
        flouds_to_stable.erase(flouds_position);
        
        // Update reverse mapping incrementally
        std::map<size_t, size_t> updated_mapping;
        for (const auto& entry : flouds_to_stable) {
            size_t pos = entry.first;
            if (pos > flouds_position) {
                updated_mapping[pos - 1] = entry.second;
            } else {
                updated_mapping[pos] = entry.second;
            }
        }
        flouds_to_stable = updated_mapping;
        
        check_compaction();
    }
    
    /**
     * Check if compaction is needed and perform it if threshold is exceeded.
     */
    void check_compaction() {
        if (delta_history.size() >= MAX_DELTA_HISTORY) {
            compact();
        }
    }
    
    /**
     * Compact the delta history by applying all deltas to base positions.
     * After compaction, the delta history is cleared and base positions are updated.
     * 
     * This is analogous to log-structured filesystem checkpointing.
     */
    void compact() {
        // Apply all deltas to base positions
        std::map<size_t, size_t> new_base_mapping;
        for (const auto& entry : stable_to_base_flouds) {
            size_t current_pos = stable_to_current_flouds(entry.first);
            new_base_mapping[entry.first] = current_pos;
        }
        
        stable_to_base_flouds = new_base_mapping;
        delta_history.clear();
        
        // Reverse mapping is already correct from incremental updates
    }
    
    /**
     * Get the current size of the delta history.
     */
    size_t get_delta_history_size() const {
        return delta_history.size();
    }

};

#endif // STABLE_INODE_MANAGER_HPP
