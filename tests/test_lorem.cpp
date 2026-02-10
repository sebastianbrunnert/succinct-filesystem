/*
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <gtest/gtest.h>

TEST(LoremTest, BasicAssertions) {
    EXPECT_EQ(7 * 6, 42);
}

TEST(LoremTest, AdvancedAssertions) {
    EXPECT_NE(7 * 6, 43);
}