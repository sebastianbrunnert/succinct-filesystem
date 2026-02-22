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
    EXPECT_TRUE(flouds->is_empty_folder(0));
    delete flouds;
}

/*
TEST(FloudsTest, InsertAndGet) {
    Flouds* flouds = create_flouds();
    flouds->insert(0, "folder1", true);
    flouds->insert(0, "file1", false);
    EXPECT_EQ(flouds->children_count(0), 2);
    EXPECT_EQ(flouds->child(0, 0), 1);
    EXPECT_EQ(flouds->child(0, 1), 2);
    EXPECT_EQ(flouds->get_name(1), "folder1");
    EXPECT_EQ(flouds->get_name(2), "file1");
    EXPECT_TRUE(flouds->is_folder(1));
    EXPECT_TRUE(flouds->is_empty_folder(1));
    EXPECT_FALSE(flouds->is_file(1));
    EXPECT_FALSE(flouds->is_folder(2));
    EXPECT_TRUE(flouds->is_empty_folder(2));
    EXPECT_TRUE(flouds->is_file(2));
    EXPECT_THROW(flouds->child(0, 2), std::out_of_range);

    delete flouds;
}
*/

TEST(FloudsTest, ChildrenCountBasicNested) {
    Flouds* flouds = create_flouds();
    EXPECT_EQ(flouds->children_count(0), 0);
    flouds->insert(0, "folder1", true);
    EXPECT_EQ(flouds->children_count(0), 1);
    flouds->insert(0, "file1", false);
    EXPECT_EQ(flouds->children_count(0), 2);
    EXPECT_EQ(flouds->children_count(1), 0);
    flouds->insert(1, "file2", false);
    std::cout << "NESTEDFLOUDS:" << std::endl;
    std::cout << *flouds << std::endl;
    EXPECT_EQ(flouds->children_count(1), 1);
    delete flouds;
}

TEST(FloudsTest, ChildrenCountBasic) {
    Flouds* flouds = create_flouds();
    EXPECT_EQ(flouds->children_count(0), 0);
    flouds->insert(0, "folder1", true);
    EXPECT_EQ(flouds->children_count(0), 1);
    flouds->insert(0, "file1", false);
    EXPECT_EQ(flouds->children_count(0), 2);
    EXPECT_EQ(flouds->children_count(1), 0);
    delete flouds;
}