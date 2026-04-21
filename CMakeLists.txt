
cmake_minimum_required(VERSION 3.15)
project(macoro VERSION 0.1.3)
include(cmake/macoroBuildOptions.cmake)
include(cmake/macoroFindDeps.cmake)

add_subdirectory(macoro)

if(MACORO_TESTS)
	add_subdirectory(tests)
	add_subdirectory(frontend)
endif()
