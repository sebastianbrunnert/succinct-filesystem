/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "../../block_device/block_device.hpp"
#include "../../serialization/serializable.hpp"

/**
 * This class defines the interface for different allocation strategies on a block device.
 * Logically related sequences of blocks are identified by a handle.
 * The first block (block 0) is reserved for the filesystem header and should not be allocated.
 */
class AllocationManager : public Serializable {
protected:
    BlockDevice* block_device;
public:

    /**
     * @param block_device The block device to use for allocation.
     */
    AllocationManager(BlockDevice* block_device) : block_device(block_device) {}

    /**
     * Virtual destructor.
     */
    virtual ~AllocationManager() = default;

    /**
     * Allocates some space on the block devices.
     * 
     * @return The handle of the allocated space.
     */
    virtual size_t allocate(size_t size) = 0;

    /**
     * Frees the space with the given handle.
     * 
     * @param handle The handle of the space to free.
     */
    virtual void free(size_t handle) = 0;

    /**
     * Reads data from the block device.
     * 
     * @param handle The handle of the space to read from.
     * @param buffer The buffer to write the data into. Must be at least size bytes.
     * @param size The number of bytes to read.
     * @param offset The offset within the allocated space to start reading from.
     */
    virtual void read(size_t handle, char* buffer, size_t size, size_t offset) = 0;

    /**
     * Writes data to the block device.
     * 
     * @param handle The handle of the space to write to.
     * @param buffer The buffer containing the data. Must be at least size bytes.
     * @param size The number of bytes to write.
     * @param offset The offset within the allocated space to start writing to.
     */
    virtual void write(size_t handle, const char* buffer, size_t size, size_t offset) = 0;

    /**
     * Resizes the allocated space with the given handle to the new size. If the new size can fit in the old space, usually the same handle is returned.
     * 
     * @param handle The handle of the space to resize.
     * @param old_size The old size of the allocated space in bytes. Must be the correct size.
     * @param new_size The new size of the allocated space in bytes.
     * @return The new handle of the allocated space.
     */
    virtual size_t resize(size_t handle, size_t old_size, size_t new_size) = 0;

    virtual void serialize(char* buffer, size_t* offset) override = 0;
    virtual void deserialize(const char* buffer, size_t* offset) override = 0;
    virtual size_t get_serialized_size() override = 0;

};

/**
 * Factory function to create an allocation manager with the given strategy.
 * 
 * @param block_device The block device to use for allocation.
 * @return A pointer to a new AllocationManager instance.
 */
template <typename AllocationStrategy> AllocationManager* create_allocation_manager(BlockDevice* block_device);

class BestFitAllocationStrategy;
template <> AllocationManager* create_allocation_manager<BestFitAllocationStrategy>(BlockDevice* block_device);