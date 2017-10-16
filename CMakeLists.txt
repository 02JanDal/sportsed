cmake_minimum_required(VERSION 3.7)

project(sportsed VERSION 0.0.1)
enable_testing()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(sportsed_debug_default OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
	set(sportsed_debug_default ON)
endif()
option(SPORTSED_DEBUG "Build sportsed with extra debugging checks" ${sportsed_debug_default})
if(SPORTSED_DEBUG)
	add_definitions(-DDEBUG_BUILD)
endif()

add_subdirectory(../jd-util jd-util)
include(CoverageHelpers)

find_package(Qt5 REQUIRED COMPONENTS Core Network Widgets)
find_package(Catch REQUIRED)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/memsharded/cmake-conan")
   file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
				 "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

find_program(CONAN conan PATHS /usr/bin /usr/local/bin)

conan_cmake_run(REQUIRES #capnproto/0.6.1@jandal/testing #msgpack/0.1.0@jason2506/testing
				OPTIONS capnproto:shared=True
				BASIC_SETUP CMAKE_TARGETS
				BUILD missing
				CONAN_COMMAND ${CONAN})
list(APPEND CMAKE_MODULE_PATH "${CONAN_CAPNPROTO_ROOT}/cmake")

configure_file(${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/config.h @ONLY)

include_directories(${CMAKE_SOURCE_DIR})
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Weverything -Werror -Wno-padded -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-switch-enum -Wno-undefined-reinterpret-cast -Wno-unused-template")

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

add_subdirectory(commonlib)
add_subdirectory(server)
add_subdirectory(clients/clientlib)
add_subdirectory(clients/si_remote)
add_subdirectory(clients/registration)
add_subdirectory(clients/admin)
add_subdirectory(clients/checkout)
add_subdirectory(clients/speaker)
add_subdirectory(clients/live)
add_subdirectory(clients/printer)
add_subdirectory(clients/apiserver)

add_coverage_flags(sportsed_serverlib sportsed_commonlib sportsed_clientlib
	sportsed_server
	#sportsed_client_admin
	#sportsed_client_apiserver
	#sportsed_client_checkout
	#sportsed_client_live
	sportsed_client_printer
	#sportsed_client_registration
	#sportsed_client_si_remote
	#sportsed_client_speaker
)
add_coverage_capture(sportsed sportsed_server tst_DatabaseMigration tst_DatabaseEngine tst_DatabaseServer tst_sportsed_server)
add_custom_target(coverage DEPENDS coverage_sportsed_html)
add_custom_target(coverage_open DEPENDS coverage_sportsed_open)
