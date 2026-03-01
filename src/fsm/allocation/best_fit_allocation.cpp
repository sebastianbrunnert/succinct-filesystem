/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "allocation_manager.hpp"
#include "../../bitvector/bitvector.hpp"
#include "../../block_device/block_device.hpp"
#include <cstring>

/**
 * This class implements a consecutive allocation strategy on a block device. It allocates a new block to the smallest possible space or at the end of the device.
 */
class BestFitAllocationStrategy : public AllocationManager {
private:
    size_t next_block = 1;

public:
    BestFitAllocationStrategy(BlockDevice* block_device) : AllocationManager(block_device) {}

    ~BestFitAllocationStrategy() override = default;

    size_t allocate(size_t size) override {
        size_t num_blocks = 1 + (size - 1) / block_device->get_block_size();
        size_t handle = next_block;
        next_block += num_blocks;
        return handle;
    }

    void free(size_t handle) override {
        // Do nothing - never reuse space
    }

    void read(size_t handle, char* buffer, size_t size, size_t offset) override {
        size_t block_size = block_device->get_block_size();
        size_t bytes_read = 0;
        
        while (bytes_read < size) {
            size_t current_offset = offset + bytes_read;
            size_t block_index = handle + (current_offset / block_size);
            size_t block_offset = current_offset % block_size;
            size_t to_read = std::min(size - bytes_read, block_size - block_offset);
            
            char* temp = new char[block_size];
            block_device->read_block(block_index, temp);
            memcpy(buffer + bytes_read, temp + block_offset, to_read);
            delete[] temp;
            
            bytes_read += to_read;
        }
    }

    void write(size_t handle, const char* buffer, size_t size, size_t offset) override {
        size_t block_size = block_device->get_block_size();
        size_t bytes_written = 0;
        
        while (bytes_written < size) {
            size_t current_offset = offset + bytes_written;
            size_t block_index = handle + (current_offset / block_size);
            size_t block_offset = current_offset % block_size;
            size_t to_write = std::min(size - bytes_written, block_size - block_offset);
            
            char* temp = new char[block_size];
            if (block_offset != 0 || to_write < block_size) {
                block_device->read_block(block_index, temp);
            }
            memcpy(temp + block_offset, buffer + bytes_written, to_write);
            block_device->write_block(block_index, temp);
            delete[] temp;
            
            bytes_written += to_write;
        }
    }

    size_t resize(size_t handle, size_t old_size, size_t new_size) override {
        // If the file is empty or has an invalid handle, allocate new space
        if (old_size == 0 || handle == 0) {
            return allocate(new_size);
        }
        
        // Check if the new size can fit in the old space
        size_t old_num_blocks = 1 + (old_size - 1) / block_device->get_block_size();
        size_t new_num_blocks = 1 + (new_size - 1) / block_device->get_block_size();
        if (new_num_blocks <= old_num_blocks) {
            return handle;
        } else {
            return allocate(new_size);
        }
    }

    void serialize(char* buffer, size_t* offset) override {
        memcpy(buffer + *offset, &next_block, sizeof(size_t));
        *offset += sizeof(size_t);
    }

    void deserialize(const char* buffer, size_t* offset) override {
        memcpy(&next_block, buffer + *offset, sizeof(size_t));
        *offset += sizeof(size_t);
    }

    size_t get_serialized_size() override {
        return sizeof(size_t);
    }
};

template <>
AllocationManager* create_allocation_manager<BestFitAllocationStrategy>(BlockDevice* block_device) {
    return new BestFitAllocationStrategy(block_device);
}