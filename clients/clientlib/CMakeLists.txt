set(SRC
	ServerConnection.h
	ServerConnection.cpp
)

add_library(${PROJECT_NAME}_clientlib ${SRC})
target_link_libraries(${PROJECT_NAME}_clientlib PUBLIC ${PROJECT_NAME}_commonlib Qt5::Network)
target_include_directories(${PROJECT_NAME}_clientlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(GUI_SRC
	ClientMainWindow.h
	ClientMainWindow.cpp

	ConnectionDialog.h
	ConnectionDialog.cpp
	ConnectionDialog.ui
)
add_library(${PROJECT_NAME}_clientlib_gui ${GUI_SRC})
target_link_libraries(${PROJECT_NAME}_clientlib_gui PUBLIC ${PROJECT_NAME}_clientlib Qt5::Widgets)
