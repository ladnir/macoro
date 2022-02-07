
set(SRC
	task_tests.cpp
	await_lifetime_tests.cpp
)


add_library(macoroTests ${SRC})

target_link_libraries(macoroTests macoro)
#target_compile_options(macoroTests PRIVATE "-save-temps")
#set_source_files_properties(await_lifetime_tests.cpp PROPERTIES COMPILE_FLAGS "-E ")
#set_source_files_properties(await_lifetime_tests.cpp PROPERTIES COMPILE_FLAGS "/P /Fi${CMAKE_CURRENT_BINARY_DIR}\\output.i")
