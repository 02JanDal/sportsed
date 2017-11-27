cmake_minimum_required(VERSION 3.5)

project(sportsed VERSION 0.0.1)
enable_testing()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(sportsed_debug_default OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(sportsed_debug_default ON)
endif()
option(SPORTSED_DEBUG "Build sportsed with extra debugging checks" ${sportsed_debug_default})
if(SPORTSED_DEBUG)
	add_definitions(-DDEBUG_BUILD)
endif()

option(SPORTSED_SKIP_AUTH "Skip past authentication when debuggning" ${sportsed_debug_default})
if(SPORTSED_SKIP_AUTH)
	add_definitions(-DSPORTSED_SKIP_AUTH)
endif()

if(NOT JD_UTIL_PATH)
	set(JD_UTIL_PATH ../jd-util)
endif()
add_subdirectory(${JD_UTIL_PATH} jd-util)
include(CoverageHelpers)

find_package(Qt5 REQUIRED COMPONENTS Core Network Widgets Sql)

find_program(CONAN conan PATHS /usr/bin /usr/local/bin)
if(${CMAKE_SOURCE_DIR}/conanfile.txt IS_NEWER_THAN ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	execute_process(COMMAND ${CONAN} install .. WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)
add_custom_target(__dummy DEPENDS ${CMAKE_SOURCE_DIR}/conanfile.txt)
set(JD_UTIL_CATCH_LIB CONAN_PKG::catch)

configure_file(${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/config.h @ONLY)

include_directories(${CMAKE_SOURCE_DIR})
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Weverything -Werror -Wno-padded -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-switch-enum -Wno-undefined-reinterpret-cast -Wno-exit-time-destructors -Wno-gnu-zero-variadic-macro-arguments -Wno-weak-vtables -Wno-global-constructors")
append_if_valid_cxx_flag(nounusedtemplate "-Wno-unused-template" CMAKE_CXX_FLAGS_DEBUG)

function(fix_osx_rpath)
	if(APPLE)
		foreach(target ${ARGN})
			add_custom_command(TARGET ${target}
				POST_BUILD
				COMMAND install_name_tool -add_rpath ${Qt5_DIR}/../../ $<TARGET_FILE:${target}>
			)
		endforeach()
	endif()
endfunction()

function(sign_osx_app target)
	set(SPORTSED_OSX_CERTIFICATE "" CACHE STRING "Name of the certificate to use for signing on OS X")
	if(APPLE AND NOT SPORTSED_OSX_CERTIFICATE STREQUAL "")
		add_custom_command(TARGET ${target}
			POST_BUILD
			COMMAND codesign -f --deep -s ${SPORTSED_OSX_CERTIFICATE} $<TARGET_FILE:${target}>
		)
	endif()
endfunction()

add_subdirectory(commonlib)
add_subdirectory(server)
add_subdirectory(clients)
add_subdirectory(tests)

add_coverage_flags(sportsed_serverlib sportsed_commonlib sportsed_clientlib
	sportsed_server
)
add_coverage_capture(sportsed sportsed_server tst_DatabaseMigration tst_DatabaseEngine tst_DatabaseServer tst_sportsed_server)
add_custom_target(coverage DEPENDS coverage_sportsed_html)
add_custom_target(coverage_open DEPENDS coverage_sportsed_open)
