cmake_minimum_required(VERSION 3.19)
if (NOT WIN32)
	find_package(Threads REQUIRED)
endif ()

set(SOURCE_FILES
	Example_ImGui_Docking.cpp
	Example_ImGui_Docking.h
	../../Editor/ModelImporter_GLTF.cpp
)
set(IMGUI_FILES
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
	ImGui/ImGuizmo.cpp
	ImGui/ImGuizmo.h
	ImGui/IconsMaterialDesign.h
)

if (WIN32)
	list(APPEND SOURCE_FILES
		main_Windows.cpp
		main_Windows.h
		Tests.rc
	)
	list(APPEND IMGUI_FILES
		ImGui/imgui_impl_win32.cpp
		ImGui/imgui_impl_win32.h
	)
	add_library(Example_ImGui_Docking_Lib OBJECT ${IMGUI_FILES})

	add_executable(Example_ImGui_Docking WIN32 ${SOURCE_FILES})

	target_link_libraries(Example_ImGui_Docking
		PUBLIC
		WickedEngine_Windows

		PRIVATE
		Example_ImGui_Docking_Lib
	)
	set(LIB_DXCOMPILER "dxcompiler.dll")
else()
	list(APPEND SOURCE_FILES
		main_SDL2.cpp
	)
	list(APPEND IMGUI_FILES
		ImGui/imgui_impl_sdl.cpp
		ImGui/imgui_impl_sdl.h
	)
	add_library(Example_ImGui_Docking_Lib OBJECT ${IMGUI_FILES})
	find_package(SDL2 REQUIRED)
	target_link_libraries(Example_ImGui_Docking_Lib PRIVATE SDL2::SDL2)
	add_executable(Example_ImGui_Docking ${SOURCE_FILES})

	target_link_libraries(Example_ImGui_Docking
		PUBLIC
		WickedEngine
		Threads::Threads

		PRIVATE
		Example_ImGui_Docking_Lib
	)

	# Copy shaders and font to build and source folders just to be safe:
	add_custom_command(
		TARGET Example_ImGui_Docking POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiPS.hlsl ${CMAKE_CURRENT_BINARY_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiVS.hlsl ${CMAKE_CURRENT_BINARY_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/Roboto-Medium.ttf ${CMAKE_CURRENT_BINARY_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/MaterialIcons-Regular.ttf ${CMAKE_CURRENT_BINARY_DIR}
	)

	set(LIB_DXCOMPILER "libdxcompiler.so")
endif ()

target_precompile_headers(Example_ImGui_Docking PRIVATE stdafx.h)

if (MSVC)
	set_property(TARGET Example_ImGui_Docking PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()
set_property(TARGET Example_ImGui_Docking PROPERTY UNITY_BUILD NO)

# Copy content to build folder:
add_dependencies(Example_ImGui_Docking Content)
add_custom_command(
	TARGET Example_ImGui_Docking POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WICKED_ROOT_DIR}/WickedEngine/${LIB_DXCOMPILER} ${CMAKE_CURRENT_BINARY_DIR}
)
