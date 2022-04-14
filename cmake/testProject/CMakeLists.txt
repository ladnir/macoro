
cmake_minimum_required (VERSION 3.15)

project (TestProj VERSION  1.0.0)

add_executable(main "src.cpp")

message("CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH}")

find_package(macoro REQUIRED HINTS ${MACORO_HINT})

target_link_libraries(main macoro::macoro)

