/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <gtest/gtest.h>
#include "../src/flouds/flouds.hpp"
#include <random>
#include <vector>
#include <tuple>

TEST(FloudsTest, Create) {
    Flouds* flouds = create_flouds();
    EXPECT_NE(flouds, nullptr);
    EXPECT_TRUE(flouds->is_folder(0));
    EXPECT_FALSE(flouds->is_file(0));
    EXPECT_TRUE(flouds->is_empty_folder(0));
    delete flouds;
}

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
    EXPECT_FALSE(flouds->is_empty_folder(2));
    EXPECT_TRUE(flouds->is_file(2));
    
    delete flouds;
}

TEST(FloudsTest, ChildrenCount) {
    Flouds* flouds = create_flouds();
    EXPECT_EQ(flouds->children_count(0), 0);
    flouds->insert(0, "folder1", true);
    EXPECT_EQ(flouds->children_count(0), 1);
    flouds->insert(0, "file1", false);
    EXPECT_EQ(flouds->children_count(0), 2);
    EXPECT_EQ(flouds->children_count(1), 0);
    flouds->insert(1, "file2", false);
    EXPECT_EQ(flouds->children_count(1), 1);
    size_t folder2 = flouds->insert(1, "folder2", true);
    EXPECT_EQ(flouds->children_count(1), 2);
    EXPECT_EQ(flouds->children_count(folder2), 0);
    flouds->insert(folder2, "file3", false);
    EXPECT_EQ(flouds->children_count(folder2), 1);
    delete flouds;
}

TEST(FloudsTest, Parent) {
    Flouds* flouds = create_flouds();
    size_t folder1 = flouds->insert(0, "folder1", true);
    size_t file1 = flouds->insert(0, "file1", false);
    size_t file2 = flouds->insert(folder1, "file2", false);

    EXPECT_EQ(flouds->parent(folder1), 0);
    EXPECT_EQ(flouds->parent(file1), 0);
    EXPECT_EQ(flouds->parent(file2), folder1);
    
    delete flouds;
}

TEST(FloudsTest, Remove) {
    Flouds* flouds = create_flouds();
    size_t folder1 = flouds->insert(0, "folder1", true);
    size_t file1 = flouds->insert(0, "file1", false);
    size_t file2 = flouds->insert(folder1, "file2", false);

    EXPECT_EQ(flouds->children_count(0), 2);
    EXPECT_EQ(flouds->children_count(folder1), 1);
    flouds->remove(file2);

    EXPECT_EQ(flouds->children_count(folder1), 0);
    EXPECT_TRUE(flouds->is_empty_folder(folder1));
    
    flouds->remove(file1);
    EXPECT_EQ(flouds->children_count(0), 1);
    
    flouds->remove(folder1);
    EXPECT_EQ(flouds->children_count(0), 0);
    
    delete flouds;
}

TEST(FloudsTest, Path) {
    Flouds* flouds = create_flouds();
    size_t folder1 = flouds->insert(0, "folder1", true);
    size_t folder2 = flouds->insert(folder1, "folder2", true);
    size_t file1 = flouds->insert(folder2, "file1", false);

    EXPECT_EQ(flouds->path("/"), 0);
    EXPECT_EQ(flouds->path("/folder1"), folder1);
    EXPECT_EQ(flouds->path("/folder1/folder2"), folder2);
    EXPECT_EQ(flouds->path("/folder1/folder2/file1"), file1);
    
    delete flouds;
}