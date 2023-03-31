


add_library(macoroTests 
	"tests.cpp"
	"CLP.cpp"
	"task_tests.cpp"
	"eager_task_tests.cpp"
	"await_lifetime_tests.cpp"
	"result_tests.cpp"
	"when_all_tests.cpp"
	"take_until_tests.cpp"
	"sequence_tests.cpp"
	"CLP.cpp" 
	"CLP.h" 
	"channel_spsc_tests.cpp" 
	"channel_mpsc_tests.cpp")

target_link_libraries(macoroTests macoro)


if(MSVC)

    target_compile_options( macoroTests PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/std:c++${MACORO_CPP_VER}>
    )
else()



    target_compile_options( macoroTests PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++${MACORO_CPP_VER}>
    )
endif()