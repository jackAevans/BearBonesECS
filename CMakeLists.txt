cmake_minimum_required(VERSION 3.12)
project(ECS BEAR_BONES_ECS)

# Use C++17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directory
include_directories(include)

# Examples
add_executable(ecs_example examples/main.cpp)

# Tests
enable_testing()
add_executable(ecs_test tests/test.cpp)
add_test(NAME RunEcsTest COMMAND ecs_test)