cmake_minimum_required(VERSION 2.8)

project(function)
add_executable(${PROJECT_NAME}
    "function.hpp"
    "gtest/gtest_main.cc"
    "gtest/gtest-all.cc"
    "gtest/gtest.h"
    "test.cpp"
    )
set(CMAKE_CXX_STANDARD 17)

target_link_libraries(${PROJECT_NAME} -DGLIBCXX_DEBUG -lpthread -fsanitize=undefined,address)
