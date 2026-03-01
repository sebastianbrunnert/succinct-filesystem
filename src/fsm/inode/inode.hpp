/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <ctime>
#include "../allocation/allocation_manager.hpp"

/**
 * This structure represents an inode in the filesystem. It contains its metadata, such as size, permissions, timestamps, reference to FLOUDS node and allocation handle.
 */
struct Inode {
    size_t allocation_handle;
    size_t size;
    uint32_t mode;
    time_t modification_time;
    time_t access_time;
    time_t creation_time;
};

/**
 * This class defines the interface for managing metadata in the filesystem. It allows inserting and deleting inodes in a sequence.
 * The sequence should be analougous to the sequence of FLOUDS nodes.
 */
class InodeManager : public Serializable {
protected:
    AllocationManager* allocation_manager;

public:
    virtual ~InodeManager() = default;

    /**
     * @param allocation_manager The allocation manager.
     */
    InodeManager(AllocationManager* allocation_manager) : allocation_manager(allocation_manager) {}

    /**
     * Gets the inode with the given inode number.
     * 
     * @param inode The inode number. Must be a valid inode number.
     * @return The inode with the given inode number.
     */
    virtual Inode* get_inode(size_t inode) = 0;

    /**
     * Inserts a new inode into the sequence.
     * 
     * @param inode The inode number to insert at. Must be a valid inode number.
     * @return The inserted inode.
     */
    virtual Inode* insert_inode(size_t inode) = 0;

    /**
     * Removes the inode with the given inode number from the sequence.
     * 
     * @param inode The inode number to remove. Must be a valid inode number.
     */
    virtual void remove_inode(size_t inode) = 0;

    virtual void serialize(char* buffer, size_t* offset) override = 0;
    virtual void deserialize(const char* buffer, size_t* offset) override = 0;
    virtual size_t get_serialized_size() override = 0;

};

/**
 * Factory function to create a new InodeManager instance with the specified strategy.
 * 
 * @param InodeManagerStrategy The strategy to use for the InodeManager implementation.
 * @param allocation_manager The allocation manager to use for the InodeManager.
 * @return A pointer to a new InodeManager instance.
 */
template <typename InodeManagerStrategy> InodeManager* create_inode_manager(AllocationManager* allocation_manager);

// Different strategies for implementing the interface
class ArrayInodeManagerStrategy;
template <> InodeManager* create_inode_manager<ArrayInodeManagerStrategy>(AllocationManager* allocation_manager);