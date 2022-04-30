
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
set(INSTALL_LOCAL_CONFIGDIR "${CMAKE_BINARY_DIR}/cmake")
set(INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/cmake/WickedEngine")

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
		wiOcean.h
		wiPhysics.h
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
		)

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
	${HEADER_FILES}
)
add_library(WickedEngine ALIAS ${TARGET_NAME})
set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${HEADER_FILES}")

target_include_directories(${TARGET_NAME} PUBLIC
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

install(TARGETS offlineshadercompiler
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/WickedEngine)

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

set(_CONFIG_INSTALL_DIR_INCLUDE "${CMAKE_SOURCE_DIR}")
set(_CONFIG_INSTALL_DIR_LIB "${CMAKE_SOURCE_DIR}")
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
install(FILES shaders/ShaderInterop_FFTGenerator.h
		shaders/blur_bilateral_float1CS.hlsl
		shaders/blur_bilateral_float3CS.hlsl
		shaders/blur_bilateral_float4CS.hlsl
		shaders/blur_bilateral_unorm1CS.hlsl
		shaders/blur_bilateral_unorm4CS.hlsl
		shaders/blur_bilateral_wide_float1CS.hlsl
		shaders/blur_bilateral_wide_float3CS.hlsl
		shaders/blur_bilateral_wide_float4CS.hlsl
		shaders/blur_bilateral_wide_unorm1CS.hlsl
		shaders/blur_bilateral_wide_unorm4CS.hlsl
		shaders/blur_gaussian_float1CS.hlsl
		shaders/blur_gaussian_float3CS.hlsl
		shaders/blur_gaussian_unorm1CS.hlsl
		shaders/blur_gaussian_unorm4CS.hlsl
		shaders/blur_gaussian_wide_float1CS.hlsl
		shaders/blur_gaussian_wide_float3CS.hlsl
		shaders/blur_gaussian_wide_float4CS.hlsl
		shaders/blur_gaussian_wide_unorm1CS.hlsl
		shaders/blur_gaussian_wide_unorm4CS.hlsl
		shaders/captureImpostorPS_albedo.hlsl
		shaders/captureImpostorPS_normal.hlsl
		shaders/captureImpostorPS_surface.hlsl
		shaders/circle.hlsli
		shaders/cone.hlsli
		shaders/copytexture2D_unorm4CS.hlsl
		shaders/copytexture2D_unorm4_borderexpandCS.hlsl
		shaders/cube.hlsli
		shaders/cubeShadowGS_emulation.hlsl
		shaders/cubeShadowGS_transparent_emulation.hlsl
		shaders/cubeShadowVS_alphatest_emulation.hlsl
		shaders/cubeShadowVS_emulation.hlsl
		shaders/cubeShadowVS_transparent_emulation.hlsl
		shaders/cylinder.hlsli
		shaders/depthoffield_mainCS_cheap.hlsl
		shaders/depthoffield_mainCS_earlyexit.hlsl
		shaders/depthoffield_prepassCS_earlyexit.hlsl
		shaders/emittedparticlePS_soft_distortion.hlsl
		shaders/emittedparticlePS_soft_lighting.hlsl
		shaders/emittedparticle_emitCS_FROMMESH.hlsl
		shaders/emittedparticle_emitCS_volume.hlsl
		shaders/emittedparticle_simulateCS_DEPTHCOLLISIONS.hlsl
		shaders/emittedparticle_simulateCS_SORTING.hlsl
		shaders/emittedparticle_simulateCS_SORTING_DEPTHCOLLISIONS.hlsl
		shaders/envMapPS_terrain.hlsl
		shaders/envMapVS_emulation.hlsl
		shaders/envMap_skyGS_emulation.hlsl
		shaders/envMap_skyPS_dynamic.hlsl
		shaders/envMap_skyVS_emulation.hlsl
		shaders/fxaa.hlsli
		shaders/generateMIPChain2DCS_unorm4.hlsl
		shaders/generateMIPChain3DCS_unorm4.hlsl
		shaders/generateMIPChainCubeArrayCS_unorm4.hlsl
		shaders/generateMIPChainCubeCS_unorm4.hlsl
		shaders/icosphere.hlsli
		shaders/lightCullingCS_ADVANCED.hlsl
		shaders/lightCullingCS_ADVANCED_DEBUG.hlsl
		shaders/lightCullingCS_DEBUG.hlsl
		shaders/lightVisualizerPS.hlsl
		shaders/motionblurCS_cheap.hlsl
		shaders/motionblurCS_earlyexit.hlsl
		shaders/msao_blurupsampleCS_blendout.hlsl
		shaders/msao_blurupsampleCS_premin.hlsl
		shaders/msao_blurupsampleCS_premin_blendout.hlsl
		shaders/msao_interleaveCS.hlsl
		shaders/objectDS.hlsl
		shaders/objectHS.hlsl
		shaders/objectPS.hlsl
		shaders/objectPS_anisotropic.hlsl
		shaders/objectPS_cartoon.hlsl
		shaders/objectPS_clearcoat.hlsl
		shaders/objectPS_cloth.hlsl
		shaders/objectPS_cloth_clearcoat.hlsl
		shaders/objectPS_debug.hlsl
		shaders/objectPS_planarreflection.hlsl
		shaders/objectPS_pom.hlsl
		shaders/objectPS_terrain.hlsl
		shaders/objectPS_transparent.hlsl
		shaders/objectPS_transparent_anisotropic.hlsl
		shaders/objectPS_transparent_cartoon.hlsl
		shaders/objectPS_transparent_clearcoat.hlsl
		shaders/objectPS_transparent_cloth.hlsl
		shaders/objectPS_transparent_cloth_clearcoat.hlsl
		shaders/objectPS_transparent_planarreflection.hlsl
		shaders/objectPS_transparent_pom.hlsl
		shaders/objectPS_transparent_unlit.hlsl
		shaders/objectPS_unlit.hlsl
		shaders/objectPS_voxelizer_terrain.hlsl
		shaders/objectPS_water.hlsl
		shaders/objectVS_common.hlsl
		shaders/objectVS_common_tessellation.hlsl
		shaders/objectVS_debug.hlsl
		shaders/objectVS_voxelizer.hlsl
		shaders/oceanSurfaceHF.hlsli
		shaders/oceanSurfaceSimplePS.hlsl
		shaders/quad.hlsli
		shaders/raytraceCS_rtapi.hlsl
		shaders/raytrace_debugbvhPS.hlsl
		shaders/renderlightmapPS_rtapi.hlsl
		shaders/renderlightmapVS.hlsl
		shaders/shadingRateClassificationCS_DEBUG.hlsl
		shaders/upsample_bilateralPS.hlsl
		shaders/upsample_bilateral_float1CS.hlsl
		shaders/upsample_bilateral_uint4CS.hlsl
		shaders/upsample_bilateral_unorm1CS.hlsl
		shaders/upsample_bilateral_unorm4CS.hlsl
		shaders/uvsphere.hlsli
		shaders/vertexcolorPS.hlsl
		shaders/vertexcolorVS.hlsl
		shaders/volumeLightHF.hlsli
		shaders/volumetricCloudsHF.hlsli
		shaders/volumetriclight_pointVS.hlsl
		shaders/volumetriclight_spotVS.hlsl
		shaders/voxelSceneCopyClearCS_TemporalSmoothing.hlsl
		shaders/ShaderInterop_Ocean.h
		shaders/ShaderInterop_Raytracing.h
		shaders/rtshadowCS.hlsl
		shaders/volumetricLightHF.hlsli
		shaders/voxelHF.hlsli
		shaders/ShaderInterop_HairParticle.h
		shaders/cubeShadowVS.hlsl
		shaders/cubeShadowVS_alphatest.hlsl
		shaders/cubeShadowVS_transparent.hlsl
		shaders/hairparticlePS_simple.hlsl
		shaders/impostorPS_prepass.hlsl
		shaders/impostorPS_simple.hlsl
		shaders/objectDS_prepass.hlsl
		shaders/objectDS_prepass_alphatest.hlsl
		shaders/objectDS_simple.hlsl
		shaders/objectHS_prepass.hlsl
		shaders/objectHS_prepass_alphatest.hlsl
		shaders/objectHS_simple.hlsl
		shaders/objectPS_prepass.hlsl
		shaders/objectPS_prepass_alphatest.hlsl
		shaders/objectPS_simple.hlsl
		shaders/objectVS_prepass.hlsl
		shaders/objectVS_prepass_alphatest.hlsl
		shaders/objectVS_prepass_alphatest_tessellation.hlsl
		shaders/objectVS_prepass_tessellation.hlsl
		shaders/objectVS_simple_tessellation.hlsl
		shaders/shadowPS_alphatest.hlsl
		shaders/shadowPS_water.hlsl
		shaders/shadowVS.hlsl
		shaders/shadowVS_alphatest.hlsl
		shaders/shadowVS_transparent.hlsl
		shaders/surfel_raytraceCS_rtapi.hlsl
		shaders/visibility_resolveCS_MSAA.hlsl
		shaders/skinningCS.hlsl
		shaders/generateMIPChain3DCS_float4.hlsl
		shaders/ColorSpaceUtility.hlsli
		shaders/PixelPacking_R11G11B10.hlsli
		shaders/ShaderInterop_Weather.h
		shaders/bloomseparateCS.hlsl
		shaders/bitonicSortHF.hlsli
		shaders/blur_gaussian_float4CS.hlsl
		shaders/bvh_hierarchyCS.hlsl
		shaders/bvh_propagateaabbCS.hlsl
		shaders/chromatic_aberrationCS.hlsl
		shaders/copytexture2D_float4CS.hlsl
		shaders/copytexture2D_float4_borderexpandCS.hlsl
		shaders/cubeMapPS.hlsl
		shaders/cullingShaderHF.hlsli
		shaders/depthOfFieldHF.hlsli
		shaders/depthoffield_kickjobsCS.hlsl
		shaders/depthoffield_mainCS.hlsl
		shaders/depthoffield_neighborhoodMaxCOCCS.hlsl
		shaders/depthoffield_postfilterCS.hlsl
		shaders/depthoffield_prepassCS.hlsl
		shaders/depthoffield_tileMaxCOC_horizontalCS.hlsl
		shaders/depthoffield_tileMaxCOC_verticalCS.hlsl
		shaders/depthoffield_upsampleCS.hlsl
		shaders/downsample4xCS.hlsl
		shaders/downsampleDepthBuffer4xPS.hlsl
		shaders/emittedparticle_finishUpdateCS.hlsl
		shaders/emittedparticle_kickoffUpdateCS.hlsl
		shaders/emittedparticle_sphdensityCS.hlsl
		shaders/emittedparticle_sphpartitionCS.hlsl
		shaders/emittedparticle_sphpartitionoffsetsCS.hlsl
		shaders/emittedparticle_sphpartitionoffsetsresetCS.hlsl
		shaders/envMap_skyPS_static.hlsl
		shaders/envMap_skyVS.hlsl
		shaders/filterEnvMapCS.hlsl
		shaders/forceFieldPlaneVisualizerVS.hlsl
		shaders/forceFieldPointVisualizerVS.hlsl
		shaders/forceFieldVisualizerPS.hlsl
		shaders/fxaaCS.hlsl
		shaders/generateMIPChainCubeArrayCS_float4.hlsl
		shaders/generateMIPChainCubeCS_float4.hlsl
		shaders/hairparticle_finishUpdateCS.hlsl
		shaders/hbaoCS.hlsl
		shaders/impostorPS.hlsl
		shaders/lensFlarePS.hlsl
		shaders/lensFlareVS.hlsl
		shaders/lightCullingCS.hlsl
		shaders/lightShaftsCS.hlsl
		shaders/luminancePass1CS.hlsl
		shaders/luminancePass2CS.hlsl
		shaders/motionblur_kickjobsCS.hlsl
		shaders/motionblur_neighborhoodMaxVelocityCS.hlsl
		shaders/motionblur_tileMaxVelocity_verticalCS.hlsl
		shaders/msao_blurupsampleCS.hlsl
		shaders/msao_preparedepthbuffers1CS.hlsl
		shaders/msao_preparedepthbuffers2CS.hlsl
		shaders/normalsfromdepthCS.hlsl
		shaders/objectHF_tessellation.hlsli
		shaders/objectPS_hologram.hlsl
		shaders/oceanSimulatorCS.hlsl
		shaders/oceanSurfaceVS.hlsl
		shaders/oceanUpdateGradientFoldingCS.hlsl
		shaders/outlinePS.hlsl
		shaders/paint_textureCS.hlsl
		shaders/raytrace_screenVS.hlsl
		shaders/resolveMSAADepthStencilCS.hlsl
		shaders/rtao_denoise_filterCS.hlsl
		shaders/rtshadow_denoise_filterCS.hlsl
		shaders/sharpenCS.hlsl
		shaders/skyAtmosphere.hlsli
		shaders/skyAtmosphere_multiScatteredLuminanceLutCS.hlsl
		shaders/skyAtmosphere_skyLuminanceLutCS.hlsl
		shaders/skyAtmosphere_skyViewLutCS.hlsl
		shaders/skyAtmosphere_transmittanceLutCS.hlsl
		shaders/skyHF.hlsli
		shaders/skyPS_dynamic.hlsl
		shaders/skyPS_static.hlsl
		shaders/skyVS.hlsl
		shaders/sphereVS.hlsl
		shaders/sunPS.hlsl
		shaders/surfel_binningCS.hlsl
		shaders/surfel_gridoffsetsCS.hlsl
		shaders/surfel_gridresetCS.hlsl
		shaders/tileFrustumsCS.hlsl
		shaders/tonemapCS.hlsl
		shaders/upsample_bilateral_float4CS.hlsl
		shaders/vPointLightVS.hlsl
		shaders/vSpotLightVS.hlsl
		shaders/volumetricCloud_curlnoiseCS.hlsl
		shaders/volumetricCloud_detailnoiseCS.hlsl
		shaders/volumetricCloud_renderCS.hlsl
		shaders/volumetricCloud_reprojectCS.hlsl
		shaders/volumetricCloud_shapenoiseCS.hlsl
		shaders/volumetricCloud_temporalCS.hlsl
		shaders/volumetricCloud_weathermapCS.hlsl
		shaders/volumetricLight_DirectionalPS.hlsl
		shaders/volumetricLight_PointPS.hlsl
		shaders/volumetricLight_SpotPS.hlsl
		shaders/volumetriclight_directionalVS.hlsl
		shaders/voxelClearOnlyNormalCS.hlsl
		shaders/voxelConeTracingHF.hlsli
		shaders/voxelGS.hlsl
		shaders/voxelRadianceSecondaryBounceCS.hlsl
		shaders/voxelSceneCopyClearCS.hlsl
		shaders/voxelVS.hlsl
		shaders/ShaderInterop.h
		shaders/ShaderInterop_BVH.h
		shaders/ShaderInterop_Font.h
		shaders/ShaderInterop_GPUSortLib.h
		shaders/ShaderInterop_Image.h
		shaders/circlePS.hlsl
		shaders/cubeShadowGS_alphatest_emulation.hlsl
		shaders/emittedparticlePS_simple.hlsl
		shaders/envMapGS_emulation.hlsl
		shaders/fft_512x512_c2c_CS.hlsl
		shaders/fft_512x512_c2c_v2_CS.hlsl
		shaders/fontPS.hlsl
		shaders/fontVS.hlsl
		shaders/fsr_sharpenCS.hlsl
		shaders/fsr_upscalingCS.hlsl
		shaders/generateMIPChain2DCS_float4.hlsl
		shaders/gpusortlib_kickoffSortCS.hlsl
		shaders/gpusortlib_sortCS.hlsl
		shaders/gpusortlib_sortInnerCS.hlsl
		shaders/gpusortlib_sortStepCS.hlsl
		shaders/hairparticlePS_prepass.hlsl
		shaders/imageHF.hlsli
		shaders/imagePS.hlsl
		shaders/imageVS.hlsl
		shaders/impostorHF.hlsli
		shaders/impostorPS_wire.hlsl
		shaders/msaoCS.hlsl
		shaders/objectGS_voxelizer.hlsl
		shaders/occludeeVS.hlsl
		shaders/oceanUpdateDisplacementMapCS.hlsl
		shaders/postprocessVS.hlsl
		shaders/shadowPS_transparent.hlsl
		shaders/voxelPS.hlsl
		shaders/ShaderInterop_DDGI.h
		shaders/ShaderInterop_EmittedParticle.h
		shaders/ShaderInterop_SurfelGI.h
		shaders/ddgi_debugPS.hlsl
		shaders/ddgi_debugVS.hlsl
		shaders/ddgi_raytraceCS_rtapi.hlsl
		shaders/ddgi_updateCS.hlsl
		shaders/ddgi_updateCS_depth.hlsl
		shaders/emittedparticlePS_soft.hlsl
		shaders/emittedparticle_sphforceCS.hlsl
		shaders/hairparticlePS.hlsl
		shaders/objectVS_simple.hlsl
		shaders/oceanSurfacePS.hlsl
		shaders/surfel_indirectprepareCS.hlsl
		shaders/surfel_integrateCS.hlsl
		shaders/ShaderInterop_Postprocess.h
		shaders/ShaderInterop_Renderer.h
		shaders/Shaders_SOURCE.vcxitems
		shaders/Shaders_SOURCE.vcxitems.filters
		shaders/brdf.hlsli
		shaders/bvh_primitivesCS.hlsl
		shaders/ddgi_raytraceCS.hlsl
		shaders/emittedparticleHF.hlsli
		shaders/emittedparticleMS.hlsl
		shaders/emittedparticleVS.hlsl
		shaders/emittedparticle_emitCS.hlsl
		shaders/emittedparticle_simulateCS.hlsl
		shaders/envMapPS.hlsl
		shaders/envMapVS.hlsl
		shaders/globals.hlsli
		shaders/hairparticleHF.hlsli
		shaders/hairparticleVS.hlsl
		shaders/hairparticle_simulateCS.hlsl
		shaders/impostorVS.hlsl
		shaders/lightingHF.hlsli
		shaders/motionblurCS.hlsl
		shaders/motionblur_tileMaxVelocity_horizontalCS.hlsl
		shaders/objectHF.hlsli
		shaders/objectPS_paintradius.hlsl
		shaders/objectPS_voxelizer.hlsl
		shaders/raytraceCS.hlsl
		shaders/raytracingHF.hlsli
		shaders/renderlightmapPS.hlsl
		shaders/rtaoCS.hlsl
		shaders/rtao_denoise_tileclassificationCS.hlsl
		shaders/rtreflectionCS.hlsl
		shaders/rtreflectionLIB.hlsl
		shaders/rtshadow_denoise_temporalCS.hlsl
		shaders/rtshadow_denoise_tileclassificationCS.hlsl
		shaders/screenspaceshadowCS.hlsl
		shaders/shadingRateClassificationCS.hlsl
		shaders/ssaoCS.hlsl
		shaders/ssr_bilateralCS.hlsl
		shaders/ssr_depthHierarchyCS.hlsl
		shaders/ssr_kickjobsCS.hlsl
		shaders/ssr_raytraceCS.hlsl
		shaders/ssr_raytraceCS_cheap.hlsl
		shaders/ssr_raytraceCS_earlyexit.hlsl
		shaders/ssr_resolveCS.hlsl
		shaders/ssr_temporalCS.hlsl
		shaders/ssr_tileMaxRoughness_horizontalCS.hlsl
		shaders/ssr_tileMaxRoughness_verticalCS.hlsl
		shaders/stochasticSSRHF.hlsli
		shaders/surfel_coverageCS.hlsl
		shaders/surfel_raytraceCS.hlsl
		shaders/surfel_updateCS.hlsl
		shaders/temporalaaCS.hlsl
		shaders/visibility_resolveCS.hlsl
		shaders/visibility_resolveCS_fast.hlsl
		shaders/visibility_resolveCS_fast_MSAA.hlsl

		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/shaders/")

install(FILES
		shaders/ffx-shadows-dnsr/ffx_denoiser_shadows_filter.h
		shaders/ffx-shadows-dnsr/ffx_denoiser_shadows_prepare.h
		shaders/ffx-shadows-dnsr/ffx_denoiser_shadows_tileclassification.h
		shaders/ffx-shadows-dnsr/ffx_denoiser_shadows_util.h
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/shaders/ffx-shadows-dnsr")
install(FILES
		shaders/ffx-fsr/ffx_a.h
		shaders/ffx-fsr/ffx_fsr1.h
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/shaders/ffx-fsr")