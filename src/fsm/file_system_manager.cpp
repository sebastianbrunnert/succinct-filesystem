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
    : flouds(nullptr), block_device(nullptr), allocation_manager(nullptr) {
    std::memset(&header, 0, sizeof(FloudsHeader));
}

FileSystemManager::~FileSystemManager() {
    delete flouds;
    delete allocation_manager;
    delete block_device;
}

void FileSystemManager::mount(std::string path) {
    this->block_device = new BlockDevice(path);
    this->allocation_manager = create_allocation_manager<BestFitAllocationStrategy>(block_device);
    this->flouds = create_flouds();
    this->inode_manager = create_inode_manager<ArrayInodeManagerStrategy>(allocation_manager);

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
    // Write FLOUDS data
    size_t flouds_size = flouds->get_serialized_size();
    size_t flouds_handle = (header.flouds_handle == 0) ? allocation_manager->allocate(flouds_size) : allocation_manager->resize(header.flouds_handle, header.flouds_size, flouds_size);
    char* flouds_buffer = new char[flouds_size];
    size_t offset = 0;
    flouds->serialize(flouds_buffer, &offset);
    allocation_manager->write(flouds_handle, flouds_buffer, flouds_size, 0);
    delete[] flouds_buffer;

    // Write inode manager data
    size_t inode_manager_size = inode_manager->get_serialized_size();
    size_t inode_manager_handle = (header.inode_manager_handle == 0) ? allocation_manager->allocate(inode_manager_size) : allocation_manager->resize(header.inode_manager_handle, header.inode_manager_size, inode_manager_size);
    char* inode_manager_buffer = new char[inode_manager_size];
    offset = 0;
    inode_manager->serialize(inode_manager_buffer, &offset);
    allocation_manager->write(inode_manager_handle, inode_manager_buffer, inode_manager_size, 0);
    delete[] inode_manager_buffer;

    // Write allocation manager data
    size_t allocation_manager_size = allocation_manager->get_serialized_size();
    size_t allocation_manager_handle = (header.allocation_manager_handle == 0) ? allocation_manager->allocate(allocation_manager_size) : allocation_manager->resize(header.allocation_manager_handle, header.allocation_manager_size, allocation_manager_size);
    // As the allocation manager has no manage itself too, it might change its own size when allocation new space for itself, so we need to check if the size changed after serialization and if so, serialize again until the size stabilizes.
    size_t allocation_manager_new_size = allocation_manager->get_serialized_size();
    while (allocation_manager_size != allocation_manager_new_size) {
        allocation_manager_size = allocation_manager_new_size;
        allocation_manager_handle = allocation_manager->resize(allocation_manager_handle, allocation_manager_size, allocation_manager_new_size);
        allocation_manager_new_size = allocation_manager->get_serialized_size();
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
    block_device->write_block(0, (char*) &header);
}

size_t FileSystemManager::add_node(size_t parent_inode, std::string name, bool is_folder, uint32_t mode) {
    size_t inode_number = flouds->insert(parent_inode, name, is_folder);
    Inode* inode = inode_manager->insert_inode(inode_number);
    inode->mode = mode;
    return inode_number;
}

void FileSystemManager::remove_node(size_t inode) {
    flouds->remove(inode);
    inode_manager->remove_inode(inode);
}

void FileSystemManager::read_file(size_t inode, char* buffer, size_t size, size_t offset) {
    Inode* node = inode_manager->get_inode(inode);
    allocation_manager->read(node->allocation_handle, buffer, size, offset);
}

void FileSystemManager::write_file(size_t inode, const char* buffer, size_t size, size_t offset) {
    Inode* node = inode_manager->get_inode(inode);
    allocation_manager->write(node->allocation_handle, buffer, size, offset);
    node->modification_time = std::time(nullptr);
}

void FileSystemManager::set_file_size(size_t inode, size_t size) {
    Inode* node = inode_manager->get_inode(inode);
    node->allocation_handle = allocation_manager->resize(node->allocation_handle, node->size, size);
    node->size = size;
}

Inode* FileSystemManager::get_inode(size_t inode) {
    return inode_manager->get_inode(inode);
}