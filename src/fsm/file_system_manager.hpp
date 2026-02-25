/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "../block_device/block_device.hpp"
#include "allocation/allocation_manager.hpp"
#include "../flouds/flouds.hpp"

/**
 * This structure defines the first block of the filesystem, which contains a magic string to identify the filesystem and allocation handles for all relevant components.
 */
struct FloudsHeader {
    // "FLOUDS"
    char magic[6];
    
    size_t allocation_manager_handle;
    size_t allocation_manager_size;

    size_t flouds_handle;
    size_t flouds_size;
};

/**
 * This class syncs the FLOUDS data structure with the block device and allows reading and writing of files and folders.
 * It works with inode numbers and these are translated to FLOUDS node indices internally. Inode numbers stay stable even if the FLOUDS structure changes.
 */
class FileSystemManager {
private:
    FloudsHeader header;
    Flouds* flouds;
    BlockDevice* block_device;
    AllocationManager* allocation_manager;

public:

    /**
     * Constructor initializes all pointers to nullptr.
     */
    FileSystemManager();

    /**
     * Virtual destructor.
     */
    virtual ~FileSystemManager();

    /**
     * Loads the filesystem the block device at the specified path. If path does not exist, a new filesystem will be created.
     * 
     * @param path The path to the block device file.
     * @throws std::runtime_error if the block device file is invalid.
     */
    virtual void mount(std::string path);

    /**
     * Unloads the filesystem.
     */
    virtual void unmount();

    /**
     * Saves the current state of the filesystem to the block device.
     */
    virtual void save();

    /**
     * Gets the FLOUDS data structure of the filesystem.
     */
    virtual Flouds* get_flouds() {
        return flouds;
    }

    /**
     * Adds a node to the filesystem as a child of the specified parent node.
     * 
     * @param parent_inode The inode number of the parent node.
     * @param name The name of the new node.
     * @param is_folder true if the new node is a folder, false if it is a file.
     * @param mode The permissions of the new node.
     */
    virtual void add_node(size_t parent_inode, std::string name, bool is_folder, uint32_t mode);

    /**
     * Removes a node from the filesystem.
     * 
     * @param inode The inode number of the node to remove. Must be a valid inode.
     */
    virtual void remove_node(size_t inode);

    virtual void read_file(size_t inode, char* buffer, size_t size, size_t offset);

    virtual void write_file(size_t inode, const char* buffer, size_t size, size_t offset);

    /**
     * Gets the size of the file represented by the inode.
     * 
     * @param inode The inode number of the file. Must be a valid inode.
     * @return The size of the file in bytes.
     */
    virtual size_t get_file_size(size_t inode);

    /**
     * Sets the size of the file represented by the inode. It allocates blocks as needed.
     * 
     * @param inode The inode number of the file. Must be a valid inode.
     * @param size The new size of the file in bytes.
     */
    virtual void set_file_size(size_t inode, size_t size);

    virtual time_t get_modification_time(size_t inode);
    virtual time_t get_access_time(size_t inode);
    virtual time_t get_creation_time(size_t inode);
    virtual void set_modification_time(size_t inode, time_t time);
    virtual void set_access_time(size_t inode, time_t time);
    virtual void set_creation_time(size_t inode, time_t time);

    virtual uint32_t get_mode(size_t inode);
    virtual void set_mode(size_t inode, uint32_t mode);

};