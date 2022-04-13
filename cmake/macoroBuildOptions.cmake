


set(MACORO_BUILD ON)
message("\nOptions\n---------------------------")

option(MACORO_FETCH_AUTO OFF)
message("MACORO_FETCH_AUTO      = ${MACORO_FETCH_AUTO}\t  ~ automatically fetch dependencies as needed")
message("MACORO_FETCH_OPTIONAL  = ${MACORO_FETCH_OPTIONAL}  \t  ~ fetch optional-lite")
message("MACORO_FETCH_VARIANT   = ${MACORO_FETCH_VARIANT}  \t  ~ fetch variant-lite")

if(NOT DEFINED MACORO_CPP_VER)
    set(MACORO_CPP_VER 14)
endif()
message("MACORO_CPP_VER         = ${MACORO_CPP_VER}\t  ~ cpp standard version")
#message("MACORO_OPTIONAL_LITE   = ${MACORO_OPTIONAL_LITE}\t  ~ build optional-lite, default ON if not c++ 20")
#message("MACORO_VARIANT_LITE    = ${MACORO_VARIANT_LITE}\t  ~ build variant-lite, default ON if not c++ 20")

message("---------------------------\n")

if(${MACORO_CPP_VER} EQUAL 20)
    set(MACORO_CPP_20 ON)
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

