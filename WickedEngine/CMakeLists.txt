find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

add_subdirectory(BULLET)
add_subdirectory(LUA)
add_subdirectory(Utility)
add_subdirectory(shaders)

add_library(WickedEngine STATIC
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
    # wiGraphicsDevice_DX11.cpp
    # wiGraphicsDevice_DX12.cpp
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
    # wiNetwork_UWP.cpp
    # wiNetwork_Windows.cpp
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
    wiTimer.cpp
    wiVersion.cpp
    wiWidget.cpp
    wiXInput.cpp
)

target_include_directories(WickedEngine PUBLIC
	${SDL2_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(WickedEngine PUBLIC
	Vulkan::Vulkan
	${SDL2_LIBRARIES}
	Bullet
	LUA
	Utility
)

add_dependencies(WickedEngine
	Shaders_SPIRV)

if (PLATFORM MATCHES "SDL2")
    target_compile_definitions(WickedEngine PUBLIC SDL2=1)
endif()
