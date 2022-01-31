
if(NOT DEFINED COPROTO_BUILD_TYPE)
	if(DEFINED CMAKE_BUILD_TYPE)
		set(COPROTO_BUILD_TYPE ${CMAKE_BUILD_TYPE})
	else()
		set(COPROTO_BUILD_TYPE "Release")
	endif()
endif()


if(NOT COPROTO_BUILD_DIR)
    if(MSVC)
        set(coproto_CONFIG_NAME "${COPROTO_BUILD_TYPE}")
        if("${coproto_CONFIG_NAME}" STREQUAL "RelWithDebInfo" )
            set(coproto_CONFIG_NAME "Release")
	    endif()


        set(COPROTO_BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/../out/build/x64-${coproto_CONFIG_NAME}")
    else()
        set(COPROTO_BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/../out/build/linux")

        if(NOT EXISTS "${COPROTO_BUILD_DIR}")
            set(COPROTO_BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/../")
        endif()
    endif()
else()
    message(STATUS "COPROTO_BUILD_DIR preset to ${COPROTO_BUILD_DIR}")
endif()

if(NOT EXISTS "${COPROTO_BUILD_DIR}")
    message(FATAL_ERROR "failed to find the coproto build directory. Looked at: ${COPROTO_BUILD_DIR}")
endif()