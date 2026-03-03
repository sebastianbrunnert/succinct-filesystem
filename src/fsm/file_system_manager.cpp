/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "file_system_manager.hpp"
#include <cstring>
#include <iostream>

FileSystemManager::FileSystemManager() 
    : flouds(nullptr), block_device(nullptr), allocation_manager(nullptr), 
      inode_manager(nullptr), stable_inode_manager(nullptr) {
    std::memset(&header, 0, sizeof(FloudsHeader));
}

FileSystemManager::~FileSystemManager() {
    delete stable_inode_manager;
    delete inode_manager;
    delete flouds;
    delete allocation_manager;
    delete block_device;
}

void FileSystemManager::mount(std::string path) {
    this->block_device = new BlockDevice(path);
    this->allocation_manager = create_allocation_manager<ExtentAllocationStrategy>(block_device);
    this->flouds = create_flouds();
    this->inode_manager = create_inode_manager<ArrayInodeManagerStrategy>(allocation_manager);
    this->stable_inode_manager = new StableInodeManager();

    char* buffer = new char[block_device->get_block_size()];
    block_device->read_block(0, buffer);
    
    // Check magic string safely
    if (std::memcmp(buffer, "FLOUDS", 6) != 0) {
        this->inode_manager->insert_inode(0); // Insert root inode

        // Initialize new filesystem
        std::memset(&header, 0, sizeof(FloudsHeader));
        std::memcpy(header.magic, "FLOUDS", 6);
        
        header.allocation_manager_size = 0;
        header.allocation_manager_handle = 0;
        header.flouds_handle = 0;
        header.flouds_size = 0;
        header.inode_manager_handle = 0;
        header.inode_manager_size = 0;
        header.stable_inode_manager_handle = 0;
        header.stable_inode_manager_size = 0;
        
        delete[] buffer;
        this->save();
    } else {
        // Load existing filesystem
        std::memcpy(&header, buffer, sizeof(FloudsHeader));
        delete[] buffer;

        // Load allocation manager
        char* allocation_manager_buffer = new char[header.allocation_manager_size];
        allocation_manager->read(header.allocation_manager_handle, allocation_manager_buffer, header.allocation_manager_size, 0);
        size_t offset = 0;
        allocation_manager->deserialize(allocation_manager_buffer, &offset);
        delete[] allocation_manager_buffer;
        
        // Load stable inode manager (before FLOUDS, as we need it to understand positions)
        if (header.stable_inode_manager_size > 0) {
            char* stable_inode_manager_buffer = new char[header.stable_inode_manager_size];
            allocation_manager->read(header.stable_inode_manager_handle, stable_inode_manager_buffer, header.stable_inode_manager_size, 0);
            offset = 0;
            stable_inode_manager->deserialize(stable_inode_manager_buffer, &offset);
            delete[] stable_inode_manager_buffer;
        }

        // Load FLOUDS
        char* flouds_buffer = new char[header.flouds_size];
        allocation_manager->read(header.flouds_handle, flouds_buffer, header.flouds_size, 0);
        offset = 0;
        flouds->deserialize(flouds_buffer, &offset);
        delete[] flouds_buffer;

        // Load inode manager
        char* inode_manager_buffer = new char[header.inode_manager_size];
        allocation_manager->read(header.inode_manager_handle, inode_manager_buffer, header.inode_manager_size, 0);
        offset = 0;
        inode_manager->deserialize(inode_manager_buffer, &offset);
        delete[] inode_manager_buffer;
    }
}

void FileSystemManager::unmount() {
    this->save();
    // Destructor will handle cleanup
}

void FileSystemManager::save() {
    // Compact stable inode manager before saving (flatten delta history into base positions)
    stable_inode_manager->compact();
    
    // Write FLOUDS data
    size_t flouds_size = flouds->get_serialized_size();
    std::cout << "Saving FLOUDS with size " << flouds_size << " bytes" << std::endl;
    size_t flouds_handle = (header.flouds_handle == 0) ? allocation_manager->allocate(flouds_size) : allocation_manager->resize(header.flouds_handle, header.flouds_size, flouds_size);
    char* flouds_buffer = new char[flouds_size];
    size_t offset = 0;
    flouds->serialize(flouds_buffer, &offset);
    allocation_manager->write(flouds_handle, flouds_buffer, flouds_size, 0);
    delete[] flouds_buffer;

    // Write inode manager data
    size_t inode_manager_size = inode_manager->get_serialized_size();
    std::cout << "Saving inode manager with size " << inode_manager_size << " bytes" << std::endl;
    size_t inode_manager_handle = (header.inode_manager_handle == 0) ? allocation_manager->allocate(inode_manager_size) : allocation_manager->resize(header.inode_manager_handle, header.inode_manager_size, inode_manager_size);
    char* inode_manager_buffer = new char[inode_manager_size];
    offset = 0;
    inode_manager->serialize(inode_manager_buffer, &offset);
    allocation_manager->write(inode_manager_handle, inode_manager_buffer, inode_manager_size, 0);
    delete[] inode_manager_buffer;
    
    // Write stable inode manager data (mappings only, delta history is session-specific)
    size_t stable_inode_manager_size = stable_inode_manager->get_serialized_size();
    std::cout << "Saving stable inode manager with size " << stable_inode_manager_size << " bytes" << std::endl;
    size_t stable_inode_manager_handle = (header.stable_inode_manager_handle == 0) ? allocation_manager->allocate(stable_inode_manager_size) : allocation_manager->resize(header.stable_inode_manager_handle, header.stable_inode_manager_size, stable_inode_manager_size);
    char* stable_inode_manager_buffer = new char[stable_inode_manager_size];
    offset = 0;
    stable_inode_manager->serialize(stable_inode_manager_buffer, &offset);
    allocation_manager->write(stable_inode_manager_handle, stable_inode_manager_buffer, stable_inode_manager_size, 0);
    delete[] stable_inode_manager_buffer;

    // Write allocation manager data
    size_t allocation_manager_size = allocation_manager->get_serialized_size();
    size_t allocation_manager_handle = (header.allocation_manager_handle == 0) ? allocation_manager->allocate(allocation_manager_size) : allocation_manager->resize(header.allocation_manager_handle, header.allocation_manager_size, allocation_manager_size);
    // As the allocation manager has no manage itself too, it might change its own size when allocation new space for itself, so we need to check if the size changed after serialization and if so, serialize again until the size stabilizes.
    size_t allocation_manager_new_size = allocation_manager->get_serialized_size();
    std::cout << "Saving allocation manager with size " << allocation_manager_new_size << " bytes" << std::endl;
    while (allocation_manager_size != allocation_manager_new_size) {
        allocation_manager_size = allocation_manager_new_size;
        allocation_manager_handle = allocation_manager->resize(allocation_manager_handle, allocation_manager_size, allocation_manager_new_size);
        allocation_manager_new_size = allocation_manager->get_serialized_size();
        std::cout << "Allocation manager size changed to " << allocation_manager_new_size << " bytes, resizing..." << std::endl;
    }
    char* allocation_manager_buffer = new char[allocation_manager_size];
    offset = 0;
    allocation_manager->serialize(allocation_manager_buffer, &offset);
    allocation_manager->write(allocation_manager_handle, allocation_manager_buffer, allocation_manager_size, 0);
    delete[] allocation_manager_buffer;

    // Write header
    header.flouds_handle = flouds_handle;
    header.flouds_size = flouds_size;
    header.allocation_manager_handle = allocation_manager_handle;
    header.allocation_manager_size = allocation_manager_size;
    header.inode_manager_handle = inode_manager_handle;
    header.inode_manager_size = inode_manager_size;
    header.stable_inode_manager_handle = stable_inode_manager_handle;
    header.stable_inode_manager_size = stable_inode_manager_size;
    block_device->write_block(0, (char*) &header);

    std::cout << "Save complete" << std::endl;
}

size_t FileSystemManager::add_node(size_t parent_stable_inode, std::string name, bool is_folder, uint32_t mode) {
    // Convert stable parent inode to current FLOUDS position
    size_t parent_flouds_pos = stable_inode_manager->stable_to_current_flouds(parent_stable_inode);
    
    // Insert into FLOUDS structure (this gives us the new node's FLOUDS position)
    size_t new_flouds_pos = flouds->insert(parent_flouds_pos, name, is_folder);
    
    // Record the insertion in delta history (shifts subsequent positions)
    stable_inode_manager->record_insert(new_flouds_pos);
    
    // Assign a new stable inode for this node
    size_t new_stable_inode = stable_inode_manager->assign_stable_inode(new_flouds_pos);
    
    // Create inode metadata (using stable inode as key, not FLOUDS position)
    Inode* inode = inode_manager->insert_inode(new_stable_inode);
    inode->mode = mode;
    
    return new_stable_inode;
}

void FileSystemManager::remove_node(size_t stable_inode) {
    // Convert stable inode to current FLOUDS position
    size_t flouds_pos = stable_inode_manager->stable_to_current_flouds(stable_inode);
    
    // Free allocated blocks if any (use stable inode, not FLOUDS position)
    Inode* inode = inode_manager->get_inode(stable_inode);
    if (inode->allocation_handle != 0) {
        allocation_manager->free(inode->allocation_handle, inode->size);
    }
    
    // Remove from FLOUDS structure
    flouds->remove(flouds_pos);
    
    // Remove from inode manager (use stable inode as key)
    inode_manager->remove_inode(stable_inode);
    
    // Record the deletion in delta history (shifts subsequent positions)
    stable_inode_manager->record_delete(flouds_pos, stable_inode);
}

void FileSystemManager::read_file(size_t stable_inode, char* buffer, size_t size, size_t offset) {
    Inode* node = inode_manager->get_inode(stable_inode);
    allocation_manager->read(node->allocation_handle, buffer, size, offset);
}

void FileSystemManager::write_file(size_t stable_inode, const char* buffer, size_t size, size_t offset) {
    Inode* node = inode_manager->get_inode(stable_inode);

    // If the file is not large enough to write at the given offset, we need to resize it first.
    if (offset + size > node->size) {
        node->allocation_handle = (node->allocation_handle == 0) ? allocation_manager->allocate(offset + size) : allocation_manager->resize(node->allocation_handle, node->size, offset + size);
        node->size = offset + size;
    }

    allocation_manager->write(node->allocation_handle, buffer, size, offset);
    node->modification_time = std::time(nullptr);
}

void FileSystemManager::set_file_size(size_t stable_inode, size_t size) {
    Inode* node = inode_manager->get_inode(stable_inode);
    std::cout << "Resizing file with stable inode " << stable_inode << " from size " << node->size << " to size " << size << std::endl;
    node->allocation_handle = (node->allocation_handle == 0) ? allocation_manager->allocate(size) : allocation_manager->resize(node->allocation_handle, node->size, size);
    node->size = size;
}

Inode* FileSystemManager::get_inode(size_t stable_inode) {
    return inode_manager->get_inode(stable_inode);
}