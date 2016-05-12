cmake_minimum_required(VERSION 2.8)

project(osc.lv2)

include_directories(${PROJECT_SOURCE_DIR})

set(CMAKE_C_FLAGS "-std=gnu11 -Wextra -Wno-unused-parameter -ffast-math -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-Wshadow -Wimplicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes ${CMAKE_C_FLAGS}")

set(OSC_MAJOR_VERSION 0)
set(OSC_MINOR_VERSION 1)
set(OSC_MICRO_VERSION 1)
set(OSC_VERSION "${OSC_MAJOR_VERSION}.${OSC_MINOR_VERSION}.${OSC_MICRO_VERSION}")

set(DEST lib/lv2/osc.lv2)

add_library(osc MODULE
	test/osc.c)
set_target_properties(osc PROPERTIES PREFIX "")
install(TARGETS osc DESTINATION ${DEST})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test/manifest.ttl.in ${PROJECT_BINARY_DIR}/manifest.ttl)
install(FILES ${PROJECT_BINARY_DIR}/manifest.ttl DESTINATION ${DEST})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/test/osc.ttl DESTINATION ${DEST})

include(CTest)

if(${BUILD_TESTING})
	add_executable(osc_test
		osc_test.c)
	add_test(NAME API-Test COMMAND osc_test)
endif()
