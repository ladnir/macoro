
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(MSVC)
    if("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
        set(MACORO_CONFIG "x64-Release")
    else()
        set(MACORO_CONFIG "x64-${CMAKE_BUILD_TYPE}")
    endif()
elseif(APPLE)
    set(MACORO_CONFIG "osx")
else()
    set(MACORO_CONFIG "linux")
endif()

