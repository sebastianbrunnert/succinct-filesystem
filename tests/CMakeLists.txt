# This file is part of the Succinct Filesystem project.
#
# Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
# SPDX-License-Identifier: GPL-2.0-only

# Automatically discover all test files matching test_*.cpp pattern
file(GLOB TEST_SOURCES "test_*.cpp")

include(GoogleTest)

foreach(TEST_SOURCE ${TEST_SOURCES})
    # Get the test name from the file name (without extension)
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    # Create executable for each test file
    add_executable(${TEST_NAME} 
        ${TEST_SOURCE}
        ${CMAKE_SOURCE_DIR}/src/bitvector/array_bitvector.cpp
        ${CMAKE_SOURCE_DIR}/src/bitvector/word_bitvector.cpp
    )
    
    target_link_libraries(${TEST_NAME} GTest::gtest_main)
    
    # Automatically discover and register tests with CTest
    gtest_discover_tests(${TEST_NAME})
endforeach()