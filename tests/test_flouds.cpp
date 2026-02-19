/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <gtest/gtest.h>
#include "../src/flouds/flouds.hpp"

TEST(FloudsTest, Create) {
    Flouds* flouds = create_flouds();
    EXPECT_NE(flouds, nullptr);
    EXPECT_TRUE(flouds->is_folder(0));
    EXPECT_FALSE(flouds->is_file(0));
    EXPECT_THROW(flouds->parent(0), std::out_of_range);
    EXPECT_THROW(flouds->parent(1), std::out_of_range);
    delete flouds;
}