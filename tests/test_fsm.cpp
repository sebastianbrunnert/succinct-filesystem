/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <gtest/gtest.h>
#include "../src/fsm/file_system_manager.hpp"

TEST(FileSystemManagerTest, Mount) {
    std::remove("test_fs.img");


    FileSystemManager* fsm = new FileSystemManager();
    EXPECT_NE(fsm, nullptr);
    fsm->mount("test_fs.img");
    delete fsm;


    FileSystemManager* fsm2 = new FileSystemManager();
    fsm2->mount("test_fs.img");
    delete fsm2;

    std::remove("test_fs.img");

}