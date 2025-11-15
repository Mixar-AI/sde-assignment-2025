cmake_minimum_required(VERSION 3.15)
project(uvunwrap)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Eigen library (bundled in third_party/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/eigen)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/mesh_io.cpp
    src/math_utils.cpp
    src/topology.cpp
    src/seam_detection.cpp
    src/lscm.cpp
    src/packing.cpp
    src/unwrap.cpp
)

# Main library
add_library(uvunwrap SHARED ${SOURCES})

# Test executable
add_executable(test_unwrap tests/test_unwrap.cpp)
target_link_libraries(test_unwrap uvunwrap)

# Enable warnings
if(MSVC)
    target_compile_options(uvunwrap PRIVATE /W4)
else()
    target_compile_options(uvunwrap PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install targets
install(TARGETS uvunwrap
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin)

install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")
