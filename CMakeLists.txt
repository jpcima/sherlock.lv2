cmake_minimum_required(VERSION 2.8)

project(sherlock.lv2)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/libosc)

set(DEST lib/lv2/sherlock.lv2)
set(LIB_EXT ".so")

find_package(PkgConfig) # ${PKG_CONFIG_FOUND}

pkg_search_module(LV2 REQUIRED lv2>=1.10)
include_directories(${LV2_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LV2_LDFLAGS})

pkg_search_module(ELM REQUIRED elementary>=1.8)
include_directories(${ELM_INCLUDE_DIRS})
set(LIBS_UI ${LIBS_UI} ${ELM_LDFLAGS})

add_library(sherlock MODULE
	sherlock.c
	atom.c)
target_link_libraries(sherlock ${LIBS})
set_target_properties(sherlock PROPERTIES PREFIX "")
install(TARGETS sherlock DESTINATION ${DEST})

add_library(sherlock_ui MODULE
	sherlock_ui.c
	atom_ui.c)
target_link_libraries(sherlock_ui ${LIBS_UI})
set_target_properties(sherlock_ui PROPERTIES PREFIX "")
install(TARGETS sherlock_ui DESTINATION ${DEST})

configure_file(${PROJECT_SOURCE_DIR}/manifest.ttl.in ${PROJECT_BINARY_DIR}/manifest.ttl)
install(FILES ${PROJECT_BINARY_DIR}/manifest.ttl DESTINATION ${DEST})
install(FILES ${PROJECT_SOURCE_DIR}/sherlock.ttl DESTINATION ${DEST})
