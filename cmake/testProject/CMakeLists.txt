
cmake_minimum_required (VERSION 3.15)

project (TestProj VERSION  1.0.0)

add_executable(main "src.cpp")

message("CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH}")

find_package(macoro REQUIRED HINTS ${MACORO_HINT})

target_link_libraries(main macoro::macoro)


option(MACORO_CPP_VER "cpp version" 20)

if(MSVC)
    target_compile_options( main PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/std=c++${MACORO_CPP_VER}>
    )
else()
    target_compile_options( main PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++${MACORO_CPP_VER}>
    )
endif()