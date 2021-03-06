cmake_policy(PUSH)
cmake_policy(SET CMP0057 NEW)
cmake_policy(SET CMP0045 NEW)
cmake_policy(SET CMP0074 NEW)

include("${CMAKE_CURRENT_LIST_DIR}/macoroPreamble.cmake")

if(NOT DEFINED MACORO_STAGE)

    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/Config.cmake.in)
        # we currenty are in the macoro source tree, macoro/cmake
		set(MACORO_STAGE "${CMAKE_CURRENT_LIST_DIR}/../out/install/${MACORO_CONFIG}")
    else()
        # we currenty are in install tree, <install-prefix>/lib/cmake/macoro
        set(MACORO_STAGE "${CMAKE_CURRENT_LIST_DIR}/../../..")
    endif()
endif()

if(DEFINED MACORO_BUILD)
    message(STATUS "Option: MACORO_STAGE = ${MACORO_STAGE}")
endif()

set(PUSHED_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
set(CMAKE_PREFIX_PATH "${MACORO_STAGE};${CMAKE_PREFIX_PATH}")

## optional-lite and variant-lite
###########################################################################
if(MACORO_OPTIONAL_LITE_V)
    if(MACORO_FETCH_AUTO AND NOT DEFINED MACORO_FETCH_OPTIONAL AND MACORO_BUILD)
        set(MACORO_FETCH_OPTIONAL ON)
    endif()
    
    if (MACORO_FETCH_OPTIONAL)
        find_package(optional-lite QUIET)
        include("${CMAKE_CURRENT_LIST_DIR}/../thirdparty/getOptionalLite.cmake")
    endif()
    find_package(optional-lite REQUIRED)
endif()

if(MACORO_VARIANT_LITE_V)
    if(MACORO_FETCH_AUTO AND NOT DEFINED MACORO_FETCH_VARIANT AND MACORO_BUILD)
        set(MACORO_FETCH_VARIANT ON)
    endif()
    if (MACORO_FETCH_VARIANT)
        find_package(variant-lite QUIET)
        include("${CMAKE_CURRENT_LIST_DIR}/../thirdparty/getVariantLite.cmake")
    endif()

    find_package(variant-lite REQUIRED)
endif()

# resort the previous prefix path
set(CMAKE_PREFIX_PATH ${PUSHED_CMAKE_PREFIX_PATH})
cmake_policy(POP)

find_package(Threads REQUIRED)