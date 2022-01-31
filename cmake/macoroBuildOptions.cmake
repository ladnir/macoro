


set(MACORO_BUILD ON)
message("\nOptions\n---------------------------")

option(MACORO_FETCH_AUTO OFF)
message("MACORO_FETCH_AUTO      = ${MACORO_FETCH_AUTO}\t  ~ automatically fetch dependencies as needed")
message("MACORO_FETCH_OPTIONAL  = ${MACORO_FETCH_OPTIONAL}  \t  ~ fetch optional-lite")
message("MACORO_FETCH_VARIANT   = ${MACORO_FETCH_VARIANT}  \t  ~ fetch variant-lite")


option(MACORO_CPP_20 OFF)
message("MACORO_CPP_20          = ${MACORO_CPP_20}\t  ~ build the library with C++ 20 coroutines")
message("MACORO_OPTIONAL_LITE   = ${MACORO_OPTIONAL_LITE}\t  ~ build optional-lite, default ON if not c++ 20")
message("MACORO_VARIANT_LITE    = ${MACORO_VARIANT_LITE}\t  ~ build variant-lite, default ON if not c++ 20")

message("---------------------------\n")


if(DEFINED MACORO_OPTIONAL_LITE)
    set(MACORO_OPTIONAL_LITE_V ${MACORO_OPTIONAL_LITE})
else()  
    if(${MACORO_CPP_20})
        set(MACORO_OPTIONAL_LITE_V OFF)
    else()
        set(MACORO_OPTIONAL_LITE_V ON)
    endif()
endif()
if(DEFINED MACORO_VARIANT_LITE)
    set(MACORO_VARIANT_LITE_V ${MACORO_VARIANT_LITE})
else()

    if(${MACORO_CPP_20})
        set(MACORO_VARIANT_LITE_V OFF)
    else()
        set(MACORO_VARIANT_LITE_V ON)
    endif()
endif()


message("MACORO_OPTIONAL_LITE_V = ${MACORO_OPTIONAL_LITE_V}")