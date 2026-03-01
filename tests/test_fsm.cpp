/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <gtest/gtest.h>
#include "../src/fsm/file_system_manager.hpp"

TEST(FileSystemManagerTest, Mount) {
    FileSystemManager* fsm = new FileSystemManager();
    EXPECT_NE(fsm, nullptr);
    fsm->mount("test_fs.img");
    delete fsm;

    FileSystemManager* fsm2 = new FileSystemManager();
    EXPECT_NE(fsm, nullptr);
    fsm2->mount("test_fs.img");
    delete fsm2;

    std::remove("test_fs.img");
}

TEST(FileSystemManagerTest, AddNodeSave) {
    FileSystemManager* fsm = new FileSystemManager();
    fsm->mount("test_fs.img");
    fsm->add_node(0, "test_file.txt", false, 0644);
    fsm->save();
    delete fsm;

    FileSystemManager* fsm2 = new FileSystemManager();
    fsm2->mount("test_fs.img");
    Flouds* flouds = fsm2->get_flouds();
    EXPECT_EQ(flouds->children_count(0), 1);
    EXPECT_EQ(flouds->get_name(flouds->child(0, 0)), "test_file.txt");
    delete fsm2;

    std::remove("test_fs.img");
}

TEST(FileSystemManagerTest, AddNodeRemoveNode) {
    FileSystemManager* fsm = new FileSystemManager();
    fsm->mount("test_fs.img");
    fsm->add_node(0, "test_file.txt", false, 0644);
    Flouds* flouds = fsm->get_flouds();
    size_t node_id = flouds->child(0, 0);
    EXPECT_EQ(flouds->children_count(0), 1);
    EXPECT_EQ(flouds->get_name(node_id), "test_file.txt");

    fsm->remove_node(node_id);
    EXPECT_EQ(flouds->children_count(0), 0);

    delete fsm;
    std::remove("test_fs.img");
}

TEST(FileSystemManagerTest, Metadata) {
    FileSystemManager* fsm = new FileSystemManager();
    fsm->mount("test_fs.img");
    fsm->add_node(0, "test_file.txt", false, 0644);
    Flouds* flouds = fsm->get_flouds();
    size_t node_id = flouds->child(0, 0);

    fsm->set_file_size(node_id, 1024);
    EXPECT_EQ(fsm->get_inode(node_id)->size, 1024);

    delete fsm;
    std::remove("test_fs.img");
}