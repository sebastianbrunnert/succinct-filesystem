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

# Only build on Linux systems (FUSE requirement)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(WARNING "FUSE filesystem can only be built on Linux systems.")
    return()
endif()

add_executable(succinct_filesystem fuse.cpp fuse.cpp)

# Place the executable directly in build/ instead of build/src/
set_target_properties(succinct_filesystem PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)