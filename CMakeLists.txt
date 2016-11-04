cmake_minimum_required(VERSION 2.8)

project(sherlock.lv2)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/libosc)
include_directories(${PROJECT_SOURCE_DIR}/osc.lv2)
include_directories(${PROJECT_SOURCE_DIR}/sandbox_ui.lv2)
include_directories(${PROJECT_SOURCE_DIR}/props.lv2)
include_directories(${PROJECT_SOURCE_DIR}/pugl)
include_directories(${PROJECT_SOURCE_DIR}/symap)

set(CMAKE_C_FLAGS "-fdata-sections -ffunction-sections ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-std=gnu11 -Wextra -Wno-unused-parameter -ffast-math -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-Wshadow -Wimplicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes ${CMAKE_C_FLAGS}")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-z,defs ${CMAKE_MODULE_LINKER_FLAGS}")
	set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-z,nodelete ${CMAKE_MODULE_LINKER_FLAGS}")
elseif(WIN32)
	set(CMAKE_C_FLAGS "-mstackrealign ${CMAKE_C_FLAGS}")
endif()

if(APPLE)
	set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-dead_strip ${CMAKE_MODULE_LINKER_FLAGS}")
else()
	set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--gc-sections -Wl,-s ${CMAKE_MODULE_LINKER_FLAGS}")
endif()

add_definitions("-D_GNU_SOURCE=1") # asprintf

file(STRINGS "VERSION" SHERLOCK_VERSION)
string(REPLACE "." ";" VERSION_LIST ${SHERLOCK_VERSION})
list(GET VERSION_LIST 0 SHERLOCK_MAJOR_VERSION)
list(GET VERSION_LIST 1 SHERLOCK_MINOR_VERSION)
list(GET VERSION_LIST 2 SHERLOCK_MICRO_VERSION)
add_definitions("-DSHERLOCK_VERSION=\"${SHERLOCK_VERSION}\"")

set(DEST lib/lv2/sherlock.lv2)

find_package(PkgConfig) # ${PKG_CONFIG_FOUND}

pkg_search_module(LV2 REQUIRED lv2>=1.10)
include_directories(${LV2_INCLUDE_DIRS})

pkg_search_module(ELM REQUIRED elementary>=1.8)
include_directories(${ELM_INCLUDE_DIRS})

pkg_search_module(NANOMSG REQUIRED libnanomsg nanomsg)
include_directories(${NANOMSG_INCLUDE_DIRS})

pkg_search_module(SRATOM REQUIRED sratom-0>=0.4.0)
include_directories(${SRATOM_INCLUDE_DIRS})

pkg_search_module(LILV REQUIRED lilv-0>=0.20.0)
include_directories(${LILV_INCLUDE_DIRS})
if((${LILV_VERSION} VERSION_EQUAL "0.22.0") OR (${LILV_VERSION} VERSION_GREATER "0.22.0"))
	add_definitions("-DLILV_0_22")
endif()

add_library(sherlock MODULE
	sherlock.c
	atom_inspector.c
	midi_inspector.c
	osc_inspector.c)
set_target_properties(sherlock PROPERTIES PREFIX "")
install(TARGETS sherlock DESTINATION ${DEST})

add_library(sherlock_ui MODULE
	${PROJECT_SOURCE_DIR}/sandbox_ui.lv2/sandbox_ui.c
	${PROJECT_SOURCE_DIR}/sandbox_ui.lv2/sandbox_master.c
	sherlock_ui.c)
target_link_libraries(sherlock_ui
	${NANOMSG_LDFLAGS}
	${SRATOM_LDFLAGS})
set_target_properties(sherlock_ui PROPERTIES PREFIX "")
install(TARGETS sherlock_ui DESTINATION ${DEST})

find_package(FLEX)
flex_target(encoder encoder.l ${PROJECT_BINARY_DIR}/encoder.c
	COMPILE_FLAGS	"--header-file=${PROJECT_BINARY_DIR}/encoder.h --prefix=enc")

add_library(sherlock_eo MODULE
	sherlock_eo.c
	atom_inspector_eo.c
	midi_inspector_eo.c
	osc_inspector_eo.c
	${FLEX_encoder_OUTPUTS})
target_link_libraries(sherlock_eo
	${ELM_LDFLAGS}
	${SRATOM_LDFLAGS})
set_target_properties(sherlock_eo PROPERTIES PREFIX "")
install(TARGETS sherlock_eo DESTINATION ${DEST})

add_definitions("-DPUGL_HAVE_GL")
add_library(sherlock_nk MODULE
	sherlock_nk.c
	midi_inspector_nk.c
	atom_inspector_nk.c
	osc_inspector_nk.c
	pugl/pugl/pugl_x11.c)
target_link_libraries(sherlock_nk
	${NANOMSG_LDFLAGS}
	${SRATOM_LDFLAGS}
	X11 GL m)
set_target_properties(sherlock_nk PROPERTIES PREFIX "")
install(TARGETS sherlock_nk DESTINATION ${DEST})

add_executable(sandbox_efl
	${PROJECT_SOURCE_DIR}/sandbox_ui.lv2/sandbox_slave.c
	${PROJECT_SOURCE_DIR}/sandbox_ui.lv2/sandbox_efl.c
	${PROJECT_SOURCE_DIR}/symap/symap.c)
target_link_libraries(sandbox_efl
	${ELM_LDFLAGS}
	${NANOMSG_LDFLAGS}
	${LILV_LDFLAGS})
install(TARGETS sandbox_efl DESTINATION ${DEST})

configure_file(${PROJECT_SOURCE_DIR}/manifest.ttl.in ${PROJECT_BINARY_DIR}/manifest.ttl)
install(FILES ${PROJECT_BINARY_DIR}/manifest.ttl DESTINATION ${DEST})
install(FILES ${PROJECT_SOURCE_DIR}/sherlock.ttl DESTINATION ${DEST})
install(FILES ${PROJECT_SOURCE_DIR}/sherlock_ui.ttl DESTINATION ${DEST})
install(FILES ${PROJECT_SOURCE_DIR}/omk_logo_256x256.png DESTINATION ${DEST})
install(FILES ${PROJECT_SOURCE_DIR}/nuklear/extra_font/Cousine-Regular.ttf DESTINATION ${DEST})
