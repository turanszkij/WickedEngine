set(MIN_OpenImageDenoise_VERSION "2.0")


include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
set(INSTALL_LOCAL_CONFIGDIR "${CMAKE_BINARY_DIR}/cmake")
set(INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/cmake/WickedEngine")

if (WICKED_DYNAMIC_LIBRARY)
	set(WICKED_LIBRARY_TYPE SHARED)
	message(STATUS "Building WickedEngine as a shared library")
else()
	set(WICKED_LIBRARY_TYPE STATIC)
	message(STATUS "Building WickedEngine as a static library")
endif()

if (WIN32)
	# TODO: Choose whether to use SDL2 on windows as well
	set(TARGET_NAME WickedEngine_Windows)
else ()
	set(TARGET_NAME WickedEngine_Linux)
	find_package(SDL2 REQUIRED)
	find_package(OpenImageDenoise "${MIN_OpenImageDenoise_VERSION}" QUIET)
	find_package(Threads REQUIRED)
	if(NOT ${OpenImageDenoise_FOUND})
		message("OpenImageDenoise>=${MIN_OpenImageDenoise_VERSION} not found, it will be disabled.")
	else()
		message("OpenImageDenoise ${OpenImageDenoise_VERSION} Found.")
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

	# minimum sdl version is 2.0.14 for controller LED support
	if((${SDL_VERSION_MAJOR} GREATER_EQUAL 2) AND (${SDL2_VERSION_MINOR} GREATER_EQUAL 0) AND (${SDL2_VERSION_PATCH} GREATER_EQUAL 14))
		add_compile_definitions(SDL2_FEATURE_CONTROLLER_LED=1)
	endif()
endif()

add_subdirectory(BULLET)
add_subdirectory(LUA)
add_subdirectory(Utility)

set(HEADER_FILES
		WickedEngine.h
		CommonInclude.h
		logo.h
		sdl2.h
		wiApplication.h
		wiApplication_BindLua.h
		wiArchive.h
		wiArguments.h
		wiAudio.h
		wiAudio_BindLua.h
		wiBacklog.h
		wiBacklog_BindLua.h
		wiCanvas.h
		wiColor.h
		wiECS.h
		wiEmittedParticle.h
		wiEnums.h
		wiEventHandler.h
		wiFadeManager.h
		wiFFTGenerator.h
		wiFont.h
		wiGPUBVH.h
		wiGPUSortLib.h
		wiGraphics.h
		wiGraphicsDevice.h
		wiGraphicsDevice_DX12.h
		wiGraphicsDevice_Vulkan.h
		wiGUI.h
		wiHairParticle.h
		wiHelper.h
		wiImage.h
		wiImageParams_BindLua.h
		wiInitializer.h
		wiInput.h
		wiInput_BindLua.h
		wiJobSystem.h
		wiLoadingScreen.h
		wiLoadingScreen_BindLua.h
		wiLua.h
		wiLua_Globals.h
		wiLuna.h
		wiMath.h
		wiMath_BindLua.h
		wiNetwork.h
		wiNetwork_BindLua.h
		wiNoise.h
		wiOcean.h
		wiPhysics.h
		wiPhysics_BindLua.h
		wiPlatform.h
		wiPrimitive.h
		wiPrimitive_BindLua.h
		wiProfiler.h
		wiRandom.h
		wiRawInput.h
		wiRectPacker.h
		wiRenderer.h
		wiRenderer_BindLua.h
		wiRenderPath.h
		wiRenderPath2D.h
		wiRenderPath2D_BindLua.h
		wiRenderPath3D.h
		wiRenderPath3D_BindLua.h
		wiRenderPath3D_PathTracing.h
		wiRenderPath_BindLua.h
		wiResourceManager.h
		wiScene.h
		wiScene_BindLua.h
		wiScene_Decl.h
		wiScene_Components.h
		wiSDLInput.h
		wiShaderCompiler.h
		wiSheenLUT.h
		wiSpinLock.h
		wiSprite.h
		wiSprite_BindLua.h
		wiSpriteAnim_BindLua.h
		wiSpriteFont.h
		wiSpriteFont_BindLua.h
		wiTexture_BindLua.h
		wiTextureHelper.h
		wiTimer.h
		wiUnorderedMap.h
		wiUnorderedSet.h
		wiVector.h
		wiVersion.h
		wiXInput.h
		wiConfig.h
		wiTerrain.h
		wiAllocator.h
		wiBVH.h
		wiLocalization.h
		wiVideo.h
		)

add_library(${TARGET_NAME} ${WICKED_LIBRARY_TYPE}
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
	wiPhysics_BindLua.cpp
	wiProfiler.cpp
	wiRandom.cpp
	wiRawInput.cpp
	wiRenderer.cpp
	wiRenderer_BindLua.cpp
	wiResourceManager.cpp
	wiScene.cpp
	wiScene_Components.cpp
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
	wiConfig.cpp
	wiTerrain.cpp
	wiLocalization.cpp
	wiVideo.cpp
	${HEADER_FILES}
)
add_library(WickedEngine ALIAS ${TARGET_NAME})
set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${HEADER_FILES}")

target_include_directories(${TARGET_NAME} SYSTEM PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine>
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

	target_compile_options(${TARGET_NAME} PRIVATE /W3)

	set(LIBDXCOMPILER "dxcompiler.dll")
else ()
	# `ska::flat_hash_map` has issues on linux because of the hash function being identity
	# in same cases. Use `robin_hood::unordered_flat_map` instead
	target_compile_definitions(${TARGET_NAME} PUBLIC WI_UNORDERED_MAP_TYPE=2)

	target_link_libraries(${TARGET_NAME} PUBLIC
		Threads::Threads
		SDL2::SDL2
		$<$<BOOL:${OpenImageDenoise_FOUND}>:OpenImageDenoise> # links OpenImageDenoise only if it's found
	)
	set(WICKEDENGINE_STATIC_LIBRARIES ${WICKEDENGINE_STATIC_LIBRARIES} FAudio)

	# add some warnings and set them as errors
	# read more details here: https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
	target_compile_options(${TARGET_NAME} PRIVATE
			-Wuninitialized
			-Wmaybe-uninitialized
			-Wwrite-strings
			-Winit-self
			-Wreturn-type
			-Wreorder
			-Werror=delete-non-virtual-dtor
			-Werror
			#uncomment this to stop the compilation at the first error
			# -Wfatal-errors
			)

	target_link_libraries(${TARGET_NAME} PRIVATE dl)

	set(LIBDXCOMPILER "libdxcompiler.so")
endif()

target_link_libraries(${TARGET_NAME} PUBLIC ${WICKEDENGINE_STATIC_LIBRARIES})

if (PLATFORM MATCHES "SDL2")
	target_compile_definitions(${TARGET_NAME} PUBLIC SDL2=1)
endif()

add_library(dxcompiler SHARED IMPORTED)
set_property(TARGET dxcompiler PROPERTY
		IMPORTED_LOCATION ${WICKED_ROOT_DIR}/WickedEngine/${LIBDXCOMPILER} )

# Copy the shader library next to the executable
add_custom_command(
		TARGET ${TARGET_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WICKED_ROOT_DIR}/WickedEngine/${LIBDXCOMPILER} ${CMAKE_CURRENT_BINARY_DIR}
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${WICKED_ROOT_DIR}/WickedEngine/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders
)


# OFFLINE SHADER COMPILER
add_executable(offlineshadercompiler
		offlineshadercompiler.cpp
)

target_link_libraries(offlineshadercompiler
		PUBLIC ${TARGET_NAME})

install(TARGETS offlineshadercompiler
		RUNTIME DESTINATION "${CMAKE_INSTALL_LIBDIR}/WickedEngine")

install(DIRECTORY "${WICKED_ROOT_DIR}/Content"
		DESTINATION "${CMAKE_INSTALL_LIBDIR}/WickedEngine")

#### LOCAL FOLDER INSTALLATION
# Create importable target here

#set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")
set_target_properties("${TARGET_NAME}" PROPERTIES EXPORT_NAME "${TARGET_NAME}")


install(TARGETS "${TARGET_NAME}" ${WICKEDENGINE_STATIC_LIBRARIES}
		EXPORT Engine-Targets
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/WickedEngine"
		PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine")

install(FILES ${LIBDXCOMPILER}
		DESTINATION "${CMAKE_INSTALL_LIBDIR}/WickedEngine")

export(EXPORT Engine-Targets
		FILE "${CMAKE_BINARY_DIR}/cmake/WickedEngineTargets.cmake"
		NAMESPACE WickedEngine::
		)

install(EXPORT Engine-Targets
		FILE WickedEngineTargets.cmake
		NAMESPACE WickedEngine::
		DESTINATION ${INSTALL_CONFIGDIR})

set(_CONFIG_INSTALL_DIR_INCLUDE "${WICKED_ROOT_DIR}")
set(_CONFIG_INSTALL_DIR_LIB "${WICKED_ROOT_DIR}")
configure_package_config_file(
		${CMAKE_CURRENT_SOURCE_DIR}/WickedEngineConfig.cmake.in
		${CMAKE_BINARY_DIR}/cmake/WickedEngineConfig.cmake
		INSTALL_DESTINATION ${INSTALL_LOCAL_CONFIGDIR}
)
set(_CONFIG_INSTALL_DIR_INCLUDE "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
set(_CONFIG_INSTALL_DIR_LIB "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
configure_package_config_file(
		${CMAKE_CURRENT_SOURCE_DIR}/WickedEngineConfig.cmake.in
		${CMAKE_BINARY_DIR}/cmake/install/WickedEngineConfig.cmake
		INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

install(FILES ${CMAKE_BINARY_DIR}/cmake/install/WickedEngineConfig.cmake
		DESTINATION ${INSTALL_CONFIGDIR}
)

# Shaders
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine"
		PATTERN "spirv" EXCLUDE
		PATTERN "hlsl6" EXCLUDE
		PATTERN "*.vcxitems*" EXCLUDE
		)
