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

	CapnprotoUtil.h
	CapnprotoUtil.cpp
)
add_library(${PROJECT_NAME}_commonlib STATIC ${SRC})
target_link_libraries(${PROJECT_NAME}_commonlib PUBLIC ${PROJECT_NAME}_schema jd-util)
target_include_directories(${PROJECT_NAME}_commonlib PUBLIC ${CMAKE_BINARY_DIR} /usr/local/Cellar/msgpack/2.1.5/include) # for config.h
