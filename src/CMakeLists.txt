# Only build on Linux systems (FUSE requirement)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(WARNING "FUSE filesystem can only be built on Linux systems.")
    return()
endif()

add_executable(succinct_filesystem fuse.cpp)

# Place the executable directly in build/ instead of build/src/
set_target_properties(succinct_filesystem PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)