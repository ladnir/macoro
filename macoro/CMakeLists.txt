
file(GLOB_RECURSE SRCS *.cpp)

message("srcs ${SRCS}")

add_executable( macoro ${SRCS})

target_include_directories( macoro PUBLIC 
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
                    $<INSTALL_INTERFACE:include>) 
target_include_directories( macoro PUBLIC 
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
                    $<INSTALL_INTERFACE:include>) 
                    
#target_link_libraries( macoro Threads::Threads)

if(APPLE)
    target_compile_options( macoro PUBLIC
        "-std=c++20"
    )
else()
    set_target_properties( macoro
        PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
    )
endif()
#target_link_libraries( macoro nonstd::span-lite)
#target_link_libraries( macoro nonstd::optional-lite)
#target_link_libraries( macoro nonstd::variant-lite)
## Get the include directories for the target.
#    
#
#
#target_link_libraries( macoro function2::function2)

