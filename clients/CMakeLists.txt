include(CMakeParseArguments)
function(add_client)
	set(options GUI)
	set(oneVal NAME DISPLAY_NAME NAMESPACE)
	set(multiVal SRCS LIBS)
	cmake_parse_arguments(CLIENT "${options}" "${oneVal}" "${multiVal}" ${ARGN})

	set(APP_NAME ${PROJECT_NAME}_client_${CLIENT_NAME})
	configure_file(${CMAKE_SOURCE_DIR}/clients/main.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/main.cpp)

	add_executable(${APP_NAME} ${CMAKE_CURRENT_BINARY_DIR}/main.cpp ${CLIENT_SRCS})

	if(CLIENT_GUI)
		target_link_libraries(${APP_NAME} PRIVATE ${PROJECT_NAME}_clientlib_gui jd-util-gui Qt5::Widgets ${CLIENT_LIBS})
	else()
		target_link_libraries(${APP_NAME} PRIVATE ${PROJECT_NAME}_clientlib ${CLIENT_LIBS})
	endif()

	if(SPORTSED_SKIP_AUTH)
		target_link_libraries(${APP_NAME} PRIVATE ${PROJECT_NAME}_serverlib)
	endif()

	fix_osx_rpath(${APP_NAME})
	add_coverage_flags(${APP_NAME})
endfunction()

add_subdirectory(clientlib)
add_subdirectory(si_remote)
add_subdirectory(registration)
add_subdirectory(admin)
add_subdirectory(checkout)
add_subdirectory(speaker)
add_subdirectory(live)
add_subdirectory(printer)
add_subdirectory(apiserver)

add_custom_target(_client_main_template SOURCES main.cpp.in)
