cmake_minimum_required(VERSION 2.6)
 
# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
 
# Link runTests with what we want to test and the GTest and pthread library
add_executable(runTests tests.cpp ../taxi_promo.cpp)
target_link_libraries(runTests ${GTEST_LIBRARIES} pthread mongoclient boost_thread boost_system boost_regex)
target_compile_features(runTests PRIVATE cxx_range_for)
