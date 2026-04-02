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

    this->delayed_write = new DelayedWrite{0, 0, 0};
    this->delayed_write_buffer = new char[block_device->get_block_size() * 16]; // Buffer for delayed writes, can hold up to 16 blocks of data

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

        this->save();
    } else {
        // Load existing filesystem
        std::memcpy(&header, buffer, sizeof(FloudsHeader));

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

    delete[] buffer;
}

void FileSystemManager::unmount() {
    #ifdef DELAYED_ALLOCATION
    flush_delayed_write();
    #endif

    this->save();
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
    #ifdef DELAYED_ALLOCATION
    // Flush any pending delayed write to ensure consistency
    flush_delayed_write();
    #endif

    size_t inode_number = flouds->insert(parent_inode, name, is_folder);
    Inode* inode = inode_manager->insert_inode(inode_number);

    inode->mode = mode;
    inode->access_time = std::time(nullptr);
    inode->creation_time = std::time(nullptr);
    inode->modification_time = std::time(nullptr);
    return inode_number;
}

void FileSystemManager::remove_node(size_t inode_number) {
    #ifdef DELAYED_ALLOCATION
    // Flush any pending delayed write to ensure consistency
    flush_delayed_write();
    #endif

    Inode* inode = inode_manager->get_inode(inode_number);
    if (inode->allocation_handle != 0) {
        allocation_manager->free(inode->allocation_handle, inode->size);
    }
    
    flouds->remove(inode_number);
    inode_manager->remove_inode(inode_number);
}

void FileSystemManager::read_file(size_t inode, char* buffer, size_t size, size_t offset) {
    #ifdef DELAYED_ALLOCATION
    if (delayed_write->size > 0 && delayed_write->inode == inode && offset >= delayed_write->offset && offset < delayed_write->offset + delayed_write->size) {
        // If the read overlaps with the delayed write, we need to read from the delayed buffer
        size_t delayed_write_offset = offset - delayed_write->offset;
        size_t bytes_from_delayed_write = std::min(size, delayed_write->size - delayed_write_offset);
        std::memcpy(buffer, delayed_write_buffer + delayed_write_offset, bytes_from_delayed_write);

        if (bytes_from_delayed_write == size) {
            return;
        }

        // Otherwise we need to read the remaining part from the block device
        buffer += bytes_from_delayed_write;
        size -= bytes_from_delayed_write;
        offset += bytes_from_delayed_write;
    }
    #endif DELAYED_ALLOCATION

    Inode* node = inode_manager->get_inode(inode);
    allocation_manager->read(node->allocation_handle, buffer, size, offset);
    node->access_time = std::time(nullptr);
}

void FileSystemManager::write_file(size_t inode, const char* buffer, size_t size, size_t offset) {
    Inode* node = inode_manager->get_inode(inode);

    #ifdef DELAYED_ALLOCATION
    if (delayed_write->size > 0) {
        // If there is already a delayed write, check if the new write can be merged with this because they are contigous to the same file.
        if (delayed_write->inode == inode && delayed_write->offset + delayed_write->size == offset && delayed_write->size + size <= block_device->get_block_size() * 16) {
            std::memcpy(delayed_write_buffer + delayed_write->size, buffer, size);
            delayed_write->size += size;
            return;
        }

        // Otherwise flush the delayed write
        flush_delayed_write();
    }

    if (size >= block_device->get_block_size() * 16) {
        // If the write is larger than the delayed write buffer, write it directly to the block device
        node->allocation_handle = (node->allocation_handle == 0) ? allocation_manager->allocate(offset + size) : allocation_manager->resize(node->allocation_handle, node->size, offset + size);
        node->size = offset + size;
        allocation_manager->write(node->allocation_handle, buffer, size, offset);
        node->modification_time = std::time(nullptr);
        return;
    } else {
        // Otherwise, store it in the delayed write buffer
        delayed_write = new DelayedWrite{inode, size, offset};
        std::memcpy(delayed_write_buffer, buffer, size);
        return;
    }
    #else
    // If the file is not large enough to write at the given offset, we need to resize it first.
    if (offset + size > node->size) {
        node->allocation_handle = (node->allocation_handle == 0) ? allocation_manager->allocate(offset + size) : allocation_manager->resize(node->allocation_handle, node->size, offset + size);
        node->size = offset + size;
    }

    allocation_manager->write(node->allocation_handle, buffer, size, offset);
    #endif

    node->modification_time = std::time(nullptr);
}

#ifdef DELAYED_ALLOCATION
void FileSystemManager::flush_delayed_write() {
    if (delayed_write->size == 0) return;

    Inode* node = inode_manager->get_inode(delayed_write->inode);
    node->allocation_handle = (node->allocation_handle == 0) ? allocation_manager->allocate(delayed_write->size) : allocation_manager->resize(node->allocation_handle, node->size, delayed_write->size);
    node->size = delayed_write->size;
    allocation_manager->write(node->allocation_handle, delayed_write_buffer, delayed_write->size, delayed_write->offset);

    delayed_write->size = 0;
}
#endif

void FileSystemManager::set_file_size(size_t inode, size_t size) {
    Inode* node = inode_manager->get_inode(inode);
    node->allocation_handle = (node->allocation_handle == 0) ? allocation_manager->allocate(size) : allocation_manager->resize(node->allocation_handle, node->size, size);
    node->size = size;
}

Inode* FileSystemManager::get_inode(size_t inode) {
    return inode_manager->get_inode(inode);
}