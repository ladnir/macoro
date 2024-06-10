

set(SRC
	main.cpp
)



add_executable(macoroFrontend ${SRC})
target_link_libraries(macoroFrontend macoroTests macoro)


if(MSVC)

    target_compile_options( macoroFrontend PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/std:c++${MACORO_CPP_VER}>
    )
else()



    target_compile_options( macoroFrontend PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++${MACORO_CPP_VER}>
    )
endif()