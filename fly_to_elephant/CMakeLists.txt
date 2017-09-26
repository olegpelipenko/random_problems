
project(fly_tests CXX)
cmake_minimum_required(VERSION 2.6)

enable_testing()
find_package(GTest REQUIRED)

#SET(GTEST_INCLUDE_DIR C:/dev/googletest/googletest/include)
#SET(GTEST_LIBRARY C:/dev/googletest/googletest/msvc/gtest/Debug/gtestd.lib)
#SET(GTEST_MAIN_LIBRARY C:/dev/googletest/googletest/msvc/gtest/Debug/gtest_maind.lib)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(fly_tests fly_tests.cpp helpers.h words_graph.h)
target_link_libraries(fly_tests ${GTEST_BOTH_LIBRARIES} pthread)
target_compile_features(fly_tests PRIVATE cxx_range_for)

# Link runTests with what we want to test and the GTest and pthread library
project(fly_to_elephant CXX)
add_executable(fly_to_elephant main.cpp helpers.h words_graph.h dijkstra.h)

set(CMAKE_BUILD_TYPE Debug)
target_compile_features(fly_to_elephant PRIVATE cxx_range_for)
