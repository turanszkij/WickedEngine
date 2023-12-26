if (NOT WIN32)
	find_package(Threads REQUIRED)
endif ()

set (SOURCE_FILES
	stdafx.cpp
	Example_ImGui.cpp
	Example_ImGui.h
	stdafx.h
	ImGui/imconfig.h
	ImGui/imgui.cpp
	ImGui/imgui.h
	ImGui/imgui_demo.cpp
	ImGui/imgui_draw.cpp
	ImGui/imgui_internal.h
	ImGui/imgui_tables.cpp
	ImGui/imgui_widgets.cpp
	ImGui/imstb_rectpack.h
	ImGui/imstb_textedit.h
	ImGui/imstb_truetype.h
	ImGui/imgui_impl_sdl.cpp
	ImGui/imgui_impl_sdl.h
)

if (WIN32)
	list (APPEND SOURCE_FILES
		main_Windows.cpp
		main_Windows.h
		Tests.rc
	)

	add_executable(Example_ImGui WIN32 ${SOURCE_FILES})

	target_link_libraries(Example_ImGui PUBLIC
		WickedEngine_Windows
	)
	set(LIB_DXCOMPILER "dxcompiler.dll")
else()
	list (APPEND SOURCE_FILES
		main_SDL2.cpp
	)

	add_executable(Example_ImGui ${SOURCE_FILES})

	target_link_libraries(Example_ImGui PUBLIC
		WickedEngine
		Threads::Threads
	)
	
	# Copy shaders to build and source folders just to be safe:
	add_custom_command(
		TARGET Example_ImGui POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiPS.hlsl ${CMAKE_CURRENT_BINARY_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiVS.hlsl ${CMAKE_CURRENT_BINARY_DIR}
	)

	set(LIB_DXCOMPILER "libdxcompiler.so")
endif ()

if (MSVC)
	set_property(TARGET Example_ImGui PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()


# Copy content to build folder:
add_custom_command(
	TARGET Example_ImGui POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WICKED_ROOT_DIR}/WickedEngine/${LIB_DXCOMPILER} ${CMAKE_CURRENT_BINARY_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${WICKED_ROOT_DIR}/Content ${CMAKE_CURRENT_BINARY_DIR}/../Content
)
