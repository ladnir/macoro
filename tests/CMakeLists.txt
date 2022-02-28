


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
	"channel_spsc_tests.cpp")

target_link_libraries(macoroTests macoro)
