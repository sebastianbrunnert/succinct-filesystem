/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <gtest/gtest.h>
#include "../src/block_device/block_device.hpp"

TEST(BlockDeviceTest, Create) {
    BlockDevice* device = new BlockDevice("test_block_device.img", 4096);
    EXPECT_NE(device, nullptr);
    EXPECT_EQ(device->get_block_size(), 4096);
    delete device;
    std::remove("test_block_device.img");
}

TEST(BlockDeviceTest, ReadWriteBlock) {
    BlockDevice* device = new BlockDevice("test_block_device.img", 4096);
    
    char write_buffer[4096];
    for (size_t i = 0; i < 4096; i++) {
        write_buffer[i] = static_cast<char>(i % 256);
    }
    for (size_t i = 0; i < 5; i++) {
        device->write_block(i, write_buffer);
    }

    for (size_t i = 0; i < 5; i++) {
    char read_buffer[4096];
        device->read_block(i, read_buffer);
        EXPECT_EQ(memcmp(write_buffer, read_buffer, 4096), 0);
    }
        
    delete device;
    std::remove("test_block_device.img");
}