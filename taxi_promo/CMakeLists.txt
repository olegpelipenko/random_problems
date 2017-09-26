cmake_minimum_required(VERSION 2.6)
 
# Locate GTest
include_directories(${GTEST_INCLUDE_DIRS})
 
# Link runTests with what we want to test and the GTest and pthread library
project(taxi CXX)
add_executable(taxi taxi_promo.cpp main.cpp)
target_compile_features(taxi PRIVATE cxx_range_for)
target_link_libraries(taxi pthread mongoclient boost_thread boost_system boost_regex)