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
#include <map>
#include <vector>

/**
 * This class implements an extent-based allocation strategy. It maintains a bitmap to track allocated blocks and allocates multiple contiguous blocks (extents) for each allocation request. Each allocation can consist of multiple non-contiguous extents, eliminating the need for data copying during resize operations.
 */
class ExtentAllocationStrategy : public AllocationManager {
private:
    struct Extent {
        size_t start_block;
        size_t num_blocks;
    };

    BitVector* block_bitmap;
    std::map<size_t, std::vector<Extent>> extent_map;

    // Find and allocate extents for the required number of blocks
    std::vector<Extent> allocate_extents(size_t required_blocks) {
        std::vector<Extent> extents;
        size_t blocks_allocated = 0;
        size_t num_blocks = block_bitmap->size();
        
        // First pass: find existing free blocks
        size_t current_start = SIZE_MAX;
        size_t current_size = 0;
        
        for (size_t i = 1; i < num_blocks && blocks_allocated < required_blocks; ++i) {
            if (!block_bitmap->access(i)) {
                if (current_start == SIZE_MAX) {
                    current_start = i;
                }
                current_size++;
            } else {
                if (current_size > 0) {
                    size_t to_allocate = std::min(current_size, required_blocks - blocks_allocated);
                    extents.push_back({current_start, to_allocate});
                    for (size_t j = current_start; j < current_start + to_allocate; ++j) {
                        block_bitmap->set(j, true);
                    }
                    blocks_allocated += to_allocate;
                }
                current_start = SIZE_MAX;
                current_size = 0;
            }
        }
        
        // Handle last extent in existing blocks
        if (current_size > 0 && blocks_allocated < required_blocks) {
            size_t to_allocate = std::min(current_size, required_blocks - blocks_allocated);
            extents.push_back({current_start, to_allocate});
            for (size_t j = current_start; j < current_start + to_allocate; ++j) {
                block_bitmap->set(j, true);
            }
            blocks_allocated += to_allocate;
        }
        
        // Allocate remaining blocks at the end
        if (blocks_allocated < required_blocks) {
            size_t remaining = required_blocks - blocks_allocated;
            size_t start = num_blocks;
            extents.push_back({start, remaining});
            for (size_t i = start; i < start + remaining; ++i) {
                block_bitmap->insert(i, true);
            }
        }
        
        return extents;
    }

public:
    ExtentAllocationStrategy(BlockDevice* block_device) : AllocationManager(block_device) {
        block_bitmap = create_bitvector<WordBitVectorStrategy>(0);
        block_bitmap->insert(0, true);  // Block 0 is reserved for the header
    }

    ~ExtentAllocationStrategy() override {
        delete block_bitmap;
    }

    size_t allocate(size_t size) override {
        size_t required_blocks = (size + block_device->get_block_size() - 1) / block_device->get_block_size();
        if (required_blocks == 0) {
            return 0;  // No allocation needed for zero-size
        }
        
        std::vector<Extent> extents = allocate_extents(required_blocks);
        
        // Use the first extent's starting block as the handle for compatibility
        size_t handle = extents[0].start_block;
        extent_map[handle] = extents;
        
        return handle;
    }

    void free(size_t handle, size_t size) override {
        auto it = extent_map.find(handle);
        if (it == extent_map.end()) {
            return;
        }
        
        for (const auto& extent : it->second) {
            for (size_t i = extent.start_block; i < extent.start_block + extent.num_blocks; ++i) {
                block_bitmap->set(i, false);
            }
        }
        
        extent_map.erase(it);
    }

    void read(size_t handle, char* buffer, size_t size, size_t offset) override {
        if (handle == 0) {
            return;  // No allocation
        }
        
        auto it = extent_map.find(handle);
        if (it == extent_map.end()){
            // Fallback: This is needed when loading own data before extent_map is initailized.
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
            return;
        }
        
        size_t block_size = block_device->get_block_size();
        size_t bytes_read = 0;
        size_t logical_offset = offset;
        
        // Translate logical offset to extent and block
        for (const auto& extent : it->second) {
            size_t extent_size = extent.num_blocks * block_size;
            
            if (logical_offset >= extent_size) {
                logical_offset -= extent_size;
                continue;
            }
            
            // Read from this extent
            while (bytes_read < size && logical_offset < extent_size) {
                size_t block_in_extent = logical_offset / block_size;
                size_t block_offset = logical_offset % block_size;
                size_t physical_block = extent.start_block + block_in_extent;
                size_t to_read = std::min({size - bytes_read, block_size - block_offset, extent_size - logical_offset});
                
                char* temp = new char[block_size];
                block_device->read_block(physical_block, temp);
                memcpy(buffer + bytes_read, temp + block_offset, to_read);
                delete[] temp;
                
                bytes_read += to_read;
                logical_offset += to_read;
            }
            
            logical_offset = 0;  // Start from beginning of next extent
            
            if (bytes_read >= size) {
                break;
            }
        }
    }

    void write(size_t handle, const char* buffer, size_t size, size_t offset) override {
        auto it = extent_map.find(handle);
        if (it == extent_map.end()) {
            return;
        }
        
        size_t block_size = block_device->get_block_size();
        size_t bytes_written = 0;
        size_t logical_offset = offset;
        
        // Translate logical offset to extent and block
        for (const auto& extent : it->second) {
            size_t extent_size = extent.num_blocks * block_size;
            
            if (logical_offset >= extent_size) {
                logical_offset -= extent_size;
                continue;
            }
            
            // Write to this extent
            while (bytes_written < size && logical_offset < extent_size) {
                size_t block_in_extent = logical_offset / block_size;
                size_t block_offset = logical_offset % block_size;
                size_t physical_block = extent.start_block + block_in_extent;
                size_t to_write = std::min({size - bytes_written, block_size - block_offset, extent_size - logical_offset});
                
                char* temp = new char[block_size];
                if (block_offset != 0 || to_write < block_size) {
                    block_device->read_block(physical_block, temp);
                }
                memcpy(temp + block_offset, buffer + bytes_written, to_write);
                block_device->write_block(physical_block, temp);
                delete[] temp;
                
                bytes_written += to_write;
                logical_offset += to_write;
            }
            
            logical_offset = 0;  // Start from beginning of next extent
            
            if (bytes_written >= size) {
                break;
            }
        }
    }

    size_t resize(size_t handle, size_t old_size, size_t new_size) override {
        size_t block_size = block_device->get_block_size();
        size_t old_blocks = (old_size + block_size - 1) / block_size;
        size_t new_blocks = (new_size + block_size - 1) / block_size;
        
        if (new_blocks <= old_blocks) {
            // Shrinking: free excess blocks from the end
            auto it = extent_map.find(handle);
            if (it != extent_map.end()) {
                size_t blocks_to_keep = new_blocks;
                std::vector<Extent> new_extents;
                
                for (const auto& extent : it->second) {
                    if (blocks_to_keep >= extent.num_blocks) {
                        new_extents.push_back(extent);
                        blocks_to_keep -= extent.num_blocks;
                    } else if (blocks_to_keep > 0) {
                        new_extents.push_back({extent.start_block, blocks_to_keep});
                        // Free the rest
                        for (size_t i = extent.start_block + blocks_to_keep; i < extent.start_block + extent.num_blocks; ++i) {
                            block_bitmap->set(i, false);
                        }
                        blocks_to_keep = 0;
                    } else {
                        // Free entire extent
                        for (size_t i = extent.start_block; i < extent.start_block + extent.num_blocks; ++i) {
                            block_bitmap->set(i, false);
                        }
                    }
                }
                
                it->second = new_extents;
            }
            return handle;
        }
        
        // Growing: just add more extents, no copying needed!
        size_t additional_blocks = new_blocks - old_blocks;
        std::vector<Extent> new_extents = allocate_extents(additional_blocks);
        
        auto it = extent_map.find(handle);
        if (it != extent_map.end()) {
            it->second.insert(it->second.end(), new_extents.begin(), new_extents.end());
        }
        
        return handle;
    }

    void serialize(char* buffer, size_t* offset) override {
        // Serialize the block bitmap
        block_bitmap->serialize(buffer, offset);
        
        // Serialize extent_map size
        size_t map_size = extent_map.size();
        memcpy(buffer + *offset, &map_size, sizeof(size_t));
        *offset += sizeof(size_t);
        
        // Serialize each entry in extent_map
        for (const auto& entry : extent_map) {
            // Serialize handle
            memcpy(buffer + *offset, &entry.first, sizeof(size_t));
            *offset += sizeof(size_t);
            
            // Serialize number of extents
            size_t num_extents = entry.second.size();
            memcpy(buffer + *offset, &num_extents, sizeof(size_t));
            *offset += sizeof(size_t);
            
            // Serialize each extent
            for (const auto& extent : entry.second) {
                memcpy(buffer + *offset, &extent.start_block, sizeof(size_t));
                *offset += sizeof(size_t);
                memcpy(buffer + *offset, &extent.num_blocks, sizeof(size_t));
                *offset += sizeof(size_t);
            }
        }
    }

    void deserialize(const char* buffer, size_t* offset) override {
        // Deserialize the block bitmap
        block_bitmap->deserialize(buffer, offset);
        
        // Deserialize extent_map size
        size_t map_size;
        memcpy(&map_size, buffer + *offset, sizeof(size_t));
        *offset += sizeof(size_t);
        
        // Deserialize each entry in extent_map
        extent_map.clear();
        for (size_t i = 0; i < map_size; ++i) {
            // Deserialize handle
            size_t handle;
            memcpy(&handle, buffer + *offset, sizeof(size_t));
            *offset += sizeof(size_t);
            
            // Deserialize number of extents
            size_t num_extents;
            memcpy(&num_extents, buffer + *offset, sizeof(size_t));
            *offset += sizeof(size_t);
            
            // Deserialize each extent
            std::vector<Extent> extents;
            for (size_t j = 0; j < num_extents; ++j) {
                Extent extent;
                memcpy(&extent.start_block, buffer + *offset, sizeof(size_t));
                *offset += sizeof(size_t);
                memcpy(&extent.num_blocks, buffer + *offset, sizeof(size_t));
                *offset += sizeof(size_t);
                extents.push_back(extent);
            }
            
            extent_map[handle] = extents;
        }
    }

    size_t get_serialized_size() override {
        size_t size = block_bitmap->get_serialized_size();
        size += sizeof(size_t);  // map_size
        
        for (const auto& entry : extent_map) {
            size += sizeof(size_t);  // handle
            size += sizeof(size_t);  // num_extents
            size += entry.second.size() * 2 * sizeof(size_t);  // extents (start_block, num_blocks)
        }
        
        return size;
    }
};

template <>
AllocationManager* create_allocation_manager<ExtentAllocationStrategy>(BlockDevice* block_device) {
    return new ExtentAllocationStrategy(block_device);
}