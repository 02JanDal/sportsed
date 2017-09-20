cmake_minimum_required(VERSION 3.7)

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

add_subdirectory(../jd-util jd-util)

find_package(Qt5 REQUIRED COMPONENTS Core)
find_package(Catch REQUIRED)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/memsharded/cmake-conan")
   file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
				 "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_run(REQUIRES capnproto/0.6.1@jandal/testing #msgpack/0.1.0@jason2506/testing
				OPTIONS capnproto:shared=True
				BASIC_SETUP CMAKE_TARGETS
				BUILD missing
				CONAN_COMMAND /usr/local/bin/conan)
list(APPEND CMAKE_MODULE_PATH "${CONAN_CAPNPROTO_ROOT}/cmake")

set(sportsed_debug_default OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
	set(sportsed_debug_default ON)
endif()
option(SPORTSED_DEBUG "Build sportsed with extra debugging checks" ${sportsed_debug_default})

configure_file(${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/config.h @ONLY)

include_directories(${CMAKE_SOURCE_DIR})
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Weverything -Werror -Wno-padded -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-switch-enum")

add_subdirectory(schema)
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
