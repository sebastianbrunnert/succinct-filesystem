/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <gtest/gtest.h>
#include "../src/fsm/delta/delta_stabilization.hpp"

TEST(DeltaStabilizationTest, BasicRoundTrip) {
    DeltaStabilization ds;

    // Test basic round-trip at initial version
    for (size_t flouds_pos = 0; flouds_pos < 100; flouds_pos++) {
        uint64_t stable_ino = ds.flouds_inode_to_stable_inode(flouds_pos);
        size_t extracted_pos = ds.stable_inode_to_flouds_inode(stable_ino);
        EXPECT_EQ(flouds_pos, extracted_pos);
    }
}

TEST(DeltaStabilizationTest, InsertShiftsPositions) {
    DeltaStabilization ds;

    // Create inodes for positions 0-9 at version 1
    uint64_t inodes[10];
    for (size_t i = 0; i < 10; i++) {
        inodes[i] = ds.flouds_inode_to_stable_inode(i);
    }
    
    // Insert at position 5 (version becomes 2)
    ds.record_insert(5);
    
    // Positions 0-4 should stay at 0-4
    for (size_t i = 0; i < 5; i++) {
        EXPECT_EQ(i, ds.stable_inode_to_flouds_inode(inodes[i]));
    }
    
    // Positions 5-9 should shift to 6-10
    for (size_t i = 5; i < 10; i++) {
        EXPECT_EQ(i + 1, ds.stable_inode_to_flouds_inode(inodes[i]));
    }
    
    // Inode numbers themselves are unchanged
    for (size_t i = 0; i < 10; i++) {
        uint64_t expected = (1ULL << 48) | i;
        EXPECT_EQ(expected, inodes[i]);
    }
}

TEST(DeltaStabilizationTest, RemoveShiftsPositions) {
    DeltaStabilization ds;

    // Create inodes for positions 0-9 at version 1
    uint64_t inodes[10];
    for (size_t i = 0; i < 10; i++) {
        inodes[i] = ds.flouds_inode_to_stable_inode(i);
    }
    
    // Remove at position 5 (version becomes 2)
    ds.record_remove(5);
    
    // Positions 0-4 should stay at 0-4
    for (size_t i = 0; i < 5; i++) {
        EXPECT_EQ(i, ds.stable_inode_to_flouds_inode(inodes[i]));
    }
    
    // Position 5 was deleted (returns sentinel)
    EXPECT_EQ(static_cast<size_t>(-1), ds.stable_inode_to_flouds_inode(inodes[5]));
    
    // Positions 6-9 should shift to 5-8
    for (size_t i = 6; i < 10; i++) {
        EXPECT_EQ(i - 1, ds.stable_inode_to_flouds_inode(inodes[i]));
    }
}

TEST(DeltaStabilizationTest, MultipleOperations) {
    DeltaStabilization ds;

    // Position 10 at version 1
    uint64_t ino_10 = ds.flouds_inode_to_stable_inode(10);
    
    // Insert at position 5: position 10 → 11
    ds.record_insert(5);
    EXPECT_EQ(11u, ds.stable_inode_to_flouds_inode(ino_10));
    
    // Insert at position 3: position 11 → 12
    ds.record_insert(3);
    EXPECT_EQ(12u, ds.stable_inode_to_flouds_inode(ino_10));
    
    // Remove at position 8: position 12 → 11
    ds.record_remove(8);
    EXPECT_EQ(11u, ds.stable_inode_to_flouds_inode(ino_10));
    
    // Inode number never changes (enables FUSE caching)
    EXPECT_EQ((1ULL << 48) | 10, ino_10);
}

TEST(DeltaStabilizationTest, NewFilesGetCurrentVersion) {
    DeltaStabilization ds;

    // Initial version is 1
    EXPECT_EQ(1u, ds.get_version());
    uint64_t ino_v1 = ds.flouds_inode_to_stable_inode(5);
    EXPECT_EQ((1ULL << 48) | 5, ino_v1);
    
    // After insert, version is 2
    ds.record_insert(3);
    EXPECT_EQ(2u, ds.get_version());
    uint64_t ino_v2 = ds.flouds_inode_to_stable_inode(5);
    EXPECT_EQ((2ULL << 48) | 5, ino_v2);
    
    // After remove, version is 3
    ds.record_remove(7);
    EXPECT_EQ(3u, ds.get_version());
    uint64_t ino_v3 = ds.flouds_inode_to_stable_inode(5);
    EXPECT_EQ((3ULL << 48) | 5, ino_v3);
}