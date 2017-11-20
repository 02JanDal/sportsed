find_package(Qt5 REQUIRED COMPONENTS Test)

add_unit_test(client_server_tests LIBRARIES
	${PROJECT_NAME}_serverlib
	${PROJECT_NAME}_clientlib
	${PROJECT_NAME}_server_tst_Util
	Qt5::Test
)

fix_osx_rpath(tst_client_server_tests)
sign_osx_app(tst_client_server_tests)
