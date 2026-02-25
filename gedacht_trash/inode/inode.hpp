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
 * This class defines the interface for managing inodes in the filesystem. It allows creating and deleting inodes, as well as getting and setting their metadata.
 * This Inode Manager also translates between inode numbers and FLOUDS node ids. Different stabilization strategies can be implemented.
 */
class InodeManager {
protected:
    AllocationManager* allocation_manager;

public:
    virtual ~InodeManager() = default;

    /**
     * @param allocation_manager The allocation manager to use for allocating space for the Inodes.
     */
    InodeManager(AllocationManager* allocation_manager) : allocation_manager(allocation_manager) {}

    /**
     * Creates a new inode and returns its inode number.
     * 
     * @param flouds_node_id The ID of the FLOUDS node to create the inode for. Must be a valid FLOUDS node ID.
     * @return The inode number of the created inode.
     */
    virtual size_t create_inode(size_t flouds_node_id) = 0;

    /**
     * Deletes the inode with the given inode number.
     * 
     * @param inode The inode number of the inode to delete. Must be a valid inode number.
     */
    virtual void delete_inode(size_t inode) = 0;

    /**
     * Gets the FLOUDS node id associated with the given inode number.
     * 
     * @param inode The inode number. Must be a valid inode number.
     * @return The FLOUDS node id associated with the given inode number.
     */
    virtual size_t get_flouds_node_id(size_t inode) = 0;

    /**
     * Gets the inode with the given inode number.
     * 
     * @param inode The inode number. Must be a valid inode number.
     * @return The inode with the given inode number.
     */
    virtual Inode get_inode(size_t inode) = 0;

};

/**
 * Factory function to create a new InodeManager instance with the specified strategy.
 * 
 * @param InodeManagerStrategy The strategy to use for the InodeManager implementation.
 * @return A pointer to a new InodeManager instance.
 */
template <typename InodeManagerStrategy> InodeManager* create_inode_manager(AllocationManager* allocation_manager);

// Different strategies for implementing the interface
class MapInodeManager;
template <> InodeManager* create_inode_manager<MapInodeManager>(AllocationManager* allocation_manager);