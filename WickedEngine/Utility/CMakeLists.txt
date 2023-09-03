include(GNUInstallDirs)
if (PLATFORM MATCHES "SDL2")
	add_subdirectory(FAudio)
endif()

set(HEADER_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/DirectXCollision.h
		${CMAKE_CURRENT_SOURCE_DIR}/DirectXColors.h
		${CMAKE_CURRENT_SOURCE_DIR}/DirectXMath.h
		${CMAKE_CURRENT_SOURCE_DIR}/DirectXMathCommon.h
		${CMAKE_CURRENT_SOURCE_DIR}/DirectXPackedVector.h
		${CMAKE_CURRENT_SOURCE_DIR}/DirectXCollision.inl
		${CMAKE_CURRENT_SOURCE_DIR}/DirectXMathConvert.inl
		${CMAKE_CURRENT_SOURCE_DIR}/DirectXMathMatrix.inl
		${CMAKE_CURRENT_SOURCE_DIR}/DirectXMathMisc.inl
		${CMAKE_CURRENT_SOURCE_DIR}/DirectXMathVector.inl
		${CMAKE_CURRENT_SOURCE_DIR}/DirectXPackedVector.inl
		${CMAKE_CURRENT_SOURCE_DIR}/liberation_sans.h
		${CMAKE_CURRENT_SOURCE_DIR}/portable-file-dialogs.h
		${CMAKE_CURRENT_SOURCE_DIR}/sal.h
		${CMAKE_CURRENT_SOURCE_DIR}/tinyddsloader.h
		${CMAKE_CURRENT_SOURCE_DIR}/robin_hood.h
		${CMAKE_CURRENT_SOURCE_DIR}/spirv_reflect.h
		${CMAKE_CURRENT_SOURCE_DIR}/stb_image.h
		${CMAKE_CURRENT_SOURCE_DIR}/stb_image_write.h
		${CMAKE_CURRENT_SOURCE_DIR}/stb_rect_pack.h
		${CMAKE_CURRENT_SOURCE_DIR}/stb_truetype.h
		${CMAKE_CURRENT_SOURCE_DIR}/qoi.h
		${CMAKE_CURRENT_SOURCE_DIR}/dxcapi.h
		${CMAKE_CURRENT_SOURCE_DIR}/D3D12MemAlloc.h
		${CMAKE_CURRENT_SOURCE_DIR}/volk.h
		${CMAKE_CURRENT_SOURCE_DIR}/vk_mem_alloc.h
		${CMAKE_CURRENT_SOURCE_DIR}/flat_hash_map.hpp
		${CMAKE_CURRENT_SOURCE_DIR}/pugixml.hpp
		${CMAKE_CURRENT_SOURCE_DIR}/pugiconfig.hpp
		${CMAKE_CURRENT_SOURCE_DIR}/h264.h
		${CMAKE_CURRENT_SOURCE_DIR}/dds_write.h
		${CMAKE_CURRENT_SOURCE_DIR}/lodepng.h
		)
install(FILES ${HEADER_FILES}
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/Utility/")

set(HEADER_FILES_dx12
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/d3d12.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/d3d12compatibility.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/d3d12sdklayers.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/d3dcommon.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/dxgicommon.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/dxgiformat.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/d3d12shader.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/d3d12video.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/d3dx12.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/d3dx12_barriers.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/d3dx12_check_feature_support.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/d3dx12_core.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/d3dx12_default.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/d3dx12_pipeline_state_stream.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/d3dx12_render_pass.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/d3dx12_resource_helpers.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/d3dx12_root_signature.h
		${CMAKE_CURRENT_SOURCE_DIR}/dx12/d3dx12_state_object.h
		)
install(FILES ${HEADER_FILES_dx12}
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/Utility/dx12")

set(HEADER_FILES_spirv
		${CMAKE_CURRENT_SOURCE_DIR}/include/spirv/unified1/spirv.h
		)
install(FILES ${HEADER_FILES_spirv}
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/Utility/include/spirv/unified1/")

set(HEADER_FILES_vulkan
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vk_layer.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vk_sdk_platform.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_directfb.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_ggp.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_ios.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_macos.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_metal.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_screen.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_vi.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_wayland.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_win32.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_xcb.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_xlib.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_xlib_xrandr.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vk_icd.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vk_platform.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_android.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_beta.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_core.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_fuchsia.h
		)
install(FILES ${HEADER_FILES_vulkan}
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/Utility/vulkan/")

		
set(HEADER_FILES_vk_video
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vk_video/vulkan_video_codec_h264std.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vk_video/vulkan_video_codec_h264std_decode.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vk_video/vulkan_video_codec_h264std_encode.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vk_video/vulkan_video_codec_h265std.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vk_video/vulkan_video_codec_h265std_decode.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vk_video/vulkan_video_codec_h265std_encode.h
		${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vk_video/vulkan_video_codecs_common.h
		)
install(FILES ${HEADER_FILES_vulkan}
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/Utility/vulkan/vk_video/")


set(HEADER_FILES_dxc
		${CMAKE_CURRENT_SOURCE_DIR}/dxc/Support/WinAdapter.h
		)
install(FILES ${HEADER_FILES_dxc}
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/Utility/dxc/Support/")


set (SOURCE_FILES
	utility_common.cpp
	spirv_reflect.c
	stb_vorbis.c
	samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp.cpp
	pugixml.cpp
	lodepng.cpp
)

if (WIN32)
	list (APPEND SOURCE_FILES
		D3D12MemAlloc.cpp
	)
endif ()

add_library(Utility STATIC
		${SOURCE_FILES}
		${HEADER_FILES}
		${HEADER_FILES_dx12}
		${HEADER_FILES_dxc}
		${HEADER_FILES_encoder}
		${HEADER_FILES_transcoder}
		${HEADER_FILES_spirv}
		${HEADER_FILES_vulkan}
		${HEADER_FILES_zstd}
		)

set_target_properties("Utility" PROPERTIES
	FOLDER "ThirdParty"
	POSITION_INDEPENDENT_CODE ${WICKED_DYNAMIC_LIBRARY}
)
