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

TEST(FloudsTest, ComplexCase) {
    Flouds* flouds = create_flouds();
    
    flouds->insert(0, "folder1", true);
    flouds->insert(0, "folder2", true);
    flouds->insert(0, "folder3", true);
    
    size_t folder1 = flouds->path("/folder1");
    flouds->insert(folder1, "file1", false);
    flouds->insert(folder1, "file2", false);
    flouds->insert(folder1, "file3", false);
    
    size_t folder2 = flouds->path("/folder2");
    flouds->insert(folder2, "subfolder1", true);
    flouds->insert(folder2, "file4", false);
    
    size_t subfolder1 = flouds->path("/folder2/subfolder1");
    flouds->insert(subfolder1, "file5", false);
    flouds->insert(subfolder1, "file6", false);
    flouds->insert(subfolder1, "deepfolder", true);
    
    size_t deepfolder = flouds->path("/folder2/subfolder1/deepfolder");
    flouds->insert(deepfolder, "file7", false);
    
    size_t folder3 = flouds->path("/folder3");
    flouds->insert(folder3, "file8", false);

    /*
    root
        folder1
            file1
            file2
            file3
        folder2
            subfolder1
                file5
                file6
                deepfolder
                    file7
            file4
        folder3
            file8
    */

    // Verify initial structure
    EXPECT_EQ(flouds->children_count(0), 3);
    folder1 = flouds->path("/folder1");
    folder2 = flouds->path("/folder2");
    folder3 = flouds->path("/folder3");
    subfolder1 = flouds->path("/folder2/subfolder1");
    deepfolder = flouds->path("/folder2/subfolder1/deepfolder");
    
    EXPECT_EQ(flouds->children_count(folder1), 3);
    EXPECT_EQ(flouds->children_count(folder2), 2);
    EXPECT_EQ(flouds->children_count(folder3), 1);
    EXPECT_EQ(flouds->children_count(subfolder1), 3);
    EXPECT_EQ(flouds->children_count(deepfolder), 1);
    
    // Verify names
    EXPECT_EQ(flouds->get_name(folder1), "folder1");
    EXPECT_EQ(flouds->get_name(flouds->child(folder1, 0)), "file1");
    EXPECT_EQ(flouds->get_name(deepfolder), "deepfolder");
    
    // Verify types
    EXPECT_TRUE(flouds->is_folder(folder1));
    EXPECT_TRUE(flouds->is_file(flouds->child(folder1, 0)));
    EXPECT_FALSE(flouds->is_empty_folder(folder1));
    
    // Remove file from middle of folder1 (file2)
    folder1 = flouds->path("/folder1");
    flouds->remove(flouds->child(folder1, 1));
    folder1 = flouds->path("/folder1");
    EXPECT_EQ(flouds->children_count(folder1), 2);
    
    // After removal, folder1's children should now be file1 and file3
    EXPECT_EQ(flouds->get_name(flouds->child(folder1, 0)), "file1");
    EXPECT_EQ(flouds->get_name(flouds->child(folder1, 1)), "file3");
    
    // Remove deepest file
    deepfolder = flouds->path("/folder2/subfolder1/deepfolder");
    flouds->remove(flouds->child(deepfolder, 0));
    
    // deepfolder should now be empty
    deepfolder = flouds->path("/folder2/subfolder1/deepfolder");
    EXPECT_EQ(flouds->children_count(deepfolder), 0);
    EXPECT_TRUE(flouds->is_empty_folder(deepfolder));
    
    // Remove the now-empty deepfolder
    subfolder1 = flouds->path("/folder2/subfolder1");
    deepfolder = flouds->child(subfolder1, 2);  // It's the 3rd child
    flouds->remove(deepfolder);
    subfolder1 = flouds->path("/folder2/subfolder1");
    EXPECT_EQ(flouds->children_count(subfolder1), 2);
    
    // Remove all files from folder3
    folder3 = flouds->path("/folder3");
    flouds->remove(flouds->child(folder3, 0));
    folder3 = flouds->path("/folder3");
    EXPECT_EQ(flouds->children_count(folder3), 0);
    EXPECT_TRUE(flouds->is_empty_folder(folder3));
    
    // Add a new file to an existing folder after removals
    folder1 = flouds->path("/folder1");
    flouds->insert(folder1, "new_file", false);
    folder1 = flouds->path("/folder1");
    EXPECT_EQ(flouds->children_count(folder1), 3);
    EXPECT_EQ(flouds->get_name(flouds->child(folder1, 2)), "new_file");
    
    // Add file to previously empty folder
    folder3 = flouds->path("/folder3");
    EXPECT_TRUE(flouds->is_empty_folder(folder3));
    flouds->insert(folder3, "recovered", false);
    folder3 = flouds->path("/folder3");
    EXPECT_FALSE(flouds->is_empty_folder(folder3));
    EXPECT_EQ(flouds->children_count(folder3), 1);
    
    // Remove first child from folder1 (should test the first-child special case)
    folder1 = flouds->path("/folder1");
    flouds->remove(flouds->child(folder1, 0));
    folder1 = flouds->path("/folder1");
    EXPECT_EQ(flouds->children_count(folder1), 2);
    
    // Verify parent relationships still work
    folder1 = flouds->path("/folder1");
    size_t child = flouds->child(folder1, 0);
    EXPECT_EQ(flouds->parent(child), folder1);
    
    // Verify paths still work after all modifications
    EXPECT_EQ(flouds->path("/"), 0);
    EXPECT_NO_THROW(flouds->path("/folder1"));
    EXPECT_NO_THROW(flouds->path("/folder2/subfolder1"));
    EXPECT_NO_THROW(flouds->path("/folder3/recovered"));
    
    // Verify invalid paths throw
    EXPECT_THROW(flouds->path("/nonexistent"), std::out_of_range);
    EXPECT_THROW(flouds->path("/folder2/subfolder1/deepfolder"), std::out_of_range);
    
    delete flouds;
}