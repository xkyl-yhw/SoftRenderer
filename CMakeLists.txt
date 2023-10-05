cmake_minimum_required(VERSION 3.5)
project(SoftRenderer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
STRING(REPLACE "-O2" "-Od" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

find_package(SDL2 CONFIG REQUIRED)

add_subdirectory(Version1)
add_subdirectory(Version1_1)
add_subdirectory(Version1_2)
add_subdirectory(Version1_3)
add_subdirectory(Version2_1)