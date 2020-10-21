cmake_minimum_required(VERSION 3.8)
project(BigramsTrigramsParallel)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c)
add_executable(BigramsTrigramsParallel ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(BigramsTrigramsParallel pthread)