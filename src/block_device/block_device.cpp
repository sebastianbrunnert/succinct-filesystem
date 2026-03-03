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

BlockDevice::BlockDevice(const std::string filename, size_t block_size) : block_size(block_size) {
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    std::string full_path = std::string(cwd) + "/" + filename;

    file = open(full_path.c_str(), O_RDWR | O_CREAT, 0644);

    if (file == -1) {
        throw std::runtime_error("Could not open or create file");
    }
    
    if (lseek(file, 0, SEEK_END) < (off_t)block_size) {
        ftruncate(file, block_size);
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