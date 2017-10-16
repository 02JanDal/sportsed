set(SRC
	${CMAKE_BINARY_DIR}/config.h
	TableQuery.h
	TableQuery.cpp
	ChangeQuery.h
	ChangeQuery.cpp
	ChangeResponse.h
	ChangeResponse.cpp
	Change.h
	Change.cpp

	Record.h
	Record.cpp

	TcpMessageSocket.h
	TcpMessageSocket.cpp
)
add_library(${PROJECT_NAME}_commonlib STATIC ${SRC})
target_link_libraries(${PROJECT_NAME}_commonlib PUBLIC jd-util Qt5::Network)
target_include_directories(${PROJECT_NAME}_commonlib PUBLIC ${CMAKE_BINARY_DIR}) # for config.h
