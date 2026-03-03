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
    BitVector* block_bitmap;

    /**
     * Represents a contiguous range.
     */
    struct Extent {
        size_t start_block;
        size_t num_blocks;
    };

    std::map<size_t, std::vector<Extent>> extent_map;

    /**
     * Allocates the required number of block and returns the corresponding extents.
     * 
     * @param required_blocks The number of blocks to allocate.
     * @return A vector of extents representing the allocated blocks.
     */
    std::vector<Extent> allocate_extents(size_t required_blocks) {
        std::vector<Extent> extents;
        
        size_t blocks_allocated = 0;
        size_t num_blocks = block_bitmap->size();
        
        // Find existing free extents
        size_t current_start = SIZE_MAX;
        size_t current_size = 0;
        for (size_t i = 1; i < num_blocks && blocks_allocated < required_blocks; ++i) {
            if (!block_bitmap->access(i)) {
                if (current_start == SIZE_MAX) {
                    current_start = i;
                }
                current_size++;
            } else if (current_size > (required_blocks - blocks_allocated) / 3) {
                // Allocate the current extent if it's large enough (at least 1/3 of the remaining blocks needed to avoid fragmentation)
                extents.push_back({current_start, current_size});
                for (size_t j = current_start; j < current_start + current_size; ++j) {
                    block_bitmap->set(j, true);
                }
                blocks_allocated += current_size;

                current_start = SIZE_MAX;
                current_size = 0;
            }
        }
        
        // Remaining blocks are allocated at the end
        if (blocks_allocated < required_blocks) {
            size_t remaining = required_blocks - blocks_allocated;
            
            if (current_size > 0) {
                // The last blocks are free and can be extended
                size_t start = num_blocks - current_size;
                size_t total_size = current_size + remaining;
                extents.push_back({start, total_size});
                for (size_t j = start; j < num_blocks; ++j) {
                    block_bitmap->set(j, true);
                }
                for (size_t j = num_blocks; j < start + total_size; ++j) {
                    block_bitmap->insert(j, true);
                }
            } else {
                // No free blocks at the end, just allocate new blocks
                extents.push_back({num_blocks, remaining});
                for (size_t j = num_blocks; j < num_blocks + remaining; ++j) {
                    block_bitmap->insert(j, true);
                }
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
        std::vector<Extent> extents = allocate_extents(required_blocks);
        
        // Use the first extents block as the handle for this allocation
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
        size_t extent_offset = offset;
        
        // Iterate through the extents and read from the appropriate blocks
        for (const auto& extent : it->second) {
            size_t extent_size = extent.num_blocks * block_size;
            
            // Skip extents until we reach the offset
            if (extent_offset >= extent_size) {
                extent_offset -= extent_size;
                continue;
            }
            
            // Read from this extent
            while (bytes_read < size && extent_offset < extent_size) {
                size_t block_in_extent = extent_offset / block_size;
                size_t block_offset = extent_offset % block_size;
                size_t physical_block = extent.start_block + block_in_extent;
                size_t to_read = std::min({size - bytes_read, block_size - block_offset, extent_size - extent_offset});
                
                char* temp = new char[block_size];
                block_device->read_block(physical_block, temp);
                memcpy(buffer + bytes_read, temp + block_offset, to_read);
                delete[] temp;
                
                bytes_read += to_read;
                extent_offset += to_read;
            }
            
            // Start from beginning of next extent
            extent_offset = 0;
            
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
        size_t extent_offset = offset;
        
        // Iterate through the extents and write to the appropriate blocks
        for (const auto& extent : it->second) {
            size_t extent_size = extent.num_blocks * block_size;
            
            if (extent_offset >= extent_size) {
                extent_offset -= extent_size;
                continue;
            }
            
            // Write to this extent
            while (bytes_written < size && extent_offset < extent_size) {
                size_t block_in_extent = extent_offset / block_size;
                size_t block_offset = extent_offset % block_size;
                size_t physical_block = extent.start_block + block_in_extent;
                size_t to_write = std::min({size - bytes_written, block_size - block_offset, extent_size - extent_offset});
                
                char* temp = new char[block_size];
                if (block_offset != 0 || to_write < block_size) {
                    block_device->read_block(physical_block, temp);
                }
                memcpy(temp + block_offset, buffer + bytes_written, to_write);
                block_device->write_block(physical_block, temp);
                delete[] temp;
                
                bytes_written += to_write;
                extent_offset += to_write;
            }
            
            // Start from beginning of next extent
            extent_offset = 0;
            
            if (bytes_written >= size) {
                break;
            }
        }
    }

    size_t resize(size_t handle, size_t old_size, size_t new_size) override {
        size_t block_size = block_device->get_block_size();
        size_t required_old_blocks = (old_size + block_size - 1) / block_size;
        size_t required_new_blocks = (new_size + block_size - 1) / block_size;
        
        if (required_new_blocks <= required_old_blocks) {
            // Shrinking: Free blocks from the end of the allocation
            auto it = extent_map.find(handle);

            size_t blocks_to_keep = required_new_blocks;
            std::vector<Extent> new_extents;
            
            // Iterate through the extents and keep the necessary blocks, freeing the rest
            for (const auto& extent : it->second) {
                if (blocks_to_keep >= extent.num_blocks) {
                    new_extents.push_back(extent);
                    blocks_to_keep -= extent.num_blocks;
                } else if (blocks_to_keep > 0) {
                    new_extents.push_back({extent.start_block, blocks_to_keep});
                    for (size_t i = extent.start_block + blocks_to_keep; i < extent.start_block + extent.num_blocks; ++i) {
                        block_bitmap->set(i, false);
                    }
                    blocks_to_keep = 0;
                } else {
                    for (size_t i = extent.start_block; i < extent.start_block + extent.num_blocks; ++i) {
                        block_bitmap->set(i, false);
                    }
                }
            }
            
            it->second = new_extents;
            return handle;
        }
        
        // Growing: Allocate additional blocks and add them as new extents
        size_t additional_blocks = required_new_blocks - required_old_blocks;
        std::vector<Extent> new_extents = allocate_extents(additional_blocks);
        
        auto it = extent_map.find(handle);
        if (it != extent_map.end()) {
            it->second.insert(it->second.end(), new_extents.begin(), new_extents.end());
        }
        
        return handle;
    }

    void serialize(char* buffer, size_t* offset) override {
        block_bitmap->serialize(buffer, offset);        
        size_t map_size = extent_map.size();
        memcpy(buffer + *offset, &map_size, sizeof(size_t));
        *offset += sizeof(size_t);        
        for (const auto& entry : extent_map) {
            memcpy(buffer + *offset, &entry.first, sizeof(size_t));
            *offset += sizeof(size_t);
            size_t num_extents = entry.second.size();
            memcpy(buffer + *offset, &num_extents, sizeof(size_t));
            *offset += sizeof(size_t);            
            for (const auto& extent : entry.second) {
                memcpy(buffer + *offset, &extent.start_block, sizeof(size_t));
                *offset += sizeof(size_t);
                memcpy(buffer + *offset, &extent.num_blocks, sizeof(size_t));
                *offset += sizeof(size_t);
            }
        }
    }

    void deserialize(const char* buffer, size_t* offset) override {
        block_bitmap->deserialize(buffer, offset);        
        size_t map_size;
        memcpy(&map_size, buffer + *offset, sizeof(size_t));
        *offset += sizeof(size_t);        
        extent_map.clear();
        for (size_t i = 0; i < map_size; ++i) {
            size_t handle;
            memcpy(&handle, buffer + *offset, sizeof(size_t));
            *offset += sizeof(size_t);            
            size_t num_extents;
            memcpy(&num_extents, buffer + *offset, sizeof(size_t));
            *offset += sizeof(size_t);            
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

        size += sizeof(size_t);
        for (const auto& entry : extent_map) {
            size += sizeof(size_t);
            size += sizeof(size_t);
            size += entry.second.size() * 2 * sizeof(size_t);
        }
        
        return size;
    }
};

template <>
AllocationManager* create_allocation_manager<ExtentAllocationStrategy>(BlockDevice* block_device) {
    return new ExtentAllocationStrategy(block_device);
}