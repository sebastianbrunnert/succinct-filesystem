/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>

#include "../src/fsm/delta/delta_stabilization.hpp"

TEST(DeltaStabilizationTest, FloudsToStableAndBack) {
    #ifdef DELTA_STABILIZATION
    DeltaStabilization delta_stabilization;

    uint64_t stable_inode = delta_stabilization.flouds_inode_to_stable_inode(5);
    auto resolved_inode = delta_stabilization.stable_inode_to_flouds_inode(stable_inode);
    ASSERT_TRUE(resolved_inode.has_value());
    EXPECT_EQ(*resolved_inode, 5);

    delta_stabilization.record_remove(5);

    EXPECT_FALSE(delta_stabilization.stable_inode_to_flouds_inode(stable_inode).has_value());
    #endif
}

TEST(DeltaStabilizationTest, Overflow) {
    #ifdef DELTA_STABILIZATION
    DeltaStabilization delta_stabilization;

    uint64_t stale_inode = delta_stabilization.flouds_inode_to_stable_inode(100);

    delta_stabilization.record_insert(0);
    uint64_t oldest_valid_inode = delta_stabilization.flouds_inode_to_stable_inode(100);

    for (size_t i = 0; i < 256; i++) {
        delta_stabilization.record_insert(0);
    }

    EXPECT_FALSE(delta_stabilization.stable_inode_to_flouds_inode(stale_inode).has_value());

    auto resolved_inode = delta_stabilization.stable_inode_to_flouds_inode(oldest_valid_inode);
    ASSERT_TRUE(resolved_inode.has_value());
    EXPECT_EQ(*resolved_inode, 356);
    #endif
}