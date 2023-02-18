

include("${CMAKE_CURRENT_LIST_DIR}/macoroPreamble.cmake")


set(MACORO_BUILD ON)


option(MACORO_FETCH_AUTO "" OFF)
option(MACORO_PIC "build with -FPIC on unix" OFF)
option(MACORO_INSTALL_THIRDPARTY "dont install third party" ON)
option(MACORO_ASAN "build with asan" OFF)

if(NOT DEFINED MACORO_THIRDPARTY_CLONE_DIR)
    set(MACORO_THIRDPARTY_CLONE_DIR "${CMAKE_CURRENT_LIST_DIR}/../out/")
endif()

if(NOT DEFINED MACORO_CPP_VER)
    set(MACORO_CPP_VER 14)
endif()
if(${MACORO_CPP_VER} EQUAL 20)
    set(MACORO_CPP_20 ON)
else()
    set(MACORO_CPP_20 OFF)
endif()

if(DEFINED MACORO_OPTIONAL_LITE)
    set(MACORO_OPTIONAL_LITE_V ${MACORO_OPTIONAL_LITE})
else()  
    if(${MACORO_CPP_VER} EQUAL 14)
        set(MACORO_OPTIONAL_LITE_V ON)
    else()
        set(MACORO_OPTIONAL_LITE_V OFF)
    endif()
endif()
if(DEFINED MACORO_VARIANT_LITE)
    set(MACORO_VARIANT_LITE_V ${MACORO_VARIANT_LITE})
else()

    if(${MACORO_CPP_VER} EQUAL 14)
        set(MACORO_VARIANT_LITE_V ON)
    else()
        set(MACORO_VARIANT_LITE_V OFF)
    endif()
endif()


message("\nOptions\n---------------------------")

message("CMAKE_BUILD_TYPE       = ${CMAKE_BUILD_TYPE}\t  ~ build type")
message("MACORO_NO_SYSTEM_PATH  = ${MACORO_NO_SYSTEM_PATH}\t  ~ do not look in system paths for dependencies")
message("MACORO_FETCH_AUTO      = ${MACORO_FETCH_AUTO}\t  ~ automatically fetch dependencies as needed")
message("MACORO_FETCH_OPTIONAL  = ${MACORO_FETCH_OPTIONAL}  \t  ~ fetch optional-lite")
message("MACORO_FETCH_VARIANT   = ${MACORO_FETCH_VARIANT}  \t  ~ fetch variant-lite")
message("MACORO_CPP_VER         = ${MACORO_CPP_VER}\t  ~ cpp standard version")

message("MACORO_OPTIONAL_LITE   =${MACORO_OPTIONAL_LITE_V}\t  ~ use optional lite")
message("MACORO_VARIANT_LITE    =${MACORO_VARIANT_LITE_V}\t  ~ use variant lite")
message("MACORO_PIC             =${MACORO_PIC}\t  ~ compile with -fPIC on unix")
message("MACORO_ASAN            =${MACORO_ASAN}\t  ~ compile with asan")

message("---------------------------\n")