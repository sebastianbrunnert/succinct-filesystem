/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <gtest/gtest.h>
#include "../src/fsm/delta/delta_stabilization.hpp"
#include <random>

/*
TEST(DeltaStabilizationTest, Inversion) {
    DeltaStabilization ds;

    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(2, 99);
    std::bernoulli_distribution bool_dist(0.5);

    for (size_t i = 0; i < 100; i++) {
        size_t stable_inode = dist(rng);
        if (bool_dist(rng)) {
            ds.record_insert(stable_inode);
        } else {
            ds.record_remove(stable_inode);
        }
    }

    for (size_t i = 50; i <= 60; i++) {
        size_t flouds_inode = ds.stable_inode_to_flouds_inode(i);
        size_t stable_inode = ds.flouds_inode_to_stable_inode(flouds_inode);
        EXPECT_EQ(i, stable_inode);
    }

}
*/