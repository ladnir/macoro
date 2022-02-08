
set(SRC )

add_library(macoro INTERFACE ${SRC})



target_include_directories( macoro INTERFACE 
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
                    $<INSTALL_INTERFACE:include>) 
target_include_directories( macoro INTERFACE 
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
                    $<INSTALL_INTERFACE:>) 
                
configure_file(config.h.in config.h)
                    

if(MACORO_OPTIONAL_LITE_V)
    target_link_Libraries(macoro INTERFACE nonstd::optional-lite)
endif()

if(MACORO_VARIANT_LITE_V)
    target_link_Libraries(macoro INTERFACE nonstd::variant-lite)
endif()

if(MACORO_CPP_20)
    set(MACORO_STD_VAL 20)
else()
    set(MACORO_STD_VAL 14)
endif()

message("MACORO_STD_VAL ${MACORO_STD_VAL}")

if(APPLE OR UNIX)
    target_compile_options( macoro INTERFACE
        "-std=c++${MACORO_STD_VAL}"
    )
elseif(MSVC)
    target_compile_options( macoro INTERFACE
        "/std:c++${MACORO_STD_VAL}"
    )
else()
    set_target_properties( macoro 
        PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
    )
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND MACORO_CPP_20)
    target_compile_options( macoro INTERFACE
        "-fcoroutines"
        )
endif()
