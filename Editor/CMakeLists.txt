cmake_minimum_required(VERSION 3.19)

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS *.cpp)
list(FILTER SOURCE_FILES EXCLUDE REGEX ${CMAKE_CURRENT_SOURCE_DIR}/main_.*)
list(APPEND SOURCE_FILES main_${PLATFORM}.cpp)


if (WIN32)
	list (APPEND SOURCE_FILES
		Editor.rc
	)

	add_executable(WickedEngineEditor WIN32 ${SOURCE_FILES})

	target_link_libraries(WickedEngineEditor PUBLIC
		WickedEngine_Windows
	)

	set_property(TARGET WickedEngineEditor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
	set(LIB_DXCOMPILER "dxcompiler.dll")
else ()
	add_executable(WickedEngineEditor ${SOURCE_FILES})

	target_link_libraries(WickedEngineEditor PUBLIC
		WickedEngine
	)
	set(LIB_DXCOMPILER "libdxcompiler.so")

	# needed for proper names in crash stacktrace
	set_target_properties(WickedEngineEditor
		PROPERTIES
		ENABLE_EXPORTS ON
	)

endif ()

# Needed for terrain system
add_dependencies(WickedEngineEditor Content)

# Copy content to build folder:
add_custom_command(
	TARGET WickedEngineEditor POST_BUILD
	# Copy shader compiler library in the source folder
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WICKED_ROOT_DIR}/WickedEngine/${LIB_DXCOMPILER} ${CMAKE_CURRENT_BINARY_DIR}
	#COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/languages ${CMAKE_CURRENT_BINARY_DIR}/languages
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fonts ${CMAKE_CURRENT_BINARY_DIR}/fonts
)

include(GNUInstallDirs)
set(EDITOR_INSTALL_FOLDER "${CMAKE_INSTALL_LIBDIR}/WickedEngine/Editor")

# WickedEngineEditor executable
install(TARGETS WickedEngineEditor RUNTIME DESTINATION ${EDITOR_INSTALL_FOLDER})

# install editor assets
install(DIRECTORY
		#"${CMAKE_CURRENT_SOURCE_DIR}/languages"
		"${CMAKE_CURRENT_SOURCE_DIR}/fonts"
		DESTINATION ${EDITOR_INSTALL_FOLDER})
# optional .ini .ico .lua
install(FILES
		${CMAKE_CURRENT_SOURCE_DIR}/config.ini
		${CMAKE_CURRENT_SOURCE_DIR}/wicked.ico
		${CMAKE_CURRENT_SOURCE_DIR}/startup.lua
		DESTINATION ${EDITOR_INSTALL_FOLDER})

