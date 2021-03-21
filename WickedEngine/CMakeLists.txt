
if (WIN32)
	# TODO: Choose whether to use SDL2 on windows as well
	set(TARGET_NAME WickedEngine_Windows)
else ()
	set(TARGET_NAME WickedEngine)
	find_package(Vulkan REQUIRED)
	find_package(SDL2 REQUIRED)

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
add_subdirectory(shaders)

add_library(${TARGET_NAME} STATIC
	LoadingScreen.cpp
	LoadingScreen_BindLua.cpp
	MainComponent.cpp
	MainComponent_BindLua.cpp
	Matrix_BindLua.cpp
	RenderPath_BindLua.cpp
	RenderPath2D.cpp
	RenderPath2D_BindLua.cpp
	RenderPath3D.cpp
	RenderPath3D_BindLua.cpp
	RenderPath3D_PathTracing.cpp
	SpriteAnim_BindLua.cpp
	Texture_BindLua.cpp
	Vector_BindLua.cpp
	wiArchive.cpp
	wiAudio.cpp
	wiAudio_BindLua.cpp
	wiBackLog.cpp
	wiBackLog_BindLua.cpp
	wiEmittedParticle.cpp
	wiEvent.cpp
	wiFadeManager.cpp
	wiFFTGenerator.cpp
	wiFont.cpp
	wiGPUBVH.cpp
	wiGPUSortLib.cpp
	wiGraphicsDevice.cpp
	wiGraphicsDevice_DX11.cpp
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
	wiIntersect.cpp
	wiIntersect_BindLua.cpp
	wiJobSystem.cpp
	wiLua.cpp
	wiMath.cpp
	wiNetwork_BindLua.cpp
	wiNetwork_Linux.cpp
	wiNetwork_Windows.cpp
	wiNetwork_UWP.cpp
	wiOcean.cpp
	wiPhysicsEngine_Bullet.cpp
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
	wiStartupArguments.cpp
	wiTextureHelper.cpp
	wiVersion.cpp
	wiWidget.cpp
	wiXInput.cpp
	wiBlueNoise.cpp
	wiShaderCompiler.cpp
)

target_include_directories(${TARGET_NAME} PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET_NAME} PUBLIC
	Bullet
	LUA
	Utility
)

if (WIN32)
	target_compile_definitions(${TARGET_NAME} PUBLIC
		UNICODE _UNICODE
	)
	
else ()
	target_link_libraries(${TARGET_NAME} PUBLIC
		Vulkan::Vulkan
		SDL2::SDL2
	)

	target_link_libraries(${TARGET_NAME} PRIVATE dl)

	add_dependencies(${TARGET_NAME}
		Shaders_SPIRV
	)
endif()


if (PLATFORM MATCHES "SDL2")
	target_compile_definitions(${TARGET_NAME} PUBLIC SDL2=1)
endif()
