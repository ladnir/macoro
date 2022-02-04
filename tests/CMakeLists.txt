
set(SRC
	task_tests.cpp
	yield_await_tests.cpp
)


add_library(macoroTests ${SRC})

target_link_libraries(macoroTests macoro)