/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <gtest/gtest.h>
#include "../src/fsm/inode/inode.hpp"
#include "../src/fsm/file_system_manager.hpp"
#include <memory>

// Parameterized test class for different strategies
class InodeManagerTest : public ::testing::Test, public ::testing::WithParamInterface<std::function<InodeManager*(AllocationManager*)>> {
protected:
    InodeManager* create_inode_manager(AllocationManager* allocation_manager) {
        return GetParam()(allocation_manager);
    }
};

TEST_P(InodeManagerTest, Initialize) {
    BlockDevice* block_device = new BlockDevice("test_block_device.img", 4096);
    AllocationManager* allocation_manager = create_allocation_manager<BestFitAllocationStrategy>(block_device);
    InodeManager* inode_manager = create_inode_manager(allocation_manager);
    EXPECT_NE(inode_manager, nullptr);
}

TEST_P(InodeManagerTest, Insert) {
    BlockDevice* block_device = new BlockDevice("test_block_device.img", 4096);
    AllocationManager* allocation_manager = create_allocation_manager<BestFitAllocationStrategy>(block_device);
    InodeManager* inode_manager = create_inode_manager(allocation_manager);

    size_t inode_number = 0;
    Inode* inode = inode_manager->insert_inode(inode_number);
    EXPECT_EQ(inode->size, 0);
    EXPECT_EQ(inode->mode, 0);
}

TEST_P(InodeManagerTest, Get) {
    BlockDevice* block_device = new BlockDevice("test_block_device.img", 4096);
    AllocationManager* allocation_manager = create_allocation_manager<BestFitAllocationStrategy>(block_device);
    InodeManager* inode_manager = create_inode_manager(allocation_manager);

    for(size_t inode_number = 0; inode_number < 10; inode_number++) {
        Inode* inode = inode_manager->insert_inode(inode_number);
        inode->size = inode_number;
    }

    for(size_t inode_number = 0; inode_number < 10; inode_number++) {
        Inode* inode = inode_manager->get_inode(inode_number);
        EXPECT_EQ(inode->size, inode_number);
        EXPECT_NE(inode, nullptr);
    }
}

TEST_P(InodeManagerTest, Remove) {
    BlockDevice* block_device = new BlockDevice("test_block_device.img", 4096);
    AllocationManager* allocation_manager = create_allocation_manager<BestFitAllocationStrategy>(block_device);
    InodeManager* inode_manager = create_inode_manager(allocation_manager);

    for(size_t inode_number = 0; inode_number < 10; inode_number++) {
        Inode* inode = inode_manager->insert_inode(inode_number);
        inode->size = inode_number;
    }

    inode_manager->remove_inode(5);

    for(size_t inode_number = 0; inode_number < 9; inode_number++) {
        Inode* inode = inode_manager->get_inode(inode_number);
        if(inode_number < 5) {
            EXPECT_EQ(inode->size, inode_number);
            EXPECT_NE(inode, nullptr);
        } else {
            EXPECT_EQ(inode->size, inode_number+1);
            EXPECT_NE(inode, nullptr);
        }
    }
}

TEST_P(InodeManagerTest, SerializeDeserialize) {
    BlockDevice* block_device = new BlockDevice("test_block_device.img", 4096);
    AllocationManager* allocation_manager = create_allocation_manager<BestFitAllocationStrategy>(block_device);
    InodeManager* inode_manager = create_inode_manager(allocation_manager);

    for(size_t inode_number = 0; inode_number < 10; inode_number++) {
        Inode* inode = inode_manager->insert_inode(inode_number);
        inode->size = inode_number;
    }

    size_t serialized_size = inode_manager->get_serialized_size();
    char* buffer = new char[serialized_size];
    size_t offset = 0;
    inode_manager->serialize(buffer, &offset);

    InodeManager* deserialized_inode_manager = create_inode_manager(allocation_manager);
    offset = 0;
    deserialized_inode_manager->deserialize(buffer, &offset);

    for(size_t inode_number = 0; inode_number < 10; inode_number++) {
        Inode* inode = deserialized_inode_manager->get_inode(inode_number);
        EXPECT_NE(inode, nullptr);
        EXPECT_EQ(inode->size, inode_number);
    }
}

INSTANTIATE_TEST_SUITE_P(
    InodeStrategies,
    InodeManagerTest,
    ::testing::Values(
        std::function<InodeManager*(AllocationManager*)>([](AllocationManager* allocation_manager) { return create_inode_manager<ArrayInodeManagerStrategy>(allocation_manager); })
    )
);
