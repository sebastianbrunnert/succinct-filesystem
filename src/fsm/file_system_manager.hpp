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
 */
class FileSystemManager {
private:
    FloudsHeader header;
    Flouds* flouds;
    BlockDevice* block_device;
    AllocationManager* allocation_manager;

public:

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
     * Saves the current state of the filesystem to the block device.
     */
    virtual void save();

    virtual void add_node(size_t parent_id, std::string name, bool is_folder, uint32_t mode);

    virtual void remove_node(size_t node_id);

    virtual void read_file(size_t node_id, char* buffer, size_t size, size_t offset);

    virtual void write_file(size_t node_id, const char* buffer, size_t size, size_t offset);

    virtual size_t get_file_size(size_t node_id);
    virtual size_t set_file_size(size_t node_id, size_t size);

    virtual time_t get_modification_time(size_t node_id);
    virtual time_t get_access_time(size_t node_id);
    virtual time_t get_creation_time(size_t node_id);
    virtual void set_modification_time(size_t node_id, time_t time);
    virtual void set_access_time(size_t node_id, time_t time);
    virtual void set_creation_time(size_t node_id, time_t time);

    virtual uint32_t get_mode(size_t node_id);
    virtual void set_mode(size_t node_id, uint32_t mode);

};