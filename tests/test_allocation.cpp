/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <gtest/gtest.h>
#include "../src/fsm/allocation/allocation_manager.hpp"
#include <memory>

// Parameterized test class for different strategies
class AllocationManagerTest : public ::testing::Test, public ::testing::WithParamInterface<std::function<AllocationManager*(BlockDevice*)>> {
protected:
    AllocationManager* create_allocation_manager(BlockDevice* block_device) {
        return GetParam()(block_device);
    }
};

TEST_P(AllocationManagerTest, Initialize) {
    BlockDevice* block_device = new BlockDevice("test_block_device.img", 4096);
    AllocationManager* allocation_manager = create_allocation_manager(block_device);
    EXPECT_NE(allocation_manager, nullptr);
}

TEST_P(AllocationManagerTest, AllocateAndFree) {
    BlockDevice* block_device = new BlockDevice("test_block_device.img", 4096);
    AllocationManager* allocation_manager = create_allocation_manager(block_device);
    size_t handle1 = allocation_manager->allocate(4096);
    size_t handle2 = allocation_manager->allocate(8192);
    EXPECT_NE(handle1, handle2);
    allocation_manager->free(handle1);
    allocation_manager->free(handle2);

    std::remove("test_block_device.img");
}

TEST_P(AllocationManagerTest, ReadAndWrite) {
    BlockDevice* block_device = new BlockDevice("test_block_device.img", 4096);
    AllocationManager* allocation_manager = create_allocation_manager(block_device);
    size_t handle = allocation_manager->allocate(4096);
    const char* data = "Lorem ipsum dolor sit amet";
    allocation_manager->write(handle, data, strlen(data) + 1, 0);
    char buffer[4096];
    allocation_manager->read(handle, buffer, sizeof(buffer), 0);
    EXPECT_STREQ(buffer, data);
    allocation_manager->free(handle);
    std::remove("test_block_device.img");
}

TEST_P(AllocationManagerTest, Resize) {
    BlockDevice* block_device = new BlockDevice("test_block_device.img", 4096);
    AllocationManager* allocation_manager = create_allocation_manager(block_device);
    size_t handle = allocation_manager->allocate(4096);
    const char* data = "Lorem ipsum dolor sit amet";
    allocation_manager->write(handle, data, strlen(data) + 1, 0);
    
    // Resize to a larger size
    size_t new_handle = allocation_manager->resize(handle, 4096, 8192);
    char buffer[8192];
    allocation_manager->read(new_handle, buffer, sizeof(buffer), 0);
    EXPECT_STREQ(buffer, data);

    // Resize to a smaller size
    new_handle = allocation_manager->resize(new_handle, 8192, 2048);
    allocation_manager->read(new_handle, buffer, sizeof(buffer), 0);
    EXPECT_STREQ(buffer, data);

    allocation_manager->free(new_handle);
    std::remove("test_block_device.img");
}

TEST_P(AllocationManagerTest, SerializeDeserialize) {
    BlockDevice* block_device = new BlockDevice("test_block_device.img", 4096);
    AllocationManager* allocation_manager = create_allocation_manager(block_device);
    size_t handle = allocation_manager->allocate(4096);
    const char* data = "Lorem ipsum dolor sit amet";
    allocation_manager->write(handle, data, strlen(data) + 1, 0);

    // Serialize
    char buffer[4096];
    size_t offset = 0;
    allocation_manager->serialize(buffer, &offset);

    // Deserialize into a new instance
    AllocationManager* deserialized_manager = create_allocation_manager(block_device);
    size_t deserialize_offset = 0;
    deserialized_manager->deserialize(buffer, &deserialize_offset);

    // Read from the deserialized manager
    char read_buffer[4096];
    deserialized_manager->read(handle, read_buffer, sizeof(read_buffer), 0);
    EXPECT_STREQ(read_buffer, data);

    allocation_manager->free(handle);
    deserialized_manager->free(handle);
    std::remove("test_block_device.img");
}

INSTANTIATE_TEST_SUITE_P(
    AllocationStrategies,
    AllocationManagerTest,
    ::testing::Values(
        std::function<AllocationManager*(BlockDevice*)>([](BlockDevice* block_device) { return create_allocation_manager<BestFitAllocationStrategy>(block_device); })
    )
);
