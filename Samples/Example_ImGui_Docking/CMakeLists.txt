cmake_minimum_required(VERSION 3.19)

project(Example_ImGui_Docking)

if (NOT WIN32)
	find_package(Threads REQUIRED)
endif ()

set(SOURCE_FILES
	Example_ImGui_Docking.cpp
	Example_ImGui_Docking.h
	../../Editor/ModelImporter_GLTF.cpp
)

if(WIN32)
	list(APPEND SOURCE_FILES
		main_Windows.cpp
		main_Windows.h
		Tests.rc
	)
	add_executable(${PROJECT_NAME} WIN32 ${SOURCE_FILES})
else()
	list(APPEND SOURCE_FILES
		main_SDL2.cpp
	)
	add_executable(${PROJECT_NAME} ${SOURCE_FILES})
endif()

setup_wicked_app(${PROJECT_NAME})

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
	list(APPEND IMGUI_FILES
		ImGui/imgui_impl_win32.cpp
		ImGui/imgui_impl_win32.h
	)
	add_library(Example_ImGui_Docking_Lib OBJECT ${IMGUI_FILES})

	target_link_libraries(${PROJECT_NAME}
		Example_ImGui_Docking_Lib
	)
	
else()

	list(APPEND IMGUI_FILES
		ImGui/imgui_impl_sdl.cpp
		ImGui/imgui_impl_sdl.h
	)
	add_library(Example_ImGui_Docking_Lib OBJECT ${IMGUI_FILES})
	find_package(SDL2 REQUIRED)
	target_link_libraries(Example_ImGui_Docking_Lib PRIVATE SDL2::SDL2)
	add_executable(Example_ImGui_Docking ${SOURCE_FILES})

	target_link_libraries(${PROJECT_NAME}
		PUBLIC
		Threads::Threads

		PRIVATE
		Example_ImGui_Docking_Lib
	)
endif()

# Copy shaders
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiPS.hlsl 
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiVS.hlsl 
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying ImGui shaders to output directory"
)

if(WICKED_USE_IPO)
set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION ON
    INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF
)
endif()

target_precompile_headers(Example_ImGui_Docking PRIVATE stdafx.h)

if (MSVC)
	set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()
set_property(TARGET ${PROJECT_NAME} PROPERTY UNITY_BUILD NO)

# Copy content to build folder:
add_dependencies(${PROJECT_NAME} Content)

add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WICKED_ROOT_DIR}/WickedEngine/${LIB_DXCOMPILER} ${CMAKE_CURRENT_BINARY_DIR}
)
