
if (WIN32)
	# TODO: Choose whether to use SDL2 on windows as well
	set(TARGET_NAME WickedEngine_Windows)
else ()
	set(TARGET_NAME WickedEngine_Linux)
	find_package(Vulkan REQUIRED)
	find_package(SDL2 REQUIRED)
	find_package(OpenImageDenoise QUIET)
	find_package(Threads REQUIRED)
	if(NOT ${OpenImageDenoise_FOUND})
		message("OpenImageDenoise not found, it will be disabled.")
	else()
		message("OpenImageDenoise Found.")
	endif()

	if(NOT TARGET SDL2::SDL2)
		# using old SDL2 cmake, lets create a SDL2 target ourselves
		find_library(SDL2_LIBRARY_FILE_LOCATION SDL2 REQUIRED)

		add_library(SDL2::SDL2 SHARED IMPORTED)
		set_target_properties(SDL2::SDL2 PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIRS}
			INTERFACE_LINK_LIBRARIES ${SDL2_LIBRARIES}
			IMPORTED_LOCATION ${SDL2_LIBRARY_FILE_LOCATION}
		)

		unset(SDL2_LIBRARY_FILE_LOCATION)
	endif()
endif()    

add_subdirectory(BULLET)
add_subdirectory(LUA)
add_subdirectory(Utility)

add_library(${TARGET_NAME} STATIC
	wiLoadingScreen.cpp
	wiLoadingScreen_BindLua.cpp
	wiApplication.cpp
	wiApplication_BindLua.cpp
	wiRenderPath_BindLua.cpp
	wiRenderPath2D.cpp
	wiRenderPath2D_BindLua.cpp
	wiRenderPath3D.cpp
	wiRenderPath3D_BindLua.cpp
	wiRenderPath3D_PathTracing.cpp
	wiSpriteAnim_BindLua.cpp
	wiTexture_BindLua.cpp
	wiMath_BindLua.cpp
	wiArchive.cpp
	wiAudio.cpp
	wiAudio_BindLua.cpp
	wiBacklog.cpp
	wiBacklog_BindLua.cpp
	wiEmittedParticle.cpp
	wiEventHandler.cpp
	wiFadeManager.cpp
	wiFFTGenerator.cpp
	wiFont.cpp
	wiGPUBVH.cpp
	wiGPUSortLib.cpp
	wiGraphicsDevice_DX12.cpp
	wiGraphicsDevice_Vulkan.cpp
	wiGUI.cpp
	wiHairParticle.cpp
	wiHelper.cpp
	wiImage.cpp
	wiImageParams_BindLua.cpp
	wiInitializer.cpp
	wiInput.cpp
	wiInput_BindLua.cpp
	wiPrimitive.cpp
	wiPrimitive_BindLua.cpp
	wiJobSystem.cpp
	wiLua.cpp
	wiMath.cpp
	wiNetwork_BindLua.cpp
	wiNetwork_Linux.cpp
	wiNetwork_Windows.cpp
	wiNetwork_UWP.cpp
	wiOcean.cpp
	wiPhysics_Bullet.cpp
	wiProfiler.cpp
	wiRandom.cpp
	wiRawInput.cpp
	wiRectPacker.cpp
	wiRenderer.cpp
	wiRenderer_BindLua.cpp
	wiResourceManager.cpp
	wiScene.cpp
	wiScene_BindLua.cpp
	wiScene_Serializers.cpp
	wiSDLInput.cpp
	wiSprite.cpp
	wiSprite_BindLua.cpp
	wiSpriteFont.cpp
	wiSpriteFont_BindLua.cpp
	wiArguments.cpp
	wiTextureHelper.cpp
	wiVersion.cpp
	wiXInput.cpp
	wiShaderCompiler.cpp
)
add_library(WickedEngine ALIAS ${TARGET_NAME})

target_include_directories(${TARGET_NAME} PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

set(WICKEDENGINE_STATIC_LIBRARIES
	Bullet
	LUA
	Utility
)

if (WIN32)
	target_compile_definitions(${TARGET_NAME} PUBLIC
		UNICODE _UNICODE
	)

	set(LIBDXCOMPILER "dxcompiler.dll")
else ()
	target_link_libraries(${TARGET_NAME} PUBLIC
		Threads::Threads
		Vulkan::Vulkan
		SDL2::SDL2
		$<$<BOOL:${OpenImageDenoise_FOUND}>:OpenImageDenoise> # links OpenImageDenoise only if it's found
	)
	set(WICKEDENGINE_STATIC_LIBRARIES ${WICKEDENGINE_STATIC_LIBRARIES} FAudio)

	target_link_libraries(${TARGET_NAME} PRIVATE dl)

	set(LIBDXCOMPILER "libdxcompiler.so")
endif()

target_link_libraries(${TARGET_NAME} PUBLIC ${WICKEDENGINE_STATIC_LIBRARIES})

if (PLATFORM MATCHES "SDL2")
	target_compile_definitions(${TARGET_NAME} PUBLIC SDL2=1)
endif()

add_library(dxcompiler SHARED IMPORTED)
set_property(TARGET dxcompiler PROPERTY
		IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/WickedEngine/${LIBDXCOMPILER} )

# Copy the shader library next to the executable
add_custom_command(
		TARGET ${TARGET_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/WickedEngine/${LIBDXCOMPILER} ${CMAKE_CURRENT_BINARY_DIR}
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/WickedEngine/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders
)


# OFFLINE SHADER COMPILER
add_executable(offlineshadercompiler
		offlineshadercompiler.cpp
)

target_link_libraries(offlineshadercompiler
		PUBLIC ${TARGET_NAME})

# Create importable target here
include(CMakePackageConfigHelpers)
set(INSTALL_CONFIGDIR "${CMAKE_BINARY_DIR}/cmake")
#set(INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/cmake/WickedEngine")

#set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")
set_target_properties("${TARGET_NAME}" PROPERTIES EXPORT_NAME "${TARGET_NAME}")


install(TARGETS "${TARGET_NAME}" ${WICKEDENGINE_STATIC_LIBRARIES}
		EXPORT Engine-Targets
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine")

export(EXPORT Engine-Targets
		FILE "${CMAKE_BINARY_DIR}/cmake/WickedEngineTargets.cmake"
		NAMESPACE WickedEngine::
		)

#install(EXPORT Engine-Targets
#		FILE WickedEngineTargets.cmake
#		NAMESPACE WickedEngine
#		DESTINATION ${INSTALL_CONFIGDIR})

configure_package_config_file(
		${CMAKE_CURRENT_SOURCE_DIR}/WickedEngineConfig.cmake.in
		${CMAKE_BINARY_DIR}/cmake/WickedEngineConfig.cmake
		INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/WickedEngineConfig.cmake
#		DESTINATION ${INSTALL_CONFIGDIR}
#)
