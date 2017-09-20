find_package(Qt5 REQUIRED COMPONENTS Sql)

set(SRC
	DatabaseServer.h
	DatabaseServer.cpp
	DatabaseMigration.h
	DatabaseMigration.cpp
	DatabaseEngine.h
	DatabaseEngine.cpp
)
add_library(${PROJECT_NAME}_serverlib STATIC ${SRC})
target_link_libraries(${PROJECT_NAME}_serverlib PUBLIC ${PROJECT_NAME}_commonlib Qt5::Sql jd-util-sql)

add_executable(${PROJECT_NAME}_server main.cpp)
target_link_libraries(${PROJECT_NAME}_server PRIVATE ${PROJECT_NAME}_serverlib)

add_unit_test(sportsed_server LIBRARIES ${PROJECT_NAME}_serverlib)
add_unit_test(DatabaseMigration LIBRARIES ${PROJECT_NAME}_serverlib)
