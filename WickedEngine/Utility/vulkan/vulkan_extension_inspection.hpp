// Copyright 2015-2023 The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT
//

// This header is generated from the Khronos Vulkan XML API Registry.

#ifndef VULKAN_EXTENSION_INSPECTION_HPP
#define VULKAN_EXTENSION_INSPECTION_HPP

#include <vulkan/vulkan.hpp>

namespace VULKAN_HPP_NAMESPACE
{
  //======================================
  //=== Extension inspection functions ===
  //======================================

  VULKAN_HPP_CONSTEXPR_20 std::string getExtensionDeprecatedBy( std::string const & name );
  VULKAN_HPP_CONSTEXPR_20 bool        isDeviceExtension( std::string const & name );
  VULKAN_HPP_CONSTEXPR_20 bool        isExtensionDeprecated( std::string const & name );
  VULKAN_HPP_CONSTEXPR_20 bool        isInstanceExtension( std::string const & name );

  //=====================================================
  //=== Extension inspection function implementations ===
  //=====================================================

  VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR_20 std::string getExtensionDeprecatedBy( std::string const & name )
  {
    if ( name == "VK_EXT_debug_report" )
    {
      return "VK_EXT_debug_utils";
    }
    if ( name == "VK_NV_glsl_shader" )
    {
      return "";
    }
    if ( name == "VK_NV_dedicated_allocation" )
    {
      return "VK_KHR_dedicated_allocation";
    }
    if ( name == "VK_AMD_gpu_shader_half_float" )
    {
      return "VK_KHR_shader_float16_int8";
    }
    if ( name == "VK_IMG_format_pvrtc" )
    {
      return "";
    }
    if ( name == "VK_NV_external_memory_capabilities" )
    {
      return "VK_KHR_external_memory_capabilities";
    }
    if ( name == "VK_NV_external_memory" )
    {
      return "VK_KHR_external_memory";
    }
#if defined( VK_USE_PLATFORM_WIN32_KHR )
    if ( name == "VK_NV_external_memory_win32" )
    {
      return "VK_KHR_external_memory_win32";
    }
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
    if ( name == "VK_EXT_validation_flags" )
    {
      return "VK_EXT_validation_features";
    }
    if ( name == "VK_EXT_shader_subgroup_ballot" )
    {
      return "VK_VERSION_1_2";
    }
    if ( name == "VK_EXT_shader_subgroup_vote" )
    {
      return "VK_VERSION_1_1";
    }
#if defined( VK_USE_PLATFORM_IOS_MVK )
    if ( name == "VK_MVK_ios_surface" )
    {
      return "VK_EXT_metal_surface";
    }
#endif /*VK_USE_PLATFORM_IOS_MVK*/
#if defined( VK_USE_PLATFORM_MACOS_MVK )
    if ( name == "VK_MVK_macos_surface" )
    {
      return "VK_EXT_metal_surface";
    }
#endif /*VK_USE_PLATFORM_MACOS_MVK*/
    if ( name == "VK_AMD_gpu_shader_int16" )
    {
      return "VK_KHR_shader_float16_int8";
    }
    if ( name == "VK_EXT_buffer_device_address" )
    {
      return "VK_KHR_buffer_device_address";
    }
    return "";
  }

  VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR_20 bool isDeviceExtension( std::string const & name )
  {
    return ( name == "VK_KHR_swapchain" ) || ( name == "VK_KHR_display_swapchain" ) || ( name == "VK_NV_glsl_shader" ) ||
           ( name == "VK_EXT_depth_range_unrestricted" ) || ( name == "VK_KHR_sampler_mirror_clamp_to_edge" ) || ( name == "VK_IMG_filter_cubic" ) ||
           ( name == "VK_AMD_rasterization_order" ) || ( name == "VK_AMD_shader_trinary_minmax" ) || ( name == "VK_AMD_shader_explicit_vertex_parameter" ) ||
           ( name == "VK_EXT_debug_marker" ) || ( name == "VK_KHR_video_queue" ) || ( name == "VK_KHR_video_decode_queue" ) ||
           ( name == "VK_AMD_gcn_shader" ) || ( name == "VK_NV_dedicated_allocation" ) || ( name == "VK_EXT_transform_feedback" ) ||
           ( name == "VK_NVX_binary_import" ) || ( name == "VK_NVX_image_view_handle" ) || ( name == "VK_AMD_draw_indirect_count" ) ||
           ( name == "VK_AMD_negative_viewport_height" ) || ( name == "VK_AMD_gpu_shader_half_float" ) || ( name == "VK_AMD_shader_ballot" ) ||
#if defined( VK_ENABLE_BETA_EXTENSIONS )
           ( name == "VK_EXT_video_encode_h264" ) || ( name == "VK_EXT_video_encode_h265" ) ||
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
           ( name == "VK_KHR_video_decode_h264" ) || ( name == "VK_AMD_texture_gather_bias_lod" ) || ( name == "VK_AMD_shader_info" ) ||
           ( name == "VK_KHR_dynamic_rendering" ) || ( name == "VK_AMD_shader_image_load_store_lod" ) || ( name == "VK_NV_corner_sampled_image" ) ||
           ( name == "VK_KHR_multiview" ) || ( name == "VK_IMG_format_pvrtc" ) || ( name == "VK_NV_external_memory" ) ||
#if defined( VK_USE_PLATFORM_WIN32_KHR )
           ( name == "VK_NV_external_memory_win32" ) || ( name == "VK_NV_win32_keyed_mutex" ) ||
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
           ( name == "VK_KHR_device_group" ) || ( name == "VK_KHR_shader_draw_parameters" ) || ( name == "VK_EXT_shader_subgroup_ballot" ) ||
           ( name == "VK_EXT_shader_subgroup_vote" ) || ( name == "VK_EXT_texture_compression_astc_hdr" ) || ( name == "VK_EXT_astc_decode_mode" ) ||
           ( name == "VK_EXT_pipeline_robustness" ) || ( name == "VK_KHR_maintenance1" ) || ( name == "VK_KHR_external_memory" ) ||
#if defined( VK_USE_PLATFORM_WIN32_KHR )
           ( name == "VK_KHR_external_memory_win32" ) ||
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
           ( name == "VK_KHR_external_memory_fd" ) ||
#if defined( VK_USE_PLATFORM_WIN32_KHR )
           ( name == "VK_KHR_win32_keyed_mutex" ) ||
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
           ( name == "VK_KHR_external_semaphore" ) ||
#if defined( VK_USE_PLATFORM_WIN32_KHR )
           ( name == "VK_KHR_external_semaphore_win32" ) ||
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
           ( name == "VK_KHR_external_semaphore_fd" ) || ( name == "VK_KHR_push_descriptor" ) || ( name == "VK_EXT_conditional_rendering" ) ||
           ( name == "VK_KHR_shader_float16_int8" ) || ( name == "VK_KHR_16bit_storage" ) || ( name == "VK_KHR_incremental_present" ) ||
           ( name == "VK_KHR_descriptor_update_template" ) || ( name == "VK_NV_clip_space_w_scaling" ) || ( name == "VK_EXT_display_control" ) ||
           ( name == "VK_GOOGLE_display_timing" ) || ( name == "VK_NV_sample_mask_override_coverage" ) || ( name == "VK_NV_geometry_shader_passthrough" ) ||
           ( name == "VK_NV_viewport_array2" ) || ( name == "VK_NVX_multiview_per_view_attributes" ) || ( name == "VK_NV_viewport_swizzle" ) ||
           ( name == "VK_EXT_discard_rectangles" ) || ( name == "VK_EXT_conservative_rasterization" ) || ( name == "VK_EXT_depth_clip_enable" ) ||
           ( name == "VK_EXT_hdr_metadata" ) || ( name == "VK_KHR_imageless_framebuffer" ) || ( name == "VK_KHR_create_renderpass2" ) ||
           ( name == "VK_KHR_shared_presentable_image" ) || ( name == "VK_KHR_external_fence" ) ||
#if defined( VK_USE_PLATFORM_WIN32_KHR )
           ( name == "VK_KHR_external_fence_win32" ) ||
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
           ( name == "VK_KHR_external_fence_fd" ) || ( name == "VK_KHR_performance_query" ) || ( name == "VK_KHR_maintenance2" ) ||
           ( name == "VK_KHR_variable_pointers" ) || ( name == "VK_EXT_external_memory_dma_buf" ) || ( name == "VK_EXT_queue_family_foreign" ) ||
           ( name == "VK_KHR_dedicated_allocation" ) ||
#if defined( VK_USE_PLATFORM_ANDROID_KHR )
           ( name == "VK_ANDROID_external_memory_android_hardware_buffer" ) ||
#endif /*VK_USE_PLATFORM_ANDROID_KHR*/
           ( name == "VK_EXT_sampler_filter_minmax" ) || ( name == "VK_KHR_storage_buffer_storage_class" ) || ( name == "VK_AMD_gpu_shader_int16" ) ||
           ( name == "VK_AMD_mixed_attachment_samples" ) || ( name == "VK_AMD_shader_fragment_mask" ) || ( name == "VK_EXT_inline_uniform_block" ) ||
           ( name == "VK_EXT_shader_stencil_export" ) || ( name == "VK_EXT_sample_locations" ) || ( name == "VK_KHR_relaxed_block_layout" ) ||
           ( name == "VK_KHR_get_memory_requirements2" ) || ( name == "VK_KHR_image_format_list" ) || ( name == "VK_EXT_blend_operation_advanced" ) ||
           ( name == "VK_NV_fragment_coverage_to_color" ) || ( name == "VK_KHR_acceleration_structure" ) || ( name == "VK_KHR_ray_tracing_pipeline" ) ||
           ( name == "VK_KHR_ray_query" ) || ( name == "VK_NV_framebuffer_mixed_samples" ) || ( name == "VK_NV_fill_rectangle" ) ||
           ( name == "VK_NV_shader_sm_builtins" ) || ( name == "VK_EXT_post_depth_coverage" ) || ( name == "VK_KHR_sampler_ycbcr_conversion" ) ||
           ( name == "VK_KHR_bind_memory2" ) || ( name == "VK_EXT_image_drm_format_modifier" ) || ( name == "VK_EXT_validation_cache" ) ||
           ( name == "VK_EXT_descriptor_indexing" ) || ( name == "VK_EXT_shader_viewport_index_layer" ) ||
#if defined( VK_ENABLE_BETA_EXTENSIONS )
           ( name == "VK_KHR_portability_subset" ) ||
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
           ( name == "VK_NV_shading_rate_image" ) || ( name == "VK_NV_ray_tracing" ) || ( name == "VK_NV_representative_fragment_test" ) ||
           ( name == "VK_KHR_maintenance3" ) || ( name == "VK_KHR_draw_indirect_count" ) || ( name == "VK_EXT_filter_cubic" ) ||
           ( name == "VK_QCOM_render_pass_shader_resolve" ) || ( name == "VK_EXT_global_priority" ) || ( name == "VK_KHR_shader_subgroup_extended_types" ) ||
           ( name == "VK_KHR_8bit_storage" ) || ( name == "VK_EXT_external_memory_host" ) || ( name == "VK_AMD_buffer_marker" ) ||
           ( name == "VK_KHR_shader_atomic_int64" ) || ( name == "VK_KHR_shader_clock" ) || ( name == "VK_AMD_pipeline_compiler_control" ) ||
           ( name == "VK_EXT_calibrated_timestamps" ) || ( name == "VK_AMD_shader_core_properties" ) || ( name == "VK_KHR_video_decode_h265" ) ||
           ( name == "VK_KHR_global_priority" ) || ( name == "VK_AMD_memory_overallocation_behavior" ) || ( name == "VK_EXT_vertex_attribute_divisor" ) ||
#if defined( VK_USE_PLATFORM_GGP )
           ( name == "VK_GGP_frame_token" ) ||
#endif /*VK_USE_PLATFORM_GGP*/
           ( name == "VK_EXT_pipeline_creation_feedback" ) || ( name == "VK_KHR_driver_properties" ) || ( name == "VK_KHR_shader_float_controls" ) ||
           ( name == "VK_NV_shader_subgroup_partitioned" ) || ( name == "VK_KHR_depth_stencil_resolve" ) || ( name == "VK_KHR_swapchain_mutable_format" ) ||
           ( name == "VK_NV_compute_shader_derivatives" ) || ( name == "VK_NV_mesh_shader" ) || ( name == "VK_NV_fragment_shader_barycentric" ) ||
           ( name == "VK_NV_shader_image_footprint" ) || ( name == "VK_NV_scissor_exclusive" ) || ( name == "VK_NV_device_diagnostic_checkpoints" ) ||
           ( name == "VK_KHR_timeline_semaphore" ) || ( name == "VK_INTEL_shader_integer_functions2" ) || ( name == "VK_INTEL_performance_query" ) ||
           ( name == "VK_KHR_vulkan_memory_model" ) || ( name == "VK_EXT_pci_bus_info" ) || ( name == "VK_AMD_display_native_hdr" ) ||
           ( name == "VK_KHR_shader_terminate_invocation" ) || ( name == "VK_EXT_fragment_density_map" ) || ( name == "VK_EXT_scalar_block_layout" ) ||
           ( name == "VK_GOOGLE_hlsl_functionality1" ) || ( name == "VK_GOOGLE_decorate_string" ) || ( name == "VK_EXT_subgroup_size_control" ) ||
           ( name == "VK_KHR_fragment_shading_rate" ) || ( name == "VK_AMD_shader_core_properties2" ) || ( name == "VK_AMD_device_coherent_memory" ) ||
           ( name == "VK_EXT_shader_image_atomic_int64" ) || ( name == "VK_KHR_spirv_1_4" ) || ( name == "VK_EXT_memory_budget" ) ||
           ( name == "VK_EXT_memory_priority" ) || ( name == "VK_NV_dedicated_allocation_image_aliasing" ) ||
           ( name == "VK_KHR_separate_depth_stencil_layouts" ) || ( name == "VK_EXT_buffer_device_address" ) || ( name == "VK_EXT_tooling_info" ) ||
           ( name == "VK_EXT_separate_stencil_usage" ) || ( name == "VK_KHR_present_wait" ) || ( name == "VK_NV_cooperative_matrix" ) ||
           ( name == "VK_NV_coverage_reduction_mode" ) || ( name == "VK_EXT_fragment_shader_interlock" ) || ( name == "VK_EXT_ycbcr_image_arrays" ) ||
           ( name == "VK_KHR_uniform_buffer_standard_layout" ) || ( name == "VK_EXT_provoking_vertex" ) ||
#if defined( VK_USE_PLATFORM_WIN32_KHR )
           ( name == "VK_EXT_full_screen_exclusive" ) ||
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
           ( name == "VK_KHR_buffer_device_address" ) || ( name == "VK_EXT_line_rasterization" ) || ( name == "VK_EXT_shader_atomic_float" ) ||
           ( name == "VK_EXT_host_query_reset" ) || ( name == "VK_EXT_index_type_uint8" ) || ( name == "VK_EXT_extended_dynamic_state" ) ||
           ( name == "VK_KHR_deferred_host_operations" ) || ( name == "VK_KHR_pipeline_executable_properties" ) || ( name == "VK_KHR_map_memory2" ) ||
           ( name == "VK_EXT_shader_atomic_float2" ) || ( name == "VK_EXT_swapchain_maintenance1" ) ||
           ( name == "VK_EXT_shader_demote_to_helper_invocation" ) || ( name == "VK_NV_device_generated_commands" ) ||
           ( name == "VK_NV_inherited_viewport_scissor" ) || ( name == "VK_KHR_shader_integer_dot_product" ) || ( name == "VK_EXT_texel_buffer_alignment" ) ||
           ( name == "VK_QCOM_render_pass_transform" ) || ( name == "VK_EXT_device_memory_report" ) || ( name == "VK_EXT_robustness2" ) ||
           ( name == "VK_EXT_custom_border_color" ) || ( name == "VK_GOOGLE_user_type" ) || ( name == "VK_KHR_pipeline_library" ) ||
           ( name == "VK_NV_present_barrier" ) || ( name == "VK_KHR_shader_non_semantic_info" ) || ( name == "VK_KHR_present_id" ) ||
           ( name == "VK_EXT_private_data" ) || ( name == "VK_EXT_pipeline_creation_cache_control" ) ||
#if defined( VK_ENABLE_BETA_EXTENSIONS )
           ( name == "VK_KHR_video_encode_queue" ) ||
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
           ( name == "VK_NV_device_diagnostics_config" ) || ( name == "VK_QCOM_render_pass_store_ops" ) || ( name == "VK_NV_low_latency" ) ||
#if defined( VK_USE_PLATFORM_METAL_EXT )
           ( name == "VK_EXT_metal_objects" ) ||
#endif /*VK_USE_PLATFORM_METAL_EXT*/
           ( name == "VK_KHR_synchronization2" ) || ( name == "VK_EXT_descriptor_buffer" ) || ( name == "VK_EXT_graphics_pipeline_library" ) ||
           ( name == "VK_AMD_shader_early_and_late_fragment_tests" ) || ( name == "VK_KHR_fragment_shader_barycentric" ) ||
           ( name == "VK_KHR_shader_subgroup_uniform_control_flow" ) || ( name == "VK_KHR_zero_initialize_workgroup_memory" ) ||
           ( name == "VK_NV_fragment_shading_rate_enums" ) || ( name == "VK_NV_ray_tracing_motion_blur" ) || ( name == "VK_EXT_mesh_shader" ) ||
           ( name == "VK_EXT_ycbcr_2plane_444_formats" ) || ( name == "VK_EXT_fragment_density_map2" ) || ( name == "VK_QCOM_rotated_copy_commands" ) ||
           ( name == "VK_EXT_image_robustness" ) || ( name == "VK_KHR_workgroup_memory_explicit_layout" ) || ( name == "VK_KHR_copy_commands2" ) ||
           ( name == "VK_EXT_image_compression_control" ) || ( name == "VK_EXT_attachment_feedback_loop_layout" ) || ( name == "VK_EXT_4444_formats" ) ||
           ( name == "VK_EXT_device_fault" ) || ( name == "VK_ARM_rasterization_order_attachment_access" ) || ( name == "VK_EXT_rgba10x6_formats" ) ||
#if defined( VK_USE_PLATFORM_WIN32_KHR )
           ( name == "VK_NV_acquire_winrt_display" ) ||
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
           ( name == "VK_VALVE_mutable_descriptor_type" ) || ( name == "VK_EXT_vertex_input_dynamic_state" ) || ( name == "VK_EXT_physical_device_drm" ) ||
           ( name == "VK_EXT_device_address_binding_report" ) || ( name == "VK_EXT_depth_clip_control" ) ||
           ( name == "VK_EXT_primitive_topology_list_restart" ) || ( name == "VK_KHR_format_feature_flags2" ) ||
#if defined( VK_USE_PLATFORM_FUCHSIA )
           ( name == "VK_FUCHSIA_external_memory" ) || ( name == "VK_FUCHSIA_external_semaphore" ) || ( name == "VK_FUCHSIA_buffer_collection" ) ||
#endif /*VK_USE_PLATFORM_FUCHSIA*/
           ( name == "VK_HUAWEI_subpass_shading" ) || ( name == "VK_HUAWEI_invocation_mask" ) || ( name == "VK_NV_external_memory_rdma" ) ||
           ( name == "VK_EXT_pipeline_properties" ) || ( name == "VK_EXT_multisampled_render_to_single_sampled" ) ||
           ( name == "VK_EXT_extended_dynamic_state2" ) || ( name == "VK_EXT_color_write_enable" ) || ( name == "VK_EXT_primitives_generated_query" ) ||
           ( name == "VK_KHR_ray_tracing_maintenance1" ) || ( name == "VK_EXT_global_priority_query" ) || ( name == "VK_EXT_image_view_min_lod" ) ||
           ( name == "VK_EXT_multi_draw" ) || ( name == "VK_EXT_image_2d_view_of_3d" ) || ( name == "VK_EXT_shader_tile_image" ) ||
           ( name == "VK_EXT_opacity_micromap" ) ||
#if defined( VK_ENABLE_BETA_EXTENSIONS )
           ( name == "VK_NV_displacement_micromap" ) ||
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
           ( name == "VK_EXT_load_store_op_none" ) || ( name == "VK_HUAWEI_cluster_culling_shader" ) || ( name == "VK_EXT_border_color_swizzle" ) ||
           ( name == "VK_EXT_pageable_device_local_memory" ) || ( name == "VK_KHR_maintenance4" ) || ( name == "VK_ARM_shader_core_properties" ) ||
           ( name == "VK_EXT_image_sliced_view_of_3d" ) || ( name == "VK_VALVE_descriptor_set_host_mapping" ) || ( name == "VK_EXT_depth_clamp_zero_one" ) ||
           ( name == "VK_EXT_non_seamless_cube_map" ) || ( name == "VK_QCOM_fragment_density_map_offset" ) || ( name == "VK_NV_copy_memory_indirect" ) ||
           ( name == "VK_NV_memory_decompression" ) || ( name == "VK_NV_linear_color_attachment" ) ||
           ( name == "VK_EXT_image_compression_control_swapchain" ) || ( name == "VK_QCOM_image_processing" ) || ( name == "VK_EXT_extended_dynamic_state3" ) ||
           ( name == "VK_EXT_subpass_merge_feedback" ) || ( name == "VK_EXT_shader_module_identifier" ) ||
           ( name == "VK_EXT_rasterization_order_attachment_access" ) || ( name == "VK_NV_optical_flow" ) || ( name == "VK_EXT_legacy_dithering" ) ||
           ( name == "VK_EXT_pipeline_protected_access" ) || ( name == "VK_EXT_shader_object" ) || ( name == "VK_QCOM_tile_properties" ) ||
           ( name == "VK_SEC_amigo_profiling" ) || ( name == "VK_QCOM_multiview_per_view_viewports" ) || ( name == "VK_NV_ray_tracing_invocation_reorder" ) ||
           ( name == "VK_EXT_mutable_descriptor_type" ) || ( name == "VK_ARM_shader_core_builtins" ) || ( name == "VK_EXT_pipeline_library_group_handles" ) ||
           ( name == "VK_QCOM_multiview_per_view_render_areas" );
  }

  VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR_20 bool isExtensionDeprecated( std::string const & name )
  {
    return ( name == "VK_EXT_debug_report" ) || ( name == "VK_NV_glsl_shader" ) || ( name == "VK_NV_dedicated_allocation" ) ||
           ( name == "VK_AMD_gpu_shader_half_float" ) || ( name == "VK_IMG_format_pvrtc" ) || ( name == "VK_NV_external_memory_capabilities" ) ||
           ( name == "VK_NV_external_memory" ) ||
#if defined( VK_USE_PLATFORM_WIN32_KHR )
           ( name == "VK_NV_external_memory_win32" ) ||
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
           ( name == "VK_EXT_validation_flags" ) || ( name == "VK_EXT_shader_subgroup_ballot" ) || ( name == "VK_EXT_shader_subgroup_vote" ) ||
#if defined( VK_USE_PLATFORM_IOS_MVK )
           ( name == "VK_MVK_ios_surface" ) ||
#endif /*VK_USE_PLATFORM_IOS_MVK*/
#if defined( VK_USE_PLATFORM_MACOS_MVK )
           ( name == "VK_MVK_macos_surface" ) ||
#endif /*VK_USE_PLATFORM_MACOS_MVK*/
           ( name == "VK_AMD_gpu_shader_int16" ) || ( name == "VK_EXT_buffer_device_address" );
  }

  VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR_20 bool isInstanceExtension( std::string const & name )
  {
    return ( name == "VK_KHR_surface" ) || ( name == "VK_KHR_display" ) ||
#if defined( VK_USE_PLATFORM_XLIB_KHR )
           ( name == "VK_KHR_xlib_surface" ) ||
#endif /*VK_USE_PLATFORM_XLIB_KHR*/
#if defined( VK_USE_PLATFORM_XCB_KHR )
           ( name == "VK_KHR_xcb_surface" ) ||
#endif /*VK_USE_PLATFORM_XCB_KHR*/
#if defined( VK_USE_PLATFORM_WAYLAND_KHR )
           ( name == "VK_KHR_wayland_surface" ) ||
#endif /*VK_USE_PLATFORM_WAYLAND_KHR*/
#if defined( VK_USE_PLATFORM_ANDROID_KHR )
           ( name == "VK_KHR_android_surface" ) ||
#endif /*VK_USE_PLATFORM_ANDROID_KHR*/
#if defined( VK_USE_PLATFORM_WIN32_KHR )
           ( name == "VK_KHR_win32_surface" ) ||
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
           ( name == "VK_EXT_debug_report" ) ||
#if defined( VK_USE_PLATFORM_GGP )
           ( name == "VK_GGP_stream_descriptor_surface" ) ||
#endif /*VK_USE_PLATFORM_GGP*/
           ( name == "VK_NV_external_memory_capabilities" ) || ( name == "VK_KHR_get_physical_device_properties2" ) || ( name == "VK_EXT_validation_flags" ) ||
#if defined( VK_USE_PLATFORM_VI_NN )
           ( name == "VK_NN_vi_surface" ) ||
#endif /*VK_USE_PLATFORM_VI_NN*/
           ( name == "VK_KHR_device_group_creation" ) || ( name == "VK_KHR_external_memory_capabilities" ) ||
           ( name == "VK_KHR_external_semaphore_capabilities" ) || ( name == "VK_EXT_direct_mode_display" ) ||
#if defined( VK_USE_PLATFORM_XLIB_XRANDR_EXT )
           ( name == "VK_EXT_acquire_xlib_display" ) ||
#endif /*VK_USE_PLATFORM_XLIB_XRANDR_EXT*/
           ( name == "VK_EXT_display_surface_counter" ) || ( name == "VK_EXT_swapchain_colorspace" ) || ( name == "VK_KHR_external_fence_capabilities" ) ||
           ( name == "VK_KHR_get_surface_capabilities2" ) || ( name == "VK_KHR_get_display_properties2" ) ||
#if defined( VK_USE_PLATFORM_IOS_MVK )
           ( name == "VK_MVK_ios_surface" ) ||
#endif /*VK_USE_PLATFORM_IOS_MVK*/
#if defined( VK_USE_PLATFORM_MACOS_MVK )
           ( name == "VK_MVK_macos_surface" ) ||
#endif /*VK_USE_PLATFORM_MACOS_MVK*/
           ( name == "VK_EXT_debug_utils" ) ||
#if defined( VK_USE_PLATFORM_FUCHSIA )
           ( name == "VK_FUCHSIA_imagepipe_surface" ) ||
#endif /*VK_USE_PLATFORM_FUCHSIA*/
#if defined( VK_USE_PLATFORM_METAL_EXT )
           ( name == "VK_EXT_metal_surface" ) ||
#endif /*VK_USE_PLATFORM_METAL_EXT*/
           ( name == "VK_KHR_surface_protected_capabilities" ) || ( name == "VK_EXT_validation_features" ) || ( name == "VK_EXT_headless_surface" ) ||
           ( name == "VK_EXT_surface_maintenance1" ) || ( name == "VK_EXT_acquire_drm_display" ) ||
#if defined( VK_USE_PLATFORM_DIRECTFB_EXT )
           ( name == "VK_EXT_directfb_surface" ) ||
#endif /*VK_USE_PLATFORM_DIRECTFB_EXT*/
#if defined( VK_USE_PLATFORM_SCREEN_QNX )
           ( name == "VK_QNX_screen_surface" ) ||
#endif /*VK_USE_PLATFORM_SCREEN_QNX*/
           ( name == "VK_KHR_portability_enumeration" ) || ( name == "VK_GOOGLE_surfaceless_query" ) || ( name == "VK_LUNARG_direct_driver_loading" );
  }
}  // namespace VULKAN_HPP_NAMESPACE

#endif
