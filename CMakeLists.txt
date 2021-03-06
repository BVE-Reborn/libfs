cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(bvestl-fs LANGUAGES CXX)

include(GenerateExportHeader)
include(GNUInstallDirs)

find_library(EASTL_NAME EASTL)
add_library(eastl::lib INTERFACE IMPORTED)
set_property(TARGET eastl::lib
			 PROPERTY INTERFACE_LINK_LIBRARIES ${EASTL_NAME}
)

if(NOT BVESTL_FS_USER)
	find_package(doctest CONFIG REQUIRED)
	find_package(fmt CONFIG REQUIRED)
endif()
find_package(bvestl CONFIG REQUIRED)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE HEADERS LIST_DIRECTORIES false CONFIGURE_DEPENDS "include/*.hpp")
file(GLOB_RECURSE SOURCES LIST_DIRECTORIES false CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false CONFIGURE_DEPENDS "tests/*.cpp")

#########
# libfs #
#########

add_library(bvestl-fs ${SOURCES} ${HEADERS})
target_include_directories(bvestl-fs PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)
if(WIN32)
	target_compile_options(bvestl-fs PRIVATE /experimental:external /external:anglebrackets /external:W0)
else()
	target_compile_options(bvestl-fs PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_link_libraries(bvestl-fs PUBLIC bvestl::bvestl eastl::lib)

generate_export_header(
		bvestl-fs
		BASE_NAME BVESTL_FS
		EXPORT_FILE_NAME include/bvestl/fs/api.hpp
)
target_include_directories(bvestl-fs PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>)

install(
		TARGETS bvestl-fs
		EXPORT bvestl-fs-config
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
		EXPORT bvestl-fs-config DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/bvestl-fs
		NAMESPACE bvestl::
)

install(
		DIRECTORY ${CMAKE_SOURCE_DIR}/include/ ${CMAKE_BINARY_DIR}/include/
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/
		FILES_MATCHING PATTERN "*.hpp*"
)

#########
# tests #
#########
if(NOT BVESTL_FS_USER)
	add_executable(path_demo ${TEST_SOURCES})
	target_link_libraries(path_demo PRIVATE bvestl-fs doctest::doctest fmt::fmt bvestl::bvestl eastl::lib)
endif()
