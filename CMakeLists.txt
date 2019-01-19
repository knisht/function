cmake_minimum_required(VERSION 2.8)

project(function)
add_executable(${PROJECT_NAME}
    "function.hpp"
    "gtest/gtest_main.cc"
    "gtest/gtest-all.cc"
    "gtest/gtest.h"
    "test.cpp")

target_link_libraries(${PROJECT_NAME} -lpthread -fsanitize=undefined,address)
