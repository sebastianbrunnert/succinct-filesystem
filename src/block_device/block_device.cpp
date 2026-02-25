/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "block_device.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cerrno>

BlockDevice::BlockDevice(const std::string filename, size_t block_size) : block_size(block_size) {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    std::cout << "DEBUG BlockDevice: Current working directory: " << cwd << std::endl;
    std::cout << "DEBUG BlockDevice: Opening file: " << filename << std::endl;
    
    file = open(filename.c_str(), O_RDWR | O_CREAT, 0644);

    if (file == -1) {
        std::cerr << "ERROR BlockDevice: Could not open file '" << filename << "': " 
                  << strerror(errno) << std::endl;
        throw std::runtime_error("Could not open or create file");
    }
    
    std::cout << "DEBUG BlockDevice: File opened successfully with fd=" << file << std::endl;
    
    off_t file_size = lseek(file, 0, SEEK_END);
    std::cout << "DEBUG BlockDevice: File size: " << file_size << " bytes, block_size: " << block_size << std::endl;
    
    if (file_size < (off_t)block_size) {
        std::cout << "DEBUG BlockDevice: Truncating file to " << block_size << " bytes" << std::endl;
        if (ftruncate(file, block_size) == -1) {
            std::cerr << "ERROR BlockDevice: ftruncate failed: " << strerror(errno) << std::endl;
        }
    }
}

BlockDevice::~BlockDevice() {
    close(file);
}

void BlockDevice::read_block(size_t block_index, char* buffer) {
    lseek(file, block_index * block_size, SEEK_SET);
    read(file, buffer, block_size);
}

void BlockDevice::write_block(size_t block_index, const char* buffer) {
    lseek(file, block_index * block_size, SEEK_SET);
    write(file, buffer, block_size);
}

/*
void BlockDevice::read_data(size_t block_index, size_t offset, size_t size, char* buffer) {
#ifdef BLOCK_DEVICE_SIMULATED
    lseek(file, block_index * block_size + offset, SEEK_SET);
    read(file, buffer, size);
#else
    char* block_buffer = new char[block_size];
    size_t blocks_to_read = (offset + size + block_size - 1) / block_size;
    size_t read = 0;

    for (size_t i = 0; i < blocks_to_read; i++) {
        read_block(block_index + i, block_buffer);
        size_t block_offset = (i == 0) ? offset : 0;
        size_t length_to_read = block_size - block_offset;
        if (i == blocks_to_read - 1) {
            length_to_read = ((offset + size - 1) % block_size) + 1;
        }

        memcpy(buffer + read, block_buffer + block_offset, length_to_read);
        read += length_to_read;
    }
#endif
}

void BlockDevice::write_data(size_t block_index, size_t offset, size_t size, const char* buffer) {
#ifdef BLOCK_DEVICE_SIMULATED
    lseek(file, block_index * block_size + offset, SEEK_SET);
    write(file, buffer, size);
#else
    char* block_buffer = new char[block_size];
    size_t blocks_to_write = (offset + size + block_size - 1) / block_size;
    size_t written = 0;

    for (size_t i = 0; i < blocks_to_write; i++) {
        size_t block_offset = (i == 0) ? offset : 0;
        size_t length_to_write = block_size - block_offset;
        if (i == blocks_to_write - 1) {
            length_to_write = ((offset + size - 1) % block_size) + 1;
        }

        if (length_to_write < block_size) {
            read_block(block_index + i, block_buffer);
        }

        memcpy(block_buffer + block_offset, buffer + written, length_to_write);
        write_block(block_index + i, block_buffer);
        written += length_to_write;
    }
#endif
}
*/