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
#include <cstdint>

/**
 * This class implements a consecutive allocation strategy on a block device. It allocates a new block to the smallest possible space or at the end of the device.
 */
class BestFitAllocationStrategy : public AllocationManager {
private:
    BitVector* block_bitmap;

public:
    BestFitAllocationStrategy(BlockDevice* block_device) : AllocationManager(block_device) {
        block_bitmap = create_bitvector<WordBitVectorStrategy>(0);
        block_bitmap->insert(0, true);  // Block 0 is reserved for the header
    }

    ~BestFitAllocationStrategy() override {
        delete block_bitmap;
    }

    size_t allocate(size_t size) override {
        size_t best_start = SIZE_MAX;
        size_t best_size = SIZE_MAX;

        size_t current_start = SIZE_MAX;
        size_t current_size = 0;

        size_t required_blocks = (size + block_device->get_block_size() - 1) / block_device->get_block_size();
        size_t num_blocks = block_bitmap->size();

        // Skip block 0 as its reserved for the header
        for (size_t i = 1; i < num_blocks; i++) {
            if (!block_bitmap->access(i)) {
                if (current_start == SIZE_MAX) {
                    current_start = i;
                }
                current_size++;
            } else {
                if (current_size >= required_blocks && current_size < best_size) {
                    best_start = current_start;
                    best_size = current_size;
                }
                current_start = SIZE_MAX;
                current_size = 0;
            }
        }

        if (current_size >= required_blocks && current_size < best_size) {
            best_start = current_start;
        }

        if (best_start == SIZE_MAX) {
            // Allocate at the end
            best_start = num_blocks;
            for (size_t i = best_start; i < best_start + required_blocks; i++) {
                block_bitmap->insert(i, true);
            }
        } else {
            for (size_t i = best_start; i < best_start + required_blocks; i++) {
                block_bitmap->set(i, true);
            }
        }

        return best_start;
    }

    void free(size_t handle, size_t size) override {
        size_t start_block = handle;
        size_t required_blocks = (size + block_device->get_block_size() - 1) / block_device->get_block_size();
        for (size_t i = start_block; i < start_block + required_blocks; i++) {
            block_bitmap->set(i, false);
        }
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
        size_t block_size = block_device->get_block_size();
        size_t required_old_blocks = (old_size + block_size - 1) / block_size;
        size_t required_new_blocks = (new_size + block_size - 1) / block_size;

        if (required_new_blocks <= required_old_blocks) {
            // Shrinking: Free blocks from the end of the allocation
            for (size_t i = handle + required_new_blocks; i < handle + required_old_blocks; i++) {
                block_bitmap->set(i, false);
            }
            return handle;
        }

        // Check if the blocks after the current allocation are free and can be used for resizing
        bool can_extend = true;
        for (size_t i = handle + required_old_blocks; i < handle + required_new_blocks; i++) {
            if (block_bitmap->access(i)) {
                can_extend = false;
                break;
            }
        }
        if (can_extend) {
            // Extend the allocation in place
            for (size_t i = handle + required_old_blocks; i < handle + required_new_blocks; i++) {
                block_bitmap->set(i, true);
            }
            return handle;
        }

        size_t new_handle = allocate(new_size);
        char* buffer = new char[old_size];
        read(handle, buffer, old_size, 0);
        write(new_handle, buffer, old_size, 0);
        free(handle, old_size);
        delete[] buffer;
        return new_handle;
    }

    size_t get_total_blocks() const override {
        return block_bitmap->size();
    }

    size_t get_used_blocks() const override {
        return block_bitmap->rank1(block_bitmap->size() - 1);
    }

    void serialize(char* buffer, size_t* offset) override {
        block_bitmap->serialize(buffer, offset);
    }

    void deserialize(const char* buffer, size_t* offset) override {
        block_bitmap->deserialize(buffer, offset);
    }

    size_t get_serialized_size() override {
        return block_bitmap->get_serialized_size();
    }
};

template <>
AllocationManager* create_allocation_manager<BestFitAllocationStrategy>(BlockDevice* block_device) {
    return new BestFitAllocationStrategy(block_device);
}