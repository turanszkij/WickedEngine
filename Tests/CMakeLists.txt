if (NOT WIN32)
	find_package(Threads REQUIRED)
endif ()

set (SOURCE_FILES
	stdafx.cpp
	Tests.cpp
	Tests.h
	stdafx.h
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
else()
	list (APPEND SOURCE_FILES
		main_SDL2.cpp
	)

	add_executable(Tests ${SOURCE_FILES})

	target_link_libraries(Tests PUBLIC 
		WickedEngine
		Threads::Threads
	)
	
endif ()

if (MSVC)
	set_property(TARGET Tests PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()



add_custom_command(
    TARGET Tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/../WickedEngine/shaders/spirv
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/spirv)
add_custom_command(
    TARGET Tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/images
            ${CMAKE_CURRENT_BINARY_DIR}/images)
add_custom_command(
    TARGET Tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/sound
            ${CMAKE_CURRENT_BINARY_DIR}/sound)
add_custom_command(
    TARGET Tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/../Content
            ${CMAKE_CURRENT_BINARY_DIR}/../Content)
