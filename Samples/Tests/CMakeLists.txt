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


# GPU integration tests are opt-in: CPU-only tests need no graphics driver.
# Each CTest invocation runs the same cases on one explicitly selected backend.
option(WICKED_OCEAN_GPU_TEST "Enable ocean exclusion graphics integration tests" OFF)
if (WICKED_OCEAN_GPU_TEST AND (WIN32 OR PLATFORM MATCHES "^SDL"))
	if (APPLE)
		set(_ocean_default_backends metal)
		set(_ocean_supported_backends metal)
	elseif (WIN32)
		set(_ocean_default_backends dx12 vulkan)
		set(_ocean_supported_backends dx12 vulkan)
	else ()
		set(_ocean_default_backends vulkan)
		set(_ocean_supported_backends vulkan)
	endif ()
	set(WICKED_OCEAN_GPU_BACKENDS "${_ocean_default_backends}" CACHE STRING
		"Semicolon-separated graphics backends to test (vulkan, dx12, metal as supported by the platform)")
	if (NOT WICKED_OCEAN_GPU_BACKENDS)
		message(FATAL_ERROR "WICKED_OCEAN_GPU_BACKENDS must select at least one backend")
	endif ()
	foreach(backend IN LISTS WICKED_OCEAN_GPU_BACKENDS)
		if (NOT backend IN_LIST _ocean_supported_backends)
			message(FATAL_ERROR "Unsupported ocean test backend '${backend}' on this platform; choose from: ${_ocean_supported_backends}")
		endif ()
	endforeach()
	add_executable(OceanExclusionGraphicsTest OceanExclusionGraphicsTest.cpp)
	target_link_libraries(OceanExclusionGraphicsTest PRIVATE WickedEngine)
	target_compile_definitions(OceanExclusionGraphicsTest PRIVATE
		WICKED_OCEAN_TEST_SHADER="${CMAKE_CURRENT_SOURCE_DIR}/shaders/oceanExclusionTestCS.hlsl"
		WICKED_ENGINE_SHADER_DIR="${WICKED_ROOT_DIR}/WickedEngine/shaders"
	)
	if (APPLE)
		set(_ocean_dxcompiler "libdxcompiler.dylib")
	else ()
		set(_ocean_dxcompiler "${LIB_DXCOMPILER}")
	endif ()
	add_custom_command(TARGET OceanExclusionGraphicsTest POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WICKED_ROOT_DIR}/WickedEngine/${_ocean_dxcompiler} $<TARGET_FILE_DIR:OceanExclusionGraphicsTest>
		VERBATIM
	)
	foreach(backend IN LISTS WICKED_OCEAN_GPU_BACKENDS)
		add_test(NAME OceanExclusionGraphicsTest_${backend} COMMAND OceanExclusionGraphicsTest ${backend})
		set_tests_properties(OceanExclusionGraphicsTest_${backend} PROPERTIES LABELS "graphics;${backend}" TIMEOUT 90)
	endforeach()
	set_target_properties(OceanExclusionGraphicsTest PROPERTIES FOLDER "Tests")
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
