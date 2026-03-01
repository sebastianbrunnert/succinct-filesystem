/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "../block_device/block_device.hpp"
#include "../flouds/flouds.hpp"
#include "allocation/allocation_manager.hpp"
#include "inode/inode.hpp"

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

    size_t inode_manager_handle;
    size_t inode_manager_size;
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
    InodeManager* inode_manager;

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
     * @return The inode number of the newly created node.
     */
    size_t add_node(size_t parent_inode, std::string name, bool is_folder, uint32_t mode);

    /**
     * Removes a node from the filesystem.
     * 
     * @param inode The inode number of the node to remove. Must be a valid inode.
     */
    virtual void remove_node(size_t inode);

    /**
     * Reads data from a file represented by the inode number.
     * 
     * @param inode The inode number of the file to read from. Must be a valid inode representing a file.
     * @param buffer The buffer to write the data into. Must be at least size bytes.
     * @param size The number of bytes to read.
     * @param offset The offset within the file to start reading from.
     */
    virtual void read_file(size_t inode, char* buffer, size_t size, size_t offset);

    /**
     * Writes data to a file represented by the inode number.
     * 
     * @param inode The inode number of the file to write to. Must be a valid inode representing a file and the file must be large enough.
     * @param buffer The buffer containing the data to write. Must be at least size bytes.
     * @param size The number of bytes to write.
     * @param offset The offset within the file to start writing to.
     */
    virtual void write_file(size_t inode, const char* buffer, size_t size, size_t offset);

    /**
     * Sets the size of the file represented by the inode. It allocates blocks as needed.
     * 
     * @param inode The inode number of the file. Must be a valid inode.
     * @param size The new size of the file in bytes.
     */
    virtual void set_file_size(size_t inode, size_t size);

    /**
     * Gets the inode structure for the given inode number.
     * 
     * @param inode The inode number to get the inode structure for. Must be a valid inode.
     * @return A pointer to the inode structure.
     */
    virtual Inode* get_inode(size_t inode);
};