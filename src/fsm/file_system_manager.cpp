/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "file_system_manager.hpp"

FileSystemManager::~FileSystemManager() {
    delete flouds;
    delete allocation_manager;
    delete block_device;
}

void FileSystemManager::mount(std::string path) {
    this->block_device = new BlockDevice(path);
    this->allocation_manager = create_allocation_manager<BestFitAllocationStrategy>(block_device);
    this->flouds = create_flouds();

    char buffer[sizeof(FloudsHeader)];
    block_device->read_block(0, buffer);
    FloudsHeader* header = (FloudsHeader*) buffer;

    if(std::string(header->magic, 6) != "FLOUDS") {
        // Initialize new filesystem
        FloudsHeader new_header;
        const char* magic_str = "FLOUDS";
        std::copy(magic_str, magic_str + 6, new_header.magic);
        new_header.allocation_manager_size = 0;
        new_header.allocation_manager_handle = 0;
        new_header.flouds_handle = 0;
        new_header.flouds_size = 0;
        this->header = new_header;
        
        block_device->write_block(0, (char*) &new_header);
        this->save();
    } else {
        // Load existing filesystem
        this->header = *header;
    }
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
    block_device->write_block(0, (char*) &header);
}

void FileSystemManager::add_node(size_t parent_id, std::string name, bool is_folder, uint32_t mode) {
    // TODO: Implement
}

void FileSystemManager::remove_node(size_t node_id) {
    // TODO: Implement
}

void FileSystemManager::read_file(size_t node_id, char* buffer, size_t size, size_t offset) {
    // TODO: Implement
}

void FileSystemManager::write_file(size_t node_id, const char* buffer, size_t size, size_t offset) {
    // TODO: Implement
}

size_t FileSystemManager::get_file_size(size_t node_id) {
    // TODO: Implement
    return 0;
}

size_t FileSystemManager::set_file_size(size_t node_id, size_t size) {
    // TODO: Implement
    return 0;
}

time_t FileSystemManager::get_modification_time(size_t node_id) {
    // TODO: Implement
    return 0;
}

time_t FileSystemManager::get_access_time(size_t node_id) {
    // TODO: Implement
    return 0;
}

time_t FileSystemManager::get_creation_time(size_t node_id) {
    // TODO: Implement
    return 0;
}

void FileSystemManager::set_modification_time(size_t node_id, time_t time) {
    // TODO: Implement
}

void FileSystemManager::set_access_time(size_t node_id, time_t time) {
    // TODO: Implement
}

void FileSystemManager::set_creation_time(size_t node_id, time_t time) {
    // TODO: Implement
}

uint32_t FileSystemManager::get_mode(size_t node_id) {
    // TODO: Implement
    return 0;
}

void FileSystemManager::set_mode(size_t node_id, uint32_t mode) {
    // TODO: Implement
}
