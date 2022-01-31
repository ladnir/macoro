

set(SRC
	main.cpp
)


add_executable(macoroFrontend ${SRC})

target_link_libraries(macoroFrontend macoroTests macoro)