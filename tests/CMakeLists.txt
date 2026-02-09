# Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

# Automatically discover all test files matching test_*.cpp pattern
file(GLOB TEST_SOURCES "test_*.cpp")

include(GoogleTest)

foreach(TEST_SOURCE ${TEST_SOURCES})
    # Get the test name from the file name (without extension)
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    # Create executable for each test file
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} GTest::gtest_main)
    
    # Automatically discover and register tests with CTest
    gtest_discover_tests(${TEST_NAME})
endforeach()