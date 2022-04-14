include("${CMAKE_CURRENT_LIST_DIR}/macoroPreamble.cmake")

if(NOT MACORO_BUILD_DIR)
    set(MACORO_BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/../out/build/${MACORO_CONFIG}")
else()
    message(STATUS "MACORO_BUILD_DIR preset to ${MACORO_BUILD_DIR}")
endif()

if(DEFINED MACORO_BUILD AND NOT EXISTS "${MACORO_BUILD_DIR}")
    message(FATAL_ERROR "failed to find the macoro build directory. Looked at: ${MACORO_BUILD_DIR}")
endif()