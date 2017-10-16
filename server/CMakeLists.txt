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
target_link_libraries(${PROJECT_NAME}_serverlib PUBLIC ${PROJECT_NAME}_commonlib Qt5::Sql Qt5::Network jd-util-sql)

add_executable(${PROJECT_NAME}_server main.cpp)
target_link_libraries(${PROJECT_NAME}_server PRIVATE ${PROJECT_NAME}_serverlib)

add_library(${PROJECT_NAME}_server_tst_Util STATIC tst_Util.h)
target_link_libraries(${PROJECT_NAME}_server_tst_Util ${PROJECT_NAME}_serverlib)

add_unit_test(sportsed_server LIBRARIES ${PROJECT_NAME}_serverlib)
add_unit_test(DatabaseMigration LIBRARIES ${PROJECT_NAME}_serverlib ${PROJECT_NAME}_server_tst_Util)
add_unit_test(DatabaseEngine LIBRARIES ${PROJECT_NAME}_serverlib ${PROJECT_NAME}_server_tst_Util)
add_unit_test(DatabaseServer LIBRARIES ${PROJECT_NAME}_serverlib ${PROJECT_NAME}_server_tst_Util)

fix_osx_rpath(tst_sportsed_server tst_DatabaseMigration tst_DatabaseEngine tst_DatabaseServer)
