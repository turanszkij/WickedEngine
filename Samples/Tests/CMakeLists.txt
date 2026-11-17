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
		main_${PLATFORM}.cpp
	)

	add_executable(Tests ${SOURCE_FILES})

	target_link_libraries(Tests PUBLIC
		WickedEngine
	)

	set(LIB_DXCOMPILER "libdxcompiler.so")
endif ()

if(WICKED_ENABLE_IPO)
set_target_properties(Tests PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION ON
    INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF
)
endif()

target_precompile_headers(Tests PRIVATE stdafx.h)

add_executable(CustomShaderLifecycleTest
	CustomShaderLifecycleTest.cpp
)
target_link_libraries(CustomShaderLifecycleTest PRIVATE WickedEngine)
add_test(NAME CustomShaderLifecycleTest COMMAND CustomShaderLifecycleTest)
set_target_properties(CustomShaderLifecycleTest PROPERTIES FOLDER "Tests")

add_executable(OceanExclusionTest
	OceanExclusionTest.cpp
)
target_link_libraries(OceanExclusionTest PRIVATE WickedEngine)
add_test(NAME OceanExclusionTest COMMAND OceanExclusionTest)
set_target_properties(OceanExclusionTest PROPERTIES FOLDER "Tests")


# CPU-only tests remain usable on machines without a Vulkan driver. This opt-in
# test uses a headless device (including software Vulkan) and the vendored DXC.
option(WICKED_OCEAN_GPU_TEST "Enable the headless Vulkan ocean exclusion integration test" OFF)
if (WICKED_OCEAN_GPU_TEST AND (WIN32 OR PLATFORM MATCHES "^SDL"))
	add_executable(OceanExclusionVulkanTest OceanExclusionVulkanTest.cpp)
	target_link_libraries(OceanExclusionVulkanTest PRIVATE WickedEngine)
	target_compile_definitions(OceanExclusionVulkanTest PRIVATE
		WICKED_OCEAN_TEST_SHADER="${CMAKE_CURRENT_SOURCE_DIR}/shaders/oceanExclusionTestCS.hlsl"
		WICKED_ENGINE_SHADER_DIR="${WICKED_ROOT_DIR}/WickedEngine/shaders"
	)
	add_custom_command(TARGET OceanExclusionVulkanTest POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WICKED_ROOT_DIR}/WickedEngine/${LIB_DXCOMPILER} $<TARGET_FILE_DIR:OceanExclusionVulkanTest>
		VERBATIM
	)
	add_test(NAME OceanExclusionVulkanTest COMMAND OceanExclusionVulkanTest)
	set_tests_properties(OceanExclusionVulkanTest PROPERTIES LABELS "vulkan" TIMEOUT 90)
	set_target_properties(OceanExclusionVulkanTest PROPERTIES FOLDER "Tests")
endif ()


if (MSVC)
	set_property(TARGET Tests PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()

# Copy content to build folder:
add_dependencies(Tests Content)

add_custom_command(
	TARGET Tests POST_BUILD
	# Copy the shader library next to the executable
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WICKED_ROOT_DIR}/WickedEngine/${LIB_DXCOMPILER} $<TARGET_FILE_DIR:Tests>
	# Copy the resources
	COMMAND ${CMAKE_COMMAND} -E ${COPY_OR_SYMLINK_DIR_CMD} ${CMAKE_CURRENT_SOURCE_DIR}/images ${CMAKE_CURRENT_BINARY_DIR}/images
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/test_script.lua ${CMAKE_CURRENT_BINARY_DIR}/test_script.lua
	# Copy font test resources
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/yumin.ttf ${CMAKE_CURRENT_BINARY_DIR}/

	COMMENT "$<IF:$<BOOL:${WICKED_ENABLE_SYMLINKS}>,Symlinking,Copying> Tests resources"
	VERBATIM
)
