cmake_policy(PUSH)
cmake_policy(SET CMP0057 NEW)
cmake_policy(SET CMP0045 NEW)
cmake_policy(SET CMP0074 NEW)

if(MSVC)
	set(MACORO_CONFIG "x64-${CMAKE_BUILD_TYPE}")
elseif(APPLE)
	set(MACORO_CONFIG "osx")
else()
	set(MACORO_CONFIG "linux")
endif()

if(NOT DEFINED MACORO_THIRDPARTY_HINT)

    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/MACOROFindBuildDir.cmake)
        # we currenty are in the macoro source tree, macoro/cmake
		set(MACORO_THIRDPARTY_HINT "${CMAKE_CURRENT_LIST_DIR}/../out/install/${MACORO_CONFIG}")
    else()
        # we currenty are in install tree, <install-prefix>/lib/cmake/macoro
        set(MACORO_THIRDPARTY_HINT "${CMAKE_CURRENT_LIST_DIR}/../../..")
    endif()
endif()

if(NOT MACORO_FIND_QUIETLY)
    message(STATUS "Option: MACORO_THIRDPARTY_HINT = ${MACORO_THIRDPARTY_HINT}")
endif()

set(PUSHED_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
set(CMAKE_PREFIX_PATH "${MACORO_THIRDPARTY_HINT};${CMAKE_PREFIX_PATH}")

## optional-lite and variant-lite
###########################################################################
if(NOT MACORO_CPP_20)
    if(MACORO_FETCH_AUTO AND NOT DEFINED MACORO_FETCH_OPTIONAL AND MACORO_BUILD)
        set(MACORO_FETCH_OPTIONAL ON)
    endif()
    if(MACORO_FETCH_AUTO AND NOT DEFINED MACORO_FETCH_VARIANT AND MACORO_BUILD)
        set(MACORO_FETCH_VARIANT ON)
    endif()
    
    if (MACORO_FETCH_OPTIONAL)
        find_package(optional-lite QUIET)
        include("${CMAKE_CURRENT_LIST_DIR}/../thirdparty/getOptionalLite.cmake")
    endif()
    
    if (MACORO_FETCH_VARIANT)
        find_package(variant-lite QUIET)
        include("${CMAKE_CURRENT_LIST_DIR}/../thirdparty/getVariantLite.cmake")
    endif()


    find_package(optional-lite REQUIRED)
    find_package(variant-lite REQUIRED)
endif()

# resort the previous prefix path
set(CMAKE_PREFIX_PATH ${PUSHED_CMAKE_PREFIX_PATH})
cmake_policy(POP)

find_package(Threads REQUIRED)