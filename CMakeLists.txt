cmake_minimum_required(VERSION 2.8)

project(sherlock.lv2)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/libosc)
include_directories(${PROJECT_SOURCE_DIR}/eo_ui.lv2)

set(CMAKE_C_FLAGS "-std=gnu99 ${CMAKE_C_FLAGS}")

set(SHERLOCK_MAJOR_VERSION 0)
set(SHERLOCK_MINOR_VERSION 1)
set(SHERLOCK_MICRO_VERSION 1)
set(SHERLOCK_VERSION "${SHERLOCK_MAJOR_VERSION}.${SHERLOCK_MINOR_VERSION}.${SHERLOCK_MICRO_VERSION}")
add_definitions("-DSHERLOCK_VERSION=\"${SHERLOCK_VERSION}\"")

set(DEST lib/lv2/sherlock.lv2)
if(WIN32)
	set(LIB_EXT ".dll")
elseif(APPLE)
	set(LIB_EXT ".so")
else()
	set(LIB_EXT ".so")
endif()

find_package(PkgConfig) # ${PKG_CONFIG_FOUND}

pkg_search_module(LV2 REQUIRED lv2>=1.10)
include_directories(${LV2_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LV2_LDFLAGS})

pkg_search_module(ELM REQUIRED elementary>=1.8)
include_directories(${ELM_INCLUDE_DIRS})
set(LIBS_UI ${LIBS_UI} ${ELM_LDFLAGS})

pkg_search_module(ECORE_X OPTIONAL ecore-x)

if((DEFINED ECORE_X_FOUND) AND ((${ELM_VERSION} VERSION_EQUAL "1.13.0") OR (${ELM_VERSION} VERSION_GREATER "1.13.0")))
	message(STATUS "X11 UI wrap enabled")
	set(X11_UI_WRAP "")
	add_definitions("-DX11_UI_WRAP")
else()
	message(STATUS "X11 UI wrap disabled")
	set(X11_UI_WRAP "#")
endif()

add_library(sherlock MODULE
	sherlock.c
	atom_inspector.c)
target_link_libraries(sherlock ${LIBS})
set_target_properties(sherlock PROPERTIES PREFIX "")
install(TARGETS sherlock DESTINATION ${DEST})

add_library(sherlock_ui MODULE
	sherlock_ui.c
	atom_inspector_ui.c)
target_link_libraries(sherlock_ui ${LIBS_UI})
set_target_properties(sherlock_ui PROPERTIES PREFIX "")
install(TARGETS sherlock_ui DESTINATION ${DEST})

configure_file(${PROJECT_SOURCE_DIR}/manifest.ttl.in ${PROJECT_BINARY_DIR}/manifest.ttl)
install(FILES ${PROJECT_BINARY_DIR}/manifest.ttl DESTINATION ${DEST})
install(FILES ${PROJECT_SOURCE_DIR}/sherlock.ttl DESTINATION ${DEST})
install(FILES ${PROJECT_SOURCE_DIR}/omk_logo_256x256.png DESTINATION ${DEST})
