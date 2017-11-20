find_package(Qt5 REQUIRED COMPONENTS PrintSupport)

add_client(GUI NAME printer DISPLAY_NAME Printer NAMESPACE Printer LIBS Qt5::PrintSupport SRCS
	gui/MainWindow.h
	gui/MainWindow.cpp
	gui/MainWindow.ui

	Report.h
	Report.cpp

	Automation.h
	Automation.cpp

	AutomationsModel.h
	AutomationsModel.cpp

	Generator.h
	Generator.cpp
)
