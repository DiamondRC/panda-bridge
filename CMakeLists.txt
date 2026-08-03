cmake_minimum_required(VERSION 3.28)

project(lqr_bridge
    VERSION 0.1.0
    DESCRIPTION "Low-latency PS->PL gain/weight bridge for the PandA LQR controller"
    LANGUAGES CXX)

# C++26, no GNU extensions
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(warnings)     # -> lqr::warnings
include(sanitizers)   # -> lqr::sanitizer

# Test framework (doctest, single header)
add_library(doctest INTERFACE)
target_include_directories(doctest SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
add_library(doctest::doctest ALIAS doctest)

# The bridge itself
add_library(lqrbridge INTERFACE)
target_include_directories(lqrbridge INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_library(lqr::bridge ALIAS lqrbridge)

# Tests
enable_testing()
add_subdirectory(test)
