cmake_minimum_required(VERSION 3.19)
set (SOURCE_FILES
	Tests.cpp
	Tests.h
)

if (WIN32)
	list (APPEND SOURCE_FILES
		main_Windows.cpp
		main_Windows.h
		Tests.rc
	)

	add_executable(Tests WIN32 ${SOURCE_FILES})

	target_link_libraries(Tests PUBLIC
		WickedEngine_Windows
	)

	set(LIB_DXCOMPILER "dxcompiler.dll")
else()
	list (APPEND SOURCE_FILES
		main_SDL2.cpp
	)

	add_executable(Tests ${SOURCE_FILES})

	target_link_libraries(Tests PUBLIC
		WickedEngine
	)

	set(LIB_DXCOMPILER "libdxcompiler.so")
endif ()

if(WICKED_USE_IPO)
set_target_properties(Tests PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION ON
    INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF
)
endif()

target_precompile_headers(Tests PRIVATE stdafx.h)


if (MSVC)
	set_property(TARGET Tests PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()

# Copy content to build folder:
add_dependencies(Tests Content)

add_custom_command(
	TARGET Tests POST_BUILD
	# Copy the shader library next to the executable
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WICKED_ROOT_DIR}/WickedEngine/${LIB_DXCOMPILER} ${CMAKE_CURRENT_BINARY_DIR}
	# Copy the resources
	COMMAND ${CMAKE_COMMAND} -E ${COPY_OR_SYMLINK_DIR_CMD} ${CMAKE_CURRENT_SOURCE_DIR}/images ${CMAKE_CURRENT_BINARY_DIR}/images
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/test_script.lua ${CMAKE_CURRENT_BINARY_DIR}/test_script.lua
	# Copy font test resources
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/yumin.ttf ${CMAKE_CURRENT_BINARY_DIR}/

	COMMENT "$<IF:$<BOOL:${WICKED_USE_SYMLINKS}>,Symlinking,Copying> Tests resources"
	VERBATIM
)
