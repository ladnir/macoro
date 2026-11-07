
cmake_minimum_required(VERSION 3.15)
project(macoro VERSION 0.1.3)
include(cmake/macoroBuildOptions.cmake)
include(cmake/macoroFindDeps.cmake)

add_subdirectory(macoro)

if(MACORO_TESTS)
    enable_testing()
	add_subdirectory(tests)
	add_subdirectory(frontend)
    add_test(NAME macoro_bulk COMMAND macoroFrontend -u
        bulk thread_pool schedule_after when_all_basic when_all_scope)
    set_tests_properties(macoro_bulk PROPERTIES TIMEOUT 30)
    if(MACORO_CPP_20)
        add_test(NAME macoro_all_of COMMAND macoroFrontend -u all_of)
        set_tests_properties(macoro_all_of PROPERTIES TIMEOUT 30
            PASS_REGULAR_EXPRESSION "All Passed \\(7\\)")
    endif()
endif()
