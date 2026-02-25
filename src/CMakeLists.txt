# This file is part of the Succinct Filesystem project.
#
# Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
# SPDX-License-Identifier: GPL-2.0-only

# Only build on Linux systems (FUSE requirement)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(WARNING "FUSE filesystem can only be built on Linux systems.")
    return()
endif()

# Find FUSE3 library using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE3 REQUIRED fuse3)

add_executable(succinct_filesystem 
    fuse.cpp
    bitvector/array_bitvector.cpp
    bitvector/word_bitvector.cpp
    bitvector/saskeli_bitvector.cpp
    bitvector/adaptive_bitvector.cpp
    name_sequence/array_name_sequence.cpp
    name_sequence/concatenated_name_sequence.cpp
    name_sequence/immer_name_sequence.cpp
    name_sequence/map_name_sequence.cpp
    flouds/flouds.cpp
    block_device/block_device.cpp
    fsm/allocation/best_fit_allocation.cpp
    fsm/file_system_manager.cpp
)

# Link FUSE3 library
target_link_libraries(succinct_filesystem ${FUSE3_LIBRARIES})
target_include_directories(succinct_filesystem PRIVATE ${FUSE3_INCLUDE_DIRS})
target_compile_options(succinct_filesystem PRIVATE ${FUSE3_CFLAGS_OTHER})

# Place the executable directly in build/ instead of build/src/
set_target_properties(succinct_filesystem PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)