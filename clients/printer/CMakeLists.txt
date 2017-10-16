set(APP_NAME ${PROJECT_NAME}_client_printer)
set(APP_DISPLAY_NAME "Sportsed Printer Client")
set(APP_NAMESPACE Printer)
configure_file(${CMAKE_SOURCE_DIR}/clients/main.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/main.cpp @ONLY)

find_package(Qt5 REQUIRED COMPONENTS PrintSupport)

set(SRC
	${CMAKE_CURRENT_BINARY_DIR}/main.cpp

	MainWindow.h
	MainWindow.cpp
	MainWindow.ui

	Report.h
	Report.cpp

	Automation.h
	Automation.cpp

	AutomationsModel.h
	AutomationsModel.cpp

	Generator.h
	Generator.cpp
)

add_executable(${PROJECT_NAME}_client_printer ${SRC})
target_link_libraries(${PROJECT_NAME}_client_printer PRIVATE ${PROJECT_NAME}_clientlib_gui jd-util-gui Qt5::Widgets Qt5::PrintSupport)

fix_osx_rpath(${PROJECT_NAME}_client_printer)
