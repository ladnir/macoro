
set(SRC
	task_tests.cpp
)


add_library(macoroTests ${SRC})

target_link_libraries(macoroTests macoro)