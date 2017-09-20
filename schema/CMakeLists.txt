set(SCHEMAS
	orm.capnp
)

if(NOT CAPNP_COMMAND)
	set(CAPNP_COMMAND ${CONAN_CAPNPROTO_ROOT}/bin/capnp CACHE FILEPATH "" FORCE)
endif()

set(output_dir ${CMAKE_CURRENT_BINARY_DIR}/schemas)
file(MAKE_DIRECTORY ${output_dir})

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/append_system_header.cmake "
file(READ \${FILE} data)
file(WRITE \${FILE} \"#pragma clang system_header
\${data}\")
")

set(SCHEMAS_OUTPUT )
foreach(schema ${SCHEMAS})
	set(output_h ${output_dir}/${schema}.h)
	set(output_cpp ${output_dir}/${schema}.c++)
	set(output ${output_h} ${output_cpp})
	add_custom_command(OUTPUT ${output}
		COMMAND ${CAPNP_COMMAND} compile -o${CAPNP_COMMAND}c-c++ --src-prefix=${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${schema}
		COMMAND ${CMAKE_COMMAND} -DFILE=${output_h} -P ${CMAKE_CURRENT_BINARY_DIR}/append_system_header.cmake
		MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${schema}
		WORKING_DIRECTORY ${output_dir}
		)
	list(APPEND SCHEMAS_OUTPUT ${output})
endforeach()

add_library(${PROJECT_NAME}_schema STATIC ${SCHEMAS_OUTPUT})
target_link_libraries(${PROJECT_NAME}_schema PUBLIC CONAN_PKG::capnproto)
target_include_directories(${PROJECT_NAME}_schema PUBLIC ${output_dir})
set_source_files_properties(${SCHEMAS_OUTPUT} PROPERTIES COMPILE_FLAGS "-Wno-non-virtual-dtor -Wno-weak-vtables")
