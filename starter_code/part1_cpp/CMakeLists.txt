cmake_minimum_required(VERSION 3.15)
project(uvunwrap)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Eigen library
# Try system installation first
find_package(Eigen3 3.3 QUIET NO_MODULE)

if(Eigen3_FOUND)
    message(STATUS "Found system Eigen3: ${EIGEN3_INCLUDE_DIR}")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/eigen")
    message(STATUS "Using bundled Eigen from third_party/eigen")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/eigen)
else()
    message(WARNING "Eigen3 not found. Please either:")
    message(WARNING "  1. Install system Eigen: sudo apt-get install libeigen3-dev")
    message(WARNING "  2. Download Eigen to third_party/eigen/")
    message(WARNING "     wget https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz")
    message(WARNING "     tar xzf eigen-3.4.0.tar.gz -C third_party/")
    message(WARNING "     mv third_party/eigen-3.4.0 third_party/eigen")
endif()

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
