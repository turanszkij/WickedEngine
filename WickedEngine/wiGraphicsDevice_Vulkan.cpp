#include "wiGraphicsDevice_Vulkan.h"

#ifdef WICKEDENGINE_BUILD_VULKAN
#include "wiHelper.h"
#include "wiBacklog.h"
#include "wiVersion.h"
#include "wiTimer.h"
#include "wiUnorderedSet.h"

#include "Utility/vulkan/vk_video/vulkan_video_codec_h264std_decode.h"
#include "Utility/h264.h"

#define VOLK_IMPLEMENTATION
#include "Utility/volk.h"

#include "Utility/spirv_reflect.h"

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "Utility/vk_mem_alloc.h"

#ifdef SDL2
#include <SDL2/SDL_vulkan.h>
#include "sdl2.h"
#endif

#include <string>
#include <cstring>
#include <iostream>
#include <algorithm>

namespace wi::graphics
{

namespace vulkan_internal
{
	// These shifts are made so that Vulkan resource bindings slots don't interfere with each other across shader stages:
	//	These are also defined in wi::shadercompiler.cpp as hard coded compiler arguments for SPIRV, so they need to be the same
	enum
	{
		VULKAN_BINDING_SHIFT_B = 0,
		VULKAN_BINDING_SHIFT_T = 1000,
		VULKAN_BINDING_SHIFT_U = 2000,
		VULKAN_BINDING_SHIFT_S = 3000,
	};

	// Converters:
	constexpr VkFormat _ConvertFormat(Format value)
	{
		switch (value)
		{
		case Format::UNKNOWN:
			return VK_FORMAT_UNDEFINED;
		case Format::R32G32B32A32_FLOAT:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case Format::R32G32B32A32_UINT:
			return VK_FORMAT_R32G32B32A32_UINT;
		case Format::R32G32B32A32_SINT:
			return VK_FORMAT_R32G32B32A32_SINT;
		case Format::R32G32B32_FLOAT:
			return VK_FORMAT_R32G32B32_SFLOAT;
		case Format::R32G32B32_UINT:
			return VK_FORMAT_R32G32B32_UINT;
		case Format::R32G32B32_SINT:
			return VK_FORMAT_R32G32B32_SINT;
		case Format::R16G16B16A16_FLOAT:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Format::R16G16B16A16_UNORM:
			return VK_FORMAT_R16G16B16A16_UNORM;
		case Format::R16G16B16A16_UINT:
			return VK_FORMAT_R16G16B16A16_UINT;
		case Format::R16G16B16A16_SNORM:
			return VK_FORMAT_R16G16B16A16_SNORM;
		case Format::R16G16B16A16_SINT:
			return VK_FORMAT_R16G16B16A16_SINT;
		case Format::R32G32_FLOAT:
			return VK_FORMAT_R32G32_SFLOAT;
		case Format::R32G32_UINT:
			return VK_FORMAT_R32G32_UINT;
		case Format::R32G32_SINT:
			return VK_FORMAT_R32G32_SINT;
		case Format::D32_FLOAT_S8X24_UINT:
			return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case Format::R10G10B10A2_UNORM:
			return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
		case Format::R10G10B10A2_UINT:
			return VK_FORMAT_A2B10G10R10_UINT_PACK32;
		case Format::R11G11B10_FLOAT:
			return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case Format::R8G8B8A8_UNORM:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case Format::R8G8B8A8_UNORM_SRGB:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case Format::R8G8B8A8_UINT:
			return VK_FORMAT_R8G8B8A8_UINT;
		case Format::R8G8B8A8_SNORM:
			return VK_FORMAT_R8G8B8A8_SNORM;
		case Format::R8G8B8A8_SINT:
			return VK_FORMAT_R8G8B8A8_SINT;
		case Format::R16G16_FLOAT:
			return VK_FORMAT_R16G16_SFLOAT;
		case Format::R16G16_UNORM:
			return VK_FORMAT_R16G16_UNORM;
		case Format::R16G16_UINT:
			return VK_FORMAT_R16G16_UINT;
		case Format::R16G16_SNORM:
			return VK_FORMAT_R16G16_SNORM;
		case Format::R16G16_SINT:
			return VK_FORMAT_R16G16_SINT;
		case Format::D32_FLOAT:
			return VK_FORMAT_D32_SFLOAT;
		case Format::R32_FLOAT:
			return VK_FORMAT_R32_SFLOAT;
		case Format::R32_UINT:
			return VK_FORMAT_R32_UINT;
		case Format::R32_SINT:
			return VK_FORMAT_R32_SINT;
		case Format::D24_UNORM_S8_UINT:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		case Format::R9G9B9E5_SHAREDEXP:
			return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		case Format::R8G8_UNORM:
			return VK_FORMAT_R8G8_UNORM;
		case Format::R8G8_UINT:
			return VK_FORMAT_R8G8_UINT;
		case Format::R8G8_SNORM:
			return VK_FORMAT_R8G8_SNORM;
		case Format::R8G8_SINT:
			return VK_FORMAT_R8G8_SINT;
		case Format::R16_FLOAT:
			return VK_FORMAT_R16_SFLOAT;
		case Format::D16_UNORM:
			return VK_FORMAT_D16_UNORM;
		case Format::R16_UNORM:
			return VK_FORMAT_R16_UNORM;
		case Format::R16_UINT:
			return VK_FORMAT_R16_UINT;
		case Format::R16_SNORM:
			return VK_FORMAT_R16_SNORM;
		case Format::R16_SINT:
			return VK_FORMAT_R16_SINT;
		case Format::R8_UNORM:
			return VK_FORMAT_R8_UNORM;
		case Format::R8_UINT:
			return VK_FORMAT_R8_UINT;
		case Format::R8_SNORM:
			return VK_FORMAT_R8_SNORM;
		case Format::R8_SINT:
			return VK_FORMAT_R8_SINT;
		case Format::BC1_UNORM:
			return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case Format::BC1_UNORM_SRGB:
			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		case Format::BC2_UNORM:
			return VK_FORMAT_BC2_UNORM_BLOCK;
		case Format::BC2_UNORM_SRGB:
			return VK_FORMAT_BC2_SRGB_BLOCK;
		case Format::BC3_UNORM:
			return VK_FORMAT_BC3_UNORM_BLOCK;
		case Format::BC3_UNORM_SRGB:
			return VK_FORMAT_BC3_SRGB_BLOCK;
		case Format::BC4_UNORM:
			return VK_FORMAT_BC4_UNORM_BLOCK;
		case Format::BC4_SNORM:
			return VK_FORMAT_BC4_SNORM_BLOCK;
		case Format::BC5_UNORM:
			return VK_FORMAT_BC5_UNORM_BLOCK;
		case Format::BC5_SNORM:
			return VK_FORMAT_BC5_SNORM_BLOCK;
		case Format::B8G8R8A8_UNORM:
			return VK_FORMAT_B8G8R8A8_UNORM;
		case Format::B8G8R8A8_UNORM_SRGB:
			return VK_FORMAT_B8G8R8A8_SRGB;
		case Format::BC6H_UF16:
			return VK_FORMAT_BC6H_UFLOAT_BLOCK;
		case Format::BC6H_SF16:
			return VK_FORMAT_BC6H_SFLOAT_BLOCK;
		case Format::BC7_UNORM:
			return VK_FORMAT_BC7_UNORM_BLOCK;
		case Format::BC7_UNORM_SRGB:
			return VK_FORMAT_BC7_SRGB_BLOCK;
		case Format::NV12:
			return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
		}
		return VK_FORMAT_UNDEFINED;
	}
	constexpr VkCompareOp _ConvertComparisonFunc(ComparisonFunc value)
	{
		switch (value)
		{
		case ComparisonFunc::NEVER:
			return VK_COMPARE_OP_NEVER;
		case ComparisonFunc::LESS:
			return VK_COMPARE_OP_LESS;
		case ComparisonFunc::EQUAL:
			return VK_COMPARE_OP_EQUAL;
		case ComparisonFunc::LESS_EQUAL:
			return VK_COMPARE_OP_LESS_OR_EQUAL;
		case ComparisonFunc::GREATER:
			return VK_COMPARE_OP_GREATER;
		case ComparisonFunc::NOT_EQUAL:
			return VK_COMPARE_OP_NOT_EQUAL;
		case ComparisonFunc::GREATER_EQUAL:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case ComparisonFunc::ALWAYS:
			return VK_COMPARE_OP_ALWAYS;
		default:
			return VK_COMPARE_OP_NEVER;
		}
	}
	constexpr VkBlendFactor _ConvertBlend(Blend value)
	{
		switch (value)
		{
		case Blend::ZERO:
			return VK_BLEND_FACTOR_ZERO;
		case Blend::ONE:
			return VK_BLEND_FACTOR_ONE;
		case Blend::SRC_COLOR:
			return VK_BLEND_FACTOR_SRC_COLOR;
		case Blend::INV_SRC_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case Blend::SRC_ALPHA:
			return VK_BLEND_FACTOR_SRC_ALPHA;
		case Blend::INV_SRC_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case Blend::DEST_ALPHA:
			return VK_BLEND_FACTOR_DST_ALPHA;
		case Blend::INV_DEST_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case Blend::DEST_COLOR:
			return VK_BLEND_FACTOR_DST_COLOR;
		case Blend::INV_DEST_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case Blend::SRC_ALPHA_SAT:
			return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		case Blend::BLEND_FACTOR:
			return VK_BLEND_FACTOR_CONSTANT_COLOR;
		case Blend::INV_BLEND_FACTOR:
			return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
			break;
		case Blend::SRC1_COLOR:
			return VK_BLEND_FACTOR_SRC1_COLOR;
		case Blend::INV_SRC1_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
		case Blend::SRC1_ALPHA:
			return VK_BLEND_FACTOR_SRC1_ALPHA;
		case Blend::INV_SRC1_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
		default:
			return VK_BLEND_FACTOR_ZERO;
		}
	}
	constexpr VkBlendOp _ConvertBlendOp(BlendOp value)
	{
		switch (value)
		{
		case BlendOp::ADD:
			return VK_BLEND_OP_ADD;
		case BlendOp::SUBTRACT:
			return VK_BLEND_OP_SUBTRACT;
		case BlendOp::REV_SUBTRACT:
			return VK_BLEND_OP_REVERSE_SUBTRACT;
		case BlendOp::MIN:
			return VK_BLEND_OP_MIN;
		case BlendOp::MAX:
			return VK_BLEND_OP_MAX;
		default:
			return VK_BLEND_OP_ADD;
		}
	}
	constexpr VkSamplerAddressMode _ConvertTextureAddressMode(TextureAddressMode value, const VkPhysicalDeviceVulkan12Features& features_1_2)
	{
		switch (value)
		{
		case TextureAddressMode::WRAP:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case TextureAddressMode::MIRROR:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case TextureAddressMode::CLAMP:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case TextureAddressMode::BORDER:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case TextureAddressMode::MIRROR_ONCE:
			if (features_1_2.samplerMirrorClampToEdge == VK_TRUE)
			{
				return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
			}
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		default:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		}
	}
	constexpr VkBorderColor _ConvertSamplerBorderColor(SamplerBorderColor value)
	{
		switch (value)
		{
		case SamplerBorderColor::TRANSPARENT_BLACK:
			return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		case SamplerBorderColor::OPAQUE_BLACK:
			return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		case SamplerBorderColor::OPAQUE_WHITE:
			return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		default:
			return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		}
	}
	constexpr VkStencilOp _ConvertStencilOp(StencilOp value)
	{
		switch (value)
		{
		case wi::graphics::StencilOp::KEEP:
			return VK_STENCIL_OP_KEEP;
		case wi::graphics::StencilOp::ZERO:
			return VK_STENCIL_OP_ZERO;
		case wi::graphics::StencilOp::REPLACE:
			return VK_STENCIL_OP_REPLACE;
		case wi::graphics::StencilOp::INCR_SAT:
			return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case wi::graphics::StencilOp::DECR_SAT:
			return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case wi::graphics::StencilOp::INVERT:
			return VK_STENCIL_OP_INVERT;
		case wi::graphics::StencilOp::INCR:
			return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case wi::graphics::StencilOp::DECR:
			return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		default:
			return VK_STENCIL_OP_KEEP;
		}
	}
	constexpr VkImageLayout _ConvertImageLayout(ResourceState value)
	{
		switch (value)
		{
		case ResourceState::UNDEFINED:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		case ResourceState::RENDERTARGET:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case ResourceState::DEPTHSTENCIL:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case ResourceState::DEPTHSTENCIL_READONLY:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		case ResourceState::SHADER_RESOURCE:
		case ResourceState::SHADER_RESOURCE_COMPUTE:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case ResourceState::UNORDERED_ACCESS:
			return VK_IMAGE_LAYOUT_GENERAL;
		case ResourceState::COPY_SRC:
		case ResourceState::COPY_DST:
			// we can't assume transfer layout because it's allowed for resource to be used by multiple queues like DX12 (decay to common state), so this is a workaround
			//	the problem is that image copy commands will require specifying the current layout, but different queues can often use textures in different layouts
			return VK_IMAGE_LAYOUT_GENERAL;
		case ResourceState::SHADING_RATE_SOURCE:
			return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
		case ResourceState::VIDEO_DECODE_SRC:
		case ResourceState::VIDEO_DECODE_DST:
			return VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR;
		default:
			// combination of state flags will default to general
			//	whether the combination of states is valid needs to be validated by the user
			//	combining read-only states should be fine
			return VK_IMAGE_LAYOUT_GENERAL;
		}
	}
	constexpr VkShaderStageFlags _ConvertStageFlags(ShaderStage value)
	{
		switch (value)
		{
		case ShaderStage::MS:
			return VK_SHADER_STAGE_MESH_BIT_EXT;
		case ShaderStage::AS:
			return VK_SHADER_STAGE_TASK_BIT_EXT;
		case ShaderStage::VS:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderStage::HS:
			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case ShaderStage::DS:
			return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		case ShaderStage::GS:
			return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderStage::PS:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderStage::CS:
			return VK_SHADER_STAGE_COMPUTE_BIT;
		default:
			return VK_SHADER_STAGE_ALL;
		}
	}
	constexpr VkImageAspectFlags _ConvertImageAspect(ImageAspect value)
	{
		switch (value)
		{
		default:
		case wi::graphics::ImageAspect::COLOR:
			return VK_IMAGE_ASPECT_COLOR_BIT;
		case wi::graphics::ImageAspect::DEPTH:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		case wi::graphics::ImageAspect::STENCIL:
			return VK_IMAGE_ASPECT_STENCIL_BIT;
		case wi::graphics::ImageAspect::LUMINANCE:
			return VK_IMAGE_ASPECT_PLANE_0_BIT;
		case wi::graphics::ImageAspect::CHROMINANCE:
			return VK_IMAGE_ASPECT_PLANE_1_BIT;
		}
	}
	constexpr VkPipelineStageFlags2 _ConvertPipelineStage(ResourceState value)
	{
		VkPipelineStageFlags2 flags = VK_PIPELINE_STAGE_2_NONE;

		if (has_flag(value, ResourceState::SHADER_RESOURCE) ||
			has_flag(value, ResourceState::SHADER_RESOURCE_COMPUTE) ||
			has_flag(value, ResourceState::UNORDERED_ACCESS) ||
			has_flag(value, ResourceState::CONSTANT_BUFFER))
		{
			flags |= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		}
		if (has_flag(value, ResourceState::COPY_SRC) ||
			has_flag(value, ResourceState::COPY_DST))
		{
			flags |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		}
		if (has_flag(value, ResourceState::RENDERTARGET))
		{
			flags |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		if (has_flag(value, ResourceState::DEPTHSTENCIL) ||
			has_flag(value, ResourceState::DEPTHSTENCIL_READONLY))
		{
			flags |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		}
		if (has_flag(value, ResourceState::SHADING_RATE_SOURCE))
		{
			flags |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
		}
		if (has_flag(value, ResourceState::VERTEX_BUFFER))
		{
			flags |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
		}
		if (has_flag(value, ResourceState::INDEX_BUFFER))
		{
			flags |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
		}
		if (has_flag(value, ResourceState::INDIRECT_ARGUMENT))
		{
			flags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
		}
		if (has_flag(value, ResourceState::RAYTRACING_ACCELERATION_STRUCTURE))
		{
			flags |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
		}
		if (has_flag(value, ResourceState::PREDICATION))
		{
			flags |= VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT;
		}
		if (has_flag(value, ResourceState::VIDEO_DECODE_DST) ||
			has_flag(value, ResourceState::VIDEO_DECODE_SRC))
		{
			flags |= VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR;
		}

		return flags;
	}
	constexpr VkAccessFlags2 _ParseResourceState(ResourceState value)
	{
		VkAccessFlags2 flags = 0;

		if (has_flag(value, ResourceState::SHADER_RESOURCE))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT;
		}
		if (has_flag(value, ResourceState::SHADER_RESOURCE_COMPUTE))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT;
		}
		if (has_flag(value, ResourceState::UNORDERED_ACCESS))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT;
			flags |= VK_ACCESS_2_SHADER_WRITE_BIT;
		}
		if (has_flag(value, ResourceState::COPY_SRC))
		{
			flags |= VK_ACCESS_2_TRANSFER_READ_BIT;
		}
		if (has_flag(value, ResourceState::COPY_DST))
		{
			flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
		}
		if (has_flag(value, ResourceState::RENDERTARGET))
		{
			flags |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
			flags |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		}
		if (has_flag(value, ResourceState::DEPTHSTENCIL))
		{
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}
		if (has_flag(value, ResourceState::DEPTHSTENCIL_READONLY))
		{
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		}
		if (has_flag(value, ResourceState::VERTEX_BUFFER))
		{
			flags |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
		}
		if (has_flag(value, ResourceState::INDEX_BUFFER))
		{
			flags |= VK_ACCESS_2_INDEX_READ_BIT;
		}
		if (has_flag(value, ResourceState::CONSTANT_BUFFER))
		{
			flags |= VK_ACCESS_2_UNIFORM_READ_BIT;
		}
		if (has_flag(value, ResourceState::INDIRECT_ARGUMENT))
		{
			flags |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
		}
		if (has_flag(value, ResourceState::PREDICATION))
		{
			flags |= VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT;
		}
		if (has_flag(value, ResourceState::SHADING_RATE_SOURCE))
		{
			flags |= VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
		}
		if (has_flag(value, ResourceState::VIDEO_DECODE_DST))
		{
			flags |= VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
		}
		if (has_flag(value, ResourceState::VIDEO_DECODE_SRC))
		{
			flags |= VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR;
		}

		return flags;
	}
	constexpr VkComponentSwizzle _ConvertComponentSwizzle(ComponentSwizzle value)
	{
		switch (value)
		{
		default:
			return VK_COMPONENT_SWIZZLE_IDENTITY;
		case wi::graphics::ComponentSwizzle::R:
			return VK_COMPONENT_SWIZZLE_R;
		case wi::graphics::ComponentSwizzle::G:
			return VK_COMPONENT_SWIZZLE_G;
		case wi::graphics::ComponentSwizzle::B:
			return VK_COMPONENT_SWIZZLE_B;
		case wi::graphics::ComponentSwizzle::A:
			return VK_COMPONENT_SWIZZLE_A;
		case wi::graphics::ComponentSwizzle::ZERO:
			return VK_COMPONENT_SWIZZLE_ZERO;
		case wi::graphics::ComponentSwizzle::ONE:
			return VK_COMPONENT_SWIZZLE_ONE;
		}
	}
	constexpr VkComponentMapping _ConvertSwizzle(Swizzle value)
	{
		VkComponentMapping mapping = {};
		mapping.r = _ConvertComponentSwizzle(value.r);
		mapping.g = _ConvertComponentSwizzle(value.g);
		mapping.b = _ConvertComponentSwizzle(value.b);
		mapping.a = _ConvertComponentSwizzle(value.a);
		return mapping;
	}


	bool checkExtensionSupport(const char* checkExtension, const wi::vector<VkExtensionProperties>& available_extensions)
	{
		for (const auto& x : available_extensions)
		{
			if (strcmp(x.extensionName, checkExtension) == 0)
			{
				return true;
			}
		}
		return false;
	}

	bool ValidateLayers(const wi::vector<const char*>& required,
		const wi::vector<VkLayerProperties>& available)
	{
		for (auto layer : required)
		{
			bool found = false;
			for (auto& available_layer : available)
			{
				if (strcmp(available_layer.layerName, layer) == 0)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				return false;
			}
		}

		return true;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, 
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
		void* user_data)
	{
		// Log debug message
		std::string ss;

		if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			ss += "[Vulkan Warning]: ";
			ss += callback_data->pMessage;
			wi::backlog::post(ss, wi::backlog::LogLevel::Warning);
		}
		else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			ss += "[Vulkan Error]: ";
			ss += callback_data->pMessage;
#if 1
			wi::backlog::post(ss, wi::backlog::LogLevel::Error);
#else
			OutputDebugStringA(callback_data->pMessage);
			OutputDebugStringA("\n");
#endif
//#ifdef _DEBUG
//			assert(0);
//#endif // _DEBUG
		}

		return VK_FALSE;
	}


	struct BindingUsage
	{
		bool used = false;
		VkDescriptorSetLayoutBinding binding = {};
	};
	struct Buffer_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VmaAllocation allocation = nullptr;
		VkBuffer resource = VK_NULL_HANDLE;
		struct BufferSubresource
		{
			bool is_typed = false;
			VkBufferView buffer_view = VK_NULL_HANDLE;
			VkDescriptorBufferInfo buffer_info = {};
			int index = -1; // bindless

			constexpr bool IsValid() const
			{
				return index >= 0;
			}
		};
		BufferSubresource srv;
		BufferSubresource uav;
		wi::vector<BufferSubresource> subresources_srv;
		wi::vector<BufferSubresource> subresources_uav;
		VkDeviceAddress address = 0;

		void destroy_subresources()
		{
			uint64_t framecount = allocationhandler->framecount;
			if (srv.IsValid())
			{
				if (srv.is_typed)
				{
					allocationhandler->destroyer_bufferviews.push_back(std::make_pair(srv.buffer_view, framecount));
					allocationhandler->destroyer_bindlessUniformTexelBuffers.push_back(std::make_pair(srv.index, framecount));
				}
				else
				{
					allocationhandler->destroyer_bindlessStorageBuffers.push_back(std::make_pair(srv.index, framecount));
				}
				srv = {};
			}
			if (uav.IsValid())
			{
				if (uav.is_typed)
				{
					allocationhandler->destroyer_bufferviews.push_back(std::make_pair(uav.buffer_view, framecount));
					allocationhandler->destroyer_bindlessStorageTexelBuffers.push_back(std::make_pair(uav.index, framecount));
				}
				else
				{
					allocationhandler->destroyer_bindlessStorageBuffers.push_back(std::make_pair(uav.index, framecount));
				}
				uav = {};
			}
			for (auto& x : subresources_srv)
			{
				if (x.is_typed)
				{
					allocationhandler->destroyer_bufferviews.push_back(std::make_pair(x.buffer_view, framecount));
					allocationhandler->destroyer_bindlessUniformTexelBuffers.push_back(std::make_pair(x.index, framecount));
				}
				else
				{
					allocationhandler->destroyer_bindlessStorageBuffers.push_back(std::make_pair(x.index, framecount));
				}
			}
			subresources_srv.clear();
			for (auto& x : subresources_uav)
			{
				if (x.is_typed)
				{
					allocationhandler->destroyer_bufferviews.push_back(std::make_pair(x.buffer_view, framecount));
					allocationhandler->destroyer_bindlessStorageTexelBuffers.push_back(std::make_pair(x.index, framecount));
				}
				else
				{
					allocationhandler->destroyer_bindlessStorageBuffers.push_back(std::make_pair(x.index, framecount));
				}
			}
			subresources_uav.clear();
		}

		~Buffer_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (resource)
			{
				allocationhandler->destroyer_buffers.push_back(std::make_pair(std::make_pair(resource, allocation), framecount));
			}
			else if(allocation)
			{
				allocationhandler->destroyer_allocations.push_back(std::make_pair(allocation, framecount));
			}
			destroy_subresources();
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Texture_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VmaAllocation allocation = nullptr;
		VkImage resource = VK_NULL_HANDLE;
		VkImageLayout defaultLayout = VK_IMAGE_LAYOUT_GENERAL;
		VkBuffer staging_resource = VK_NULL_HANDLE;
		struct TextureSubresource
		{
			VkImageView image_view = VK_NULL_HANDLE;
			int index = -1; // bindless
			uint32_t firstMip = 0;
			uint32_t mipCount = 0;
			uint32_t firstSlice = 0;
			uint32_t sliceCount = 0;

			constexpr bool IsValid() const
			{
				return image_view != VK_NULL_HANDLE;
			}
		};
		TextureSubresource srv;
		TextureSubresource uav;
		TextureSubresource rtv;
		TextureSubresource dsv;
		uint32_t framebuffer_layercount = 0;
		wi::vector<TextureSubresource> subresources_srv;
		wi::vector<TextureSubresource> subresources_uav;
		wi::vector<TextureSubresource> subresources_rtv;
		wi::vector<TextureSubresource> subresources_dsv;

		wi::vector<SubresourceData> mapped_subresources;
		SparseTextureProperties sparse_texture_properties;

		VkImageView video_decode_view = VK_NULL_HANDLE;

		void destroy_subresources()
		{
			uint64_t framecount = allocationhandler->framecount;
			if (srv.IsValid())
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(srv.image_view, framecount));
				allocationhandler->destroyer_bindlessSampledImages.push_back(std::make_pair(srv.index, framecount));
				srv = {};
			}
			if (uav.IsValid())
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(uav.image_view, framecount));
				allocationhandler->destroyer_bindlessStorageImages.push_back(std::make_pair(uav.index, framecount));
				uav = {};
			}
			if (rtv.IsValid())
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(rtv.image_view, framecount));
				rtv = {};
			}
			if (dsv.IsValid())
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(dsv.image_view, framecount));
				dsv = {};
			}
			for (auto x : subresources_srv)
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(x.image_view, framecount));
				allocationhandler->destroyer_bindlessSampledImages.push_back(std::make_pair(x.index, framecount));
			}
			subresources_srv.clear();
			for (auto x : subresources_uav)
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(x.image_view, framecount));
				allocationhandler->destroyer_bindlessStorageImages.push_back(std::make_pair(x.index, framecount));
			}
			subresources_uav.clear();
			for (auto x : subresources_rtv)
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(x.image_view, framecount));
			}
			subresources_rtv.clear();
			for (auto x : subresources_dsv)
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(x.image_view, framecount));
			}
			subresources_dsv.clear();
		}

		~Texture_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (resource)
			{
				allocationhandler->destroyer_images.push_back(std::make_pair(std::make_pair(resource, allocation), framecount));
			}
			else if (staging_resource)
			{
				allocationhandler->destroyer_buffers.push_back(std::make_pair(std::make_pair(staging_resource, allocation), framecount));
			}
			else if (allocation)
			{
				allocationhandler->destroyer_allocations.push_back(std::make_pair(allocation, framecount));
			}
			if (video_decode_view != VK_NULL_HANDLE)
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(video_decode_view, framecount));
			}
			destroy_subresources();
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Sampler_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VkSampler resource = VK_NULL_HANDLE;
		int index = -1;

		~Sampler_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (resource) allocationhandler->destroyer_samplers.push_back(std::make_pair(resource, framecount));
			if (index >= 0) allocationhandler->destroyer_bindlessSamplers.push_back(std::make_pair(index, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct QueryHeap_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VkQueryPool pool = VK_NULL_HANDLE;

		~QueryHeap_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (pool) allocationhandler->destroyer_querypools.push_back(std::make_pair(pool, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Shader_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VkShaderModule shaderModule = VK_NULL_HANDLE;
		VkPipeline pipeline_cs = VK_NULL_HANDLE;
		VkPipelineShaderStageCreateInfo stageInfo = {};
		VkPipelineLayout pipelineLayout_cs = VK_NULL_HANDLE; // no lifetime management here
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE; // no lifetime management here
		wi::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		wi::vector<VkImageViewType> imageViewTypes;

		wi::vector<BindingUsage> bindlessBindings;
		wi::vector<VkDescriptorSet> bindlessSets;
		uint32_t bindlessFirstSet = 0;

		VkPushConstantRange pushconstants = {};

		VkDeviceSize uniform_buffer_sizes[DESCRIPTORBINDER_CBV_COUNT] = {};
		wi::vector<uint32_t> uniform_buffer_dynamic_slots;

		size_t binding_hash = 0;

		~Shader_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (shaderModule) allocationhandler->destroyer_shadermodules.push_back(std::make_pair(shaderModule, framecount));
			if (pipeline_cs) allocationhandler->destroyer_pipelines.push_back(std::make_pair(pipeline_cs, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct PipelineState_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE; // no lifetime management here
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE; // no lifetime management here
		wi::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		wi::vector<VkImageViewType> imageViewTypes;
		size_t hash = 0;

		wi::vector<BindingUsage> bindlessBindings;
		wi::vector<VkDescriptorSet> bindlessSets;
		uint32_t bindlessFirstSet = 0;

		VkPushConstantRange pushconstants = {};

		VkDeviceSize uniform_buffer_sizes[DESCRIPTORBINDER_CBV_COUNT] = {};
		wi::vector<uint32_t> uniform_buffer_dynamic_slots;

		size_t binding_hash = 0;

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		VkPipelineShaderStageCreateInfo shaderStages[static_cast<size_t>(ShaderStage::Count)] = {};
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		VkPipelineRasterizationDepthClipStateCreateInfoEXT depthclip = {};
		VkPipelineViewportStateCreateInfo viewportState = {};
		VkPipelineDepthStencilStateCreateInfo depthstencil = {};
		VkSampleMask samplemask = {};
		VkPipelineTessellationStateCreateInfo tessellationInfo = {};

		~PipelineState_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (pipeline) allocationhandler->destroyer_pipelines.push_back(std::make_pair(pipeline, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct BVH_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VmaAllocation allocation = nullptr;
		VkBuffer buffer = VK_NULL_HANDLE;
		VkAccelerationStructureKHR resource = VK_NULL_HANDLE;
		int index = -1;

		VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
		VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {};
		VkAccelerationStructureCreateInfoKHR createInfo = {};
		wi::vector<VkAccelerationStructureGeometryKHR> geometries;
		wi::vector<uint32_t> primitiveCounts;
		VkDeviceAddress scratch_address = 0;
		VkDeviceAddress as_address = 0;

		~BVH_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (buffer) allocationhandler->destroyer_buffers.push_back(std::make_pair(std::make_pair(buffer, allocation), framecount));
			if (resource) allocationhandler->destroyer_bvhs.push_back(std::make_pair(resource, framecount));
			if (index >= 0) allocationhandler->destroyer_bindlessAccelerationStructures.push_back(std::make_pair(index, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct RTPipelineState_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VkPipeline pipeline;

		~RTPipelineState_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (pipeline) allocationhandler->destroyer_pipelines.push_back(std::make_pair(pipeline, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct SwapChain_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VkSwapchainKHR swapChain = VK_NULL_HANDLE;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		wi::vector<VkImage> swapChainImages;
		wi::vector<VkImageView> swapChainImageViews;

		Texture dummyTexture;

		VkSurfaceKHR surface = VK_NULL_HANDLE;

		uint32_t swapChainImageIndex = 0;
		uint32_t swapChainAcquireSemaphoreIndex = 0;
		wi::vector<VkSemaphore> swapchainAcquireSemaphores;
		VkSemaphore swapchainReleaseSemaphore = VK_NULL_HANDLE;

		ColorSpace colorSpace = ColorSpace::SRGB;
		SwapChainDesc desc;
		std::mutex locker;

		~SwapChain_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;

			for (size_t i = 0; i < swapChainImages.size(); ++i)
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(swapChainImageViews[i], framecount));
				allocationhandler->destroyer_semaphores.push_back(std::make_pair(swapchainAcquireSemaphores[i], framecount));
			}

#ifdef SDL2
			// Checks if the SDL VIDEO System was already destroyed.
			// If so we would delete the swapchain twice, causing a crash on wayland.
			if (SDL_WasInit(SDL_INIT_VIDEO))
#endif
			{
				allocationhandler->destroyer_swapchains.push_back(std::make_pair(swapChain, framecount));
				allocationhandler->destroyer_surfaces.push_back(std::make_pair(surface, framecount));
			}
			allocationhandler->destroyer_semaphores.push_back(std::make_pair(swapchainReleaseSemaphore, framecount));

			allocationhandler->destroylocker.unlock();

		}
	};
	struct VideoDecoder_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VkVideoSessionKHR video_session = VK_NULL_HANDLE;
		VkVideoSessionParametersKHR session_parameters = VK_NULL_HANDLE;
		wi::vector<VmaAllocation> allocations;

		~VideoDecoder_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_video_sessions.push_back(std::make_pair(video_session, framecount));
			allocationhandler->destroyer_video_session_parameters.push_back(std::make_pair(session_parameters, framecount));
			for (auto& x : allocations)
			{
				allocationhandler->destroyer_allocations.push_back(std::make_pair(x, framecount));
			}
			allocationhandler->destroylocker.unlock();
		}
	};

	Buffer_Vulkan* to_internal(const GPUBuffer* param)
	{
		return static_cast<Buffer_Vulkan*>(param->internal_state.get());
	}
	Texture_Vulkan* to_internal(const Texture* param)
	{
		return static_cast<Texture_Vulkan*>(param->internal_state.get());
	}
	Sampler_Vulkan* to_internal(const Sampler* param)
	{
		return static_cast<Sampler_Vulkan*>(param->internal_state.get());
	}
	QueryHeap_Vulkan* to_internal(const GPUQueryHeap* param)
	{
		return static_cast<QueryHeap_Vulkan*>(param->internal_state.get());
	}
	Shader_Vulkan* to_internal(const Shader* param)
	{
		return static_cast<Shader_Vulkan*>(param->internal_state.get());
	}
	PipelineState_Vulkan* to_internal(const PipelineState* param)
	{
		return static_cast<PipelineState_Vulkan*>(param->internal_state.get());
	}
	BVH_Vulkan* to_internal(const RaytracingAccelerationStructure* param)
	{
		return static_cast<BVH_Vulkan*>(param->internal_state.get());
	}
	RTPipelineState_Vulkan* to_internal(const RaytracingPipelineState* param)
	{
		return static_cast<RTPipelineState_Vulkan*>(param->internal_state.get());
	}
	SwapChain_Vulkan* to_internal(const SwapChain* param)
	{
		return static_cast<SwapChain_Vulkan*>(param->internal_state.get());
	}
	VideoDecoder_Vulkan* to_internal(const VideoDecoder* param)
	{
		return static_cast<VideoDecoder_Vulkan*>(param->internal_state.get());
	}

	inline const std::string GetCachePath()
	{
		return wi::helper::GetCacheDirectoryPath() + "/wiPipelineCache_Vulkan";
	}

	bool CreateSwapChainInternal(
		SwapChain_Vulkan* internal_state,
		VkPhysicalDevice physicalDevice,
		VkDevice device,
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler
	)
	{
		// In vulkan, the swapchain recreate can happen whenever it gets outdated, it's not in application's control
		//	so we have to be extra careful
		std::scoped_lock lock(internal_state->locker);

		VkResult res;

		VkSurfaceCapabilitiesKHR swapchain_capabilities;
		res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, internal_state->surface, &swapchain_capabilities);
		assert(res == VK_SUCCESS);

		uint32_t formatCount;
		res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, internal_state->surface, &formatCount, nullptr);
		assert(res == VK_SUCCESS);

		wi::vector<VkSurfaceFormatKHR> swapchain_formats(formatCount);
		res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, internal_state->surface, &formatCount, swapchain_formats.data());
		assert(res == VK_SUCCESS);

		uint32_t presentModeCount;
		res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, internal_state->surface, &presentModeCount, nullptr);
		assert(res == VK_SUCCESS);

		wi::vector<VkPresentModeKHR> swapchain_presentModes(presentModeCount);
		swapchain_presentModes.resize(presentModeCount);
		res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, internal_state->surface, &presentModeCount, swapchain_presentModes.data());
		assert(res == VK_SUCCESS);

		VkSurfaceFormatKHR surfaceFormat = {};
		surfaceFormat.format = _ConvertFormat(internal_state->desc.format);
		surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		bool valid = false;

		for (const auto& format : swapchain_formats)
		{
			if (!internal_state->desc.allow_hdr && format.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				continue;
			if (format.format == surfaceFormat.format)
			{
				surfaceFormat = format;
				valid = true;
				break;
			}
		}
		if (!valid)
		{
			internal_state->desc.format = Format::B8G8R8A8_UNORM;
			surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
			surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		}

		// For now, we only include the color spaces that were tested successfully:
		ColorSpace prev_colorspace = internal_state->colorSpace;
		switch (surfaceFormat.colorSpace)
		{
		default:
		case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
			internal_state->colorSpace = ColorSpace::SRGB;
			break;
		case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
			internal_state->colorSpace = ColorSpace::HDR_LINEAR;
			break;
		case VK_COLOR_SPACE_HDR10_ST2084_EXT:
			internal_state->colorSpace = ColorSpace::HDR10_ST2084;
			break;
		}

		if (prev_colorspace != internal_state->colorSpace)
		{
			if (internal_state->swapChain != VK_NULL_HANDLE)
			{
				// For some reason, if the swapchain gets recreated (via oldSwapChain) with different color space but same image format,
				//	the color space change will not be applied
				res = vkDeviceWaitIdle(device);
				assert(res == VK_SUCCESS);
				vkDestroySwapchainKHR(device, internal_state->swapChain, nullptr);
				internal_state->swapChain = nullptr;
			}
		}

		if (swapchain_capabilities.currentExtent.width != 0xFFFFFFFF && swapchain_capabilities.currentExtent.height != 0xFFFFFFFF)
		{
			internal_state->swapChainExtent = swapchain_capabilities.currentExtent;
		}
		else
		{
			internal_state->swapChainExtent = { internal_state->desc.width, internal_state->desc.height };
			internal_state->swapChainExtent.width = std::max(swapchain_capabilities.minImageExtent.width, std::min(swapchain_capabilities.maxImageExtent.width, internal_state->swapChainExtent.width));
			internal_state->swapChainExtent.height = std::max(swapchain_capabilities.minImageExtent.height, std::min(swapchain_capabilities.maxImageExtent.height, internal_state->swapChainExtent.height));
		}

		uint32_t imageCount = std::max(internal_state->desc.buffer_count, swapchain_capabilities.minImageCount);
		if ((swapchain_capabilities.maxImageCount > 0) && (imageCount > swapchain_capabilities.maxImageCount))
		{
			imageCount = swapchain_capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = internal_state->surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = internal_state->swapChainExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.preTransform = swapchain_capabilities.currentTransform;

		createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // The only one that is always supported
		if (!internal_state->desc.vsync)
		{
			// The mailbox/immediate present mode is not necessarily supported:
			for (auto& presentMode : swapchain_presentModes)
			{
				if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					createInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
				if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					createInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
				}
			}
		}
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = internal_state->swapChain;

		res = vkCreateSwapchainKHR(device, &createInfo, nullptr, &internal_state->swapChain);
		assert(res == VK_SUCCESS);

		if (createInfo.oldSwapchain != VK_NULL_HANDLE)
		{
			std::scoped_lock lock(allocationhandler->destroylocker);
			allocationhandler->destroyer_swapchains.emplace_back(createInfo.oldSwapchain, allocationhandler->framecount);
		}

		res = vkGetSwapchainImagesKHR(device, internal_state->swapChain, &imageCount, nullptr);
		assert(res == VK_SUCCESS);
		internal_state->swapChainImages.resize(imageCount);
		res = vkGetSwapchainImagesKHR(device, internal_state->swapChain, &imageCount, internal_state->swapChainImages.data());
		assert(res == VK_SUCCESS);
		internal_state->swapChainImageFormat = surfaceFormat.format;

		// Create swap chain render targets:
		internal_state->swapChainImageViews.resize(internal_state->swapChainImages.size());
		for (size_t i = 0; i < internal_state->swapChainImages.size(); ++i)
		{
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = internal_state->swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = internal_state->swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (internal_state->swapChainImageViews[i] != VK_NULL_HANDLE)
			{
				allocationhandler->destroylocker.lock();
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(internal_state->swapChainImageViews[i], allocationhandler->framecount));
				allocationhandler->destroylocker.unlock();
			}
			res = vkCreateImageView(device, &createInfo, nullptr, &internal_state->swapChainImageViews[i]);
			assert(res == VK_SUCCESS);
		}


		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (internal_state->swapchainAcquireSemaphores.empty())
		{
			for (size_t i = 0; i < internal_state->swapChainImages.size(); ++i)
			{
				res = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &internal_state->swapchainAcquireSemaphores.emplace_back());
				assert(res == VK_SUCCESS);
			}
		}

		if (internal_state->swapchainReleaseSemaphore == VK_NULL_HANDLE)
		{
			res = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &internal_state->swapchainReleaseSemaphore);
			assert(res == VK_SUCCESS);
		}

		return true;
	}
}
using namespace vulkan_internal;



	void GraphicsDevice_Vulkan::CommandQueue::signal(VkSemaphore semaphore)
	{
		if (queue == VK_NULL_HANDLE)
			return;
		VkSemaphoreSubmitInfo& signalSemaphore = submit_signalSemaphoreInfos.emplace_back();
		signalSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphore.semaphore = semaphore;
		signalSemaphore.value = 0; // not a timeline semaphore
		signalSemaphore.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	}
	void GraphicsDevice_Vulkan::CommandQueue::wait(VkSemaphore semaphore)
	{
		if (queue == VK_NULL_HANDLE)
			return;
		VkSemaphoreSubmitInfo& waitSemaphore = submit_waitSemaphoreInfos.emplace_back();
		waitSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitSemaphore.semaphore = semaphore;
		waitSemaphore.value = 0; // not a timeline semaphore
		waitSemaphore.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	}
	void GraphicsDevice_Vulkan::CommandQueue::submit(GraphicsDevice_Vulkan* device, VkFence fence)
	{
		if (queue == VK_NULL_HANDLE)
			return;
		std::scoped_lock lock(*locker);

		VkSubmitInfo2 submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.commandBufferInfoCount = (uint32_t)submit_cmds.size();
		submitInfo.pCommandBufferInfos = submit_cmds.data();

		submitInfo.waitSemaphoreInfoCount = (uint32_t)submit_waitSemaphoreInfos.size();
		submitInfo.pWaitSemaphoreInfos = submit_waitSemaphoreInfos.data();

		submitInfo.signalSemaphoreInfoCount = (uint32_t)submit_signalSemaphoreInfos.size();
		submitInfo.pSignalSemaphoreInfos = submit_signalSemaphoreInfos.data();

		VkResult res = vkQueueSubmit2(queue, 1, &submitInfo, fence);
		assert(res == VK_SUCCESS);

		if (!submit_swapchains.empty())
		{
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = (uint32_t)submit_signalSemaphores.size();
			presentInfo.pWaitSemaphores = submit_signalSemaphores.data();
			presentInfo.swapchainCount = (uint32_t)submit_swapchains.size();
			presentInfo.pSwapchains = submit_swapchains.data();
			presentInfo.pImageIndices = submit_swapChainImageIndices.data();
			res = vkQueuePresentKHR(queue, &presentInfo);
			if (res != VK_SUCCESS)
			{
				// Handle outdated error in present:
				if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
				{
					for (auto& swapchain : swapchain_updates)
					{
						auto internal_state = to_internal(&swapchain);
						bool success = CreateSwapChainInternal(internal_state, device->physicalDevice, device->device, device->allocationhandler);
						assert(success);
					}
				}
				else
				{
					assert(0);
				}
			}
		}

		swapchain_updates.clear();
		submit_swapchains.clear();
		submit_swapChainImageIndices.clear();
		submit_waitSemaphoreInfos.clear();
		submit_signalSemaphores.clear();
		submit_signalSemaphoreInfos.clear();
		submit_cmds.clear();
	}

	void GraphicsDevice_Vulkan::CopyAllocator::init(GraphicsDevice_Vulkan* device)
	{
		this->device = device;
	}
	void GraphicsDevice_Vulkan::CopyAllocator::destroy()
	{
		vkQueueWaitIdle(device->queues[QUEUE_COPY].queue);
		for (auto& x : freelist)
		{
			vkDestroyCommandPool(device->device, x.transferCommandPool, nullptr);
			vkDestroyCommandPool(device->device, x.transitionCommandPool, nullptr);
			for (auto& sema : x.semaphores)
			{
				vkDestroySemaphore(device->device, sema, nullptr);
			}
			vkDestroyFence(device->device, x.fence, nullptr);
		}
	}
	GraphicsDevice_Vulkan::CopyAllocator::CopyCMD GraphicsDevice_Vulkan::CopyAllocator::allocate(uint64_t staging_size)
	{
		CopyCMD cmd;

		locker.lock();
		// Try to search for a staging buffer that can fit the request:
		for (size_t i = 0; i < freelist.size(); ++i)
		{
			if (freelist[i].uploadbuffer.desc.size >= staging_size)
			{
				if (vkGetFenceStatus(device->device, freelist[i].fence) == VK_SUCCESS)
				{
					cmd = std::move(freelist[i]);
					std::swap(freelist[i], freelist.back());
					freelist.pop_back();
					break;
				}
			}
		}
		locker.unlock();

		// If no buffer was found that fits the data, create one:
		if (!cmd.IsValid())
		{
			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			poolInfo.queueFamilyIndex = device->copyFamily;
			VkResult res = vkCreateCommandPool(device->device, &poolInfo, nullptr, &cmd.transferCommandPool);
			assert(res == VK_SUCCESS);
			poolInfo.queueFamilyIndex = device->graphicsFamily;
			res = vkCreateCommandPool(device->device, &poolInfo, nullptr, &cmd.transitionCommandPool);
			assert(res == VK_SUCCESS);

			VkCommandBufferAllocateInfo commandBufferInfo = {};
			commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferInfo.commandBufferCount = 1;
			commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			commandBufferInfo.commandPool = cmd.transferCommandPool;
			res = vkAllocateCommandBuffers(device->device, &commandBufferInfo, &cmd.transferCommandBuffer);
			assert(res == VK_SUCCESS);
			commandBufferInfo.commandPool = cmd.transitionCommandPool;
			res = vkAllocateCommandBuffers(device->device, &commandBufferInfo, &cmd.transitionCommandBuffer);
			assert(res == VK_SUCCESS);

			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			res = vkCreateFence(device->device, &fenceInfo, nullptr, &cmd.fence);
			assert(res == VK_SUCCESS);

			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			res = vkCreateSemaphore(device->device, &semaphoreInfo, nullptr, &cmd.semaphores[0]);
			assert(res == VK_SUCCESS);
			res = vkCreateSemaphore(device->device, &semaphoreInfo, nullptr, &cmd.semaphores[1]);
			assert(res == VK_SUCCESS);
			res = vkCreateSemaphore(device->device, &semaphoreInfo, nullptr, &cmd.semaphores[2]);
			assert(res == VK_SUCCESS);

			GPUBufferDesc uploaddesc;
			uploaddesc.size = wi::math::GetNextPowerOfTwo(staging_size);
			uploaddesc.size = std::max(uploaddesc.size, uint64_t(65536));
			uploaddesc.usage = Usage::UPLOAD;
			bool upload_success = device->CreateBuffer(&uploaddesc, nullptr, &cmd.uploadbuffer);
			assert(upload_success);
			device->SetName(&cmd.uploadbuffer, "CopyAllocator::uploadBuffer");
		}

		// begin command list in valid state:
		VkResult res = vkResetCommandPool(device->device, cmd.transferCommandPool, 0);
		assert(res == VK_SUCCESS);
		res = vkResetCommandPool(device->device, cmd.transitionCommandPool, 0);
		assert(res == VK_SUCCESS);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;
		res = vkBeginCommandBuffer(cmd.transferCommandBuffer, &beginInfo);
		assert(res == VK_SUCCESS);
		res = vkBeginCommandBuffer(cmd.transitionCommandBuffer, &beginInfo);
		assert(res == VK_SUCCESS);

		res = vkResetFences(device->device, 1, &cmd.fence);
		assert(res == VK_SUCCESS);

		return cmd;
	}
	void GraphicsDevice_Vulkan::CopyAllocator::submit(CopyCMD cmd)
	{
		VkResult res = vkEndCommandBuffer(cmd.transferCommandBuffer);
		assert(res == VK_SUCCESS);
		res = vkEndCommandBuffer(cmd.transitionCommandBuffer);
		assert(res == VK_SUCCESS);

		VkSubmitInfo2 submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

		VkCommandBufferSubmitInfo cbSubmitInfo = {};
		cbSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;

		VkSemaphoreSubmitInfo signalSemaphoreInfos[2] = {};
		signalSemaphoreInfos[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphoreInfos[1].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;

		VkSemaphoreSubmitInfo waitSemaphoreInfo = {};
		waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;

		{
			cbSubmitInfo.commandBuffer = cmd.transferCommandBuffer;
			signalSemaphoreInfos[0].semaphore = cmd.semaphores[0]; // signal for graphics queue
			signalSemaphoreInfos[0].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

			submitInfo.commandBufferInfoCount = 1;
			submitInfo.pCommandBufferInfos = &cbSubmitInfo;
			submitInfo.signalSemaphoreInfoCount = 1;
			submitInfo.pSignalSemaphoreInfos = signalSemaphoreInfos;

			std::scoped_lock lock(*device->queues[QUEUE_COPY].locker);
			res = vkQueueSubmit2(device->queues[QUEUE_COPY].queue, 1, &submitInfo, VK_NULL_HANDLE);
			assert(res == VK_SUCCESS);
		}

		{
			waitSemaphoreInfo.semaphore = cmd.semaphores[0]; // wait for copy queue
			waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

			cbSubmitInfo.commandBuffer = cmd.transitionCommandBuffer;
			signalSemaphoreInfos[0].semaphore = cmd.semaphores[1]; // signal for compute queue
			signalSemaphoreInfos[0].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // signal for compute queue

			submitInfo.waitSemaphoreInfoCount = 1;
			submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
			submitInfo.commandBufferInfoCount = 1;
			submitInfo.pCommandBufferInfos = &cbSubmitInfo;
			if (device->queues[QUEUE_VIDEO_DECODE].queue != VK_NULL_HANDLE)
			{
				signalSemaphoreInfos[1].semaphore = cmd.semaphores[2]; // signal for video decode queue
				signalSemaphoreInfos[1].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // signal for video decode queue
				submitInfo.signalSemaphoreInfoCount = 2;
			}
			else
			{
				submitInfo.signalSemaphoreInfoCount = 1;
			}
			submitInfo.pSignalSemaphoreInfos = signalSemaphoreInfos;

			std::scoped_lock lock(*device->queues[QUEUE_GRAPHICS].locker);
			res = vkQueueSubmit2(device->queues[QUEUE_GRAPHICS].queue, 1, &submitInfo, VK_NULL_HANDLE);
			assert(res == VK_SUCCESS);
		}

		if (device->queues[QUEUE_VIDEO_DECODE].queue != VK_NULL_HANDLE)
		{
			waitSemaphoreInfo.semaphore = cmd.semaphores[2]; // wait for graphics queue
			waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

			submitInfo.waitSemaphoreInfoCount = 1;
			submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
			submitInfo.commandBufferInfoCount = 0;
			submitInfo.pCommandBufferInfos = nullptr;
			submitInfo.signalSemaphoreInfoCount = 0;
			submitInfo.pSignalSemaphoreInfos = nullptr;

			std::scoped_lock lock(*device->queues[QUEUE_VIDEO_DECODE].locker);
			res = vkQueueSubmit2(device->queues[QUEUE_VIDEO_DECODE].queue, 1, &submitInfo, VK_NULL_HANDLE);
			assert(res == VK_SUCCESS);
		}

		// This must be final submit in this function because it will also signal a fence for state tracking by CPU!
		{
			waitSemaphoreInfo.semaphore = cmd.semaphores[1]; // wait for graphics queue
			waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

			submitInfo.waitSemaphoreInfoCount = 1;
			submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
			submitInfo.commandBufferInfoCount = 0;
			submitInfo.pCommandBufferInfos = nullptr;
			submitInfo.signalSemaphoreInfoCount = 0;
			submitInfo.pSignalSemaphoreInfos = nullptr;

			std::scoped_lock lock(*device->queues[QUEUE_COMPUTE].locker);
			res = vkQueueSubmit2(device->queues[QUEUE_COMPUTE].queue, 1, &submitInfo, cmd.fence); // final submit also signals fence!
			assert(res == VK_SUCCESS);
		}

		std::scoped_lock lock(locker);
		freelist.push_back(cmd);
	}

	void GraphicsDevice_Vulkan::DescriptorBinderPool::init(GraphicsDevice_Vulkan* device)
	{
		this->device = device;

		VkResult res;

		// Create descriptor pool:
		VkDescriptorPoolSize poolSizes[10] = {};
		uint32_t count = 0;

		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = DESCRIPTORBINDER_CBV_COUNT * poolSize;
		count++;

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		poolSizes[1].descriptorCount = DESCRIPTORBINDER_CBV_COUNT * poolSize;
		count++;

		poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		poolSizes[2].descriptorCount = DESCRIPTORBINDER_SRV_COUNT * poolSize;
		count++;

		poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		poolSizes[3].descriptorCount = DESCRIPTORBINDER_SRV_COUNT * poolSize;
		count++;

		poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[4].descriptorCount = DESCRIPTORBINDER_SRV_COUNT * poolSize;
		count++;

		poolSizes[5].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[5].descriptorCount = DESCRIPTORBINDER_UAV_COUNT * poolSize;
		count++;

		poolSizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		poolSizes[6].descriptorCount = DESCRIPTORBINDER_UAV_COUNT * poolSize;
		count++;

		poolSizes[7].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[7].descriptorCount = DESCRIPTORBINDER_UAV_COUNT * poolSize;
		count++;

		poolSizes[8].type = VK_DESCRIPTOR_TYPE_SAMPLER;
		poolSizes[8].descriptorCount = DESCRIPTORBINDER_SAMPLER_COUNT * poolSize;
		count++;

		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			poolSizes[9].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			poolSizes[9].descriptorCount = DESCRIPTORBINDER_SRV_COUNT * poolSize;
			count++;
		}

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = count;
		poolInfo.pPoolSizes = poolSizes;
		poolInfo.maxSets = poolSize;

		res = vkCreateDescriptorPool(device->device, &poolInfo, nullptr, &descriptorPool);
		assert(res == VK_SUCCESS);

		// WARNING: MUST NOT CALL reset() HERE!
		//	This is because init can be called mid-frame when there is allocation error, but the bindings must be retained!

	}
	void GraphicsDevice_Vulkan::DescriptorBinderPool::destroy()
	{
		if (descriptorPool != VK_NULL_HANDLE)
		{
			device->allocationhandler->destroylocker.lock();
			device->allocationhandler->destroyer_descriptorPools.push_back(std::make_pair(descriptorPool, device->FRAMECOUNT));
			descriptorPool = VK_NULL_HANDLE;
			device->allocationhandler->destroylocker.unlock();
		}
	}
	void GraphicsDevice_Vulkan::DescriptorBinderPool::reset()
	{
		if (descriptorPool != VK_NULL_HANDLE)
		{
			VkResult res = vkResetDescriptorPool(device->device, descriptorPool, 0);
			assert(res == VK_SUCCESS);
		}
	}

	void GraphicsDevice_Vulkan::DescriptorBinder::init(GraphicsDevice_Vulkan* device)
	{
		this->device = device;

		// Important that these don't reallocate themselves during writing descriptors!
		descriptorWrites.reserve(128);
		bufferInfos.reserve(128);
		imageInfos.reserve(128);
		texelBufferViews.reserve(128);
		accelerationStructureViews.reserve(128);
	}
	void GraphicsDevice_Vulkan::DescriptorBinder::reset()
	{
		table = {};
		dirty = true;
	}
	void GraphicsDevice_Vulkan::DescriptorBinder::flush(bool graphics, CommandList cmd)
	{
		if (dirty == DIRTY_NONE)
			return;

		CommandList_Vulkan& commandlist = device->GetCommandList(cmd);
		auto pso_internal = graphics ? to_internal(commandlist.active_pso) : nullptr;
		auto cs_internal = graphics ? nullptr : to_internal(commandlist.active_cs);
		VkCommandBuffer commandBuffer = commandlist.GetCommandBuffer();

		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		uint32_t uniform_buffer_dynamic_count = 0;
		if (graphics)
		{
			pipelineLayout = pso_internal->pipelineLayout;
			descriptorSetLayout = pso_internal->descriptorSetLayout;
			descriptorSet = descriptorSet_graphics;
			uniform_buffer_dynamic_count = (uint32_t)pso_internal->uniform_buffer_dynamic_slots.size();
			for (size_t i = 0; i < pso_internal->uniform_buffer_dynamic_slots.size(); ++i)
			{
				uniform_buffer_dynamic_offsets[i] = (uint32_t)table.CBV_offset[pso_internal->uniform_buffer_dynamic_slots[i]];
			}
		}
		else
		{
			pipelineLayout = cs_internal->pipelineLayout_cs;
			descriptorSetLayout = cs_internal->descriptorSetLayout;
			descriptorSet = descriptorSet_compute;
			uniform_buffer_dynamic_count = (uint32_t)cs_internal->uniform_buffer_dynamic_slots.size();
			for (size_t i = 0; i < cs_internal->uniform_buffer_dynamic_slots.size(); ++i)
			{
				uniform_buffer_dynamic_offsets[i] = (uint32_t)table.CBV_offset[cs_internal->uniform_buffer_dynamic_slots[i]];
			}
		}

		if (dirty & DIRTY_DESCRIPTOR)
		{
			auto& binder_pool = commandlist.binder_pools[device->GetBufferIndex()];

			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = binder_pool.descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &descriptorSetLayout;

			VkResult res = vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet);
			while (res == VK_ERROR_OUT_OF_POOL_MEMORY)
			{
				binder_pool.poolSize *= 2;
				binder_pool.destroy();
				binder_pool.init(device);
				allocInfo.descriptorPool = binder_pool.descriptorPool;
				res = vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet);
			}
			assert(res == VK_SUCCESS);

			descriptorWrites.clear();
			bufferInfos.clear();
			imageInfos.clear();
			texelBufferViews.clear();
			accelerationStructureViews.clear();

			const auto& layoutBindings = graphics ? pso_internal->layoutBindings : cs_internal->layoutBindings;
			const auto& imageViewTypes = graphics ? pso_internal->imageViewTypes : cs_internal->imageViewTypes;


			int i = 0;
			for (auto& x : layoutBindings)
			{
				if (x.pImmutableSamplers != nullptr)
				{
					i++;
					continue;
				}

				VkImageViewType viewtype = imageViewTypes[i++];

				for (uint32_t descriptor_index = 0; descriptor_index < x.descriptorCount; ++descriptor_index)
				{
					uint32_t unrolled_binding = x.binding + descriptor_index;

					descriptorWrites.emplace_back();
					auto& write = descriptorWrites.back();
					write = {};
					write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					write.dstSet = descriptorSet;
					write.dstArrayElement = descriptor_index;
					write.descriptorType = x.descriptorType;
					write.dstBinding = x.binding;
					write.descriptorCount = 1;

					switch (x.descriptorType)
					{
					case VK_DESCRIPTOR_TYPE_SAMPLER:
					{
						imageInfos.emplace_back();
						write.pImageInfo = &imageInfos.back();
						imageInfos.back() = {};

						const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_S;
						const Sampler& sampler = table.SAM[original_binding];
						if (!sampler.IsValid())
						{
							imageInfos.back().sampler = device->nullSampler;
						}
						else
						{
							imageInfos.back().sampler = to_internal(&sampler)->resource;
						}
					}
					break;

					case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
					{
						imageInfos.emplace_back();
						write.pImageInfo = &imageInfos.back();
						imageInfos.back() = {};

						const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_T;
						const GPUResource& resource = table.SRV[original_binding];
						if (!resource.IsValid() || !resource.IsTexture())
						{
							switch (viewtype)
							{
							case VK_IMAGE_VIEW_TYPE_1D:
								imageInfos.back().imageView = device->nullImageView1D;
								break;
							case VK_IMAGE_VIEW_TYPE_2D:
								imageInfos.back().imageView = device->nullImageView2D;
								break;
							case VK_IMAGE_VIEW_TYPE_3D:
								imageInfos.back().imageView = device->nullImageView3D;
								break;
							case VK_IMAGE_VIEW_TYPE_CUBE:
								imageInfos.back().imageView = device->nullImageViewCube;
								break;
							case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
								imageInfos.back().imageView = device->nullImageView1DArray;
								break;
							case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
								imageInfos.back().imageView = device->nullImageView2DArray;
								break;
							case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
								imageInfos.back().imageView = device->nullImageViewCubeArray;
								break;
							case VK_IMAGE_VIEW_TYPE_MAX_ENUM:
								break;
							default:
								break;
							}
							imageInfos.back().imageLayout = VK_IMAGE_LAYOUT_GENERAL;
						}
						else
						{
							int subresource = table.SRV_index[original_binding];
							auto texture_internal = to_internal((const Texture*)&resource);
							auto& subresource_descriptor = subresource >= 0 ? texture_internal->subresources_srv[subresource] : texture_internal->srv;
							imageInfos.back().imageView = subresource_descriptor.image_view;
							imageInfos.back().imageLayout = texture_internal->defaultLayout;
						}
					}
					break;

					case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					{
						imageInfos.emplace_back();
						write.pImageInfo = &imageInfos.back();
						imageInfos.back() = {};
						imageInfos.back().imageLayout = VK_IMAGE_LAYOUT_GENERAL;

						const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_U;
						const GPUResource& resource = table.UAV[original_binding];
						if (!resource.IsValid() || !resource.IsTexture())
						{
							switch (viewtype)
							{
							case VK_IMAGE_VIEW_TYPE_1D:
								imageInfos.back().imageView = device->nullImageView1D;
								break;
							case VK_IMAGE_VIEW_TYPE_2D:
								imageInfos.back().imageView = device->nullImageView2D;
								break;
							case VK_IMAGE_VIEW_TYPE_3D:
								imageInfos.back().imageView = device->nullImageView3D;
								break;
							case VK_IMAGE_VIEW_TYPE_CUBE:
								imageInfos.back().imageView = device->nullImageViewCube;
								break;
							case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
								imageInfos.back().imageView = device->nullImageView1DArray;
								break;
							case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
								imageInfos.back().imageView = device->nullImageView2DArray;
								break;
							case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
								imageInfos.back().imageView = device->nullImageViewCubeArray;
								break;
							case VK_IMAGE_VIEW_TYPE_MAX_ENUM:
								break;
							default:
								break;
							}
						}
						else
						{
							int subresource = table.UAV_index[original_binding];
							auto texture_internal = to_internal((const Texture*)&resource);
							auto& subresource_descriptor = subresource >= 0 ? texture_internal->subresources_uav[subresource] : texture_internal->uav;
							imageInfos.back().imageView = subresource_descriptor.image_view;
						}
					}
					break;

					case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
					{
						bufferInfos.emplace_back();
						write.pBufferInfo = &bufferInfos.back();
						bufferInfos.back() = {};

						const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_B;
						const GPUBuffer& buffer = table.CBV[original_binding];
						uint64_t offset = table.CBV_offset[original_binding];

						if (!buffer.IsValid())
						{
							bufferInfos.back().buffer = device->nullBuffer;
							bufferInfos.back().range = VK_WHOLE_SIZE;
						}
						else
						{
							auto internal_state = to_internal(&buffer);
							bufferInfos.back().buffer = internal_state->resource;
							bufferInfos.back().offset = offset;
							if (graphics)
							{
								bufferInfos.back().range = pso_internal->uniform_buffer_sizes[original_binding];
							}
							else
							{
								bufferInfos.back().range = cs_internal->uniform_buffer_sizes[original_binding];
							}
							if (bufferInfos.back().range == 0ull)
							{
								bufferInfos.back().range = VK_WHOLE_SIZE;
							}
						}
					}
					break;

					case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
					{
						bufferInfos.emplace_back();
						write.pBufferInfo = &bufferInfos.back();
						bufferInfos.back() = {};

						const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_B;
						const GPUBuffer& buffer = table.CBV[original_binding];

						if (!buffer.IsValid())
						{
							bufferInfos.back().buffer = device->nullBuffer;
							bufferInfos.back().range = VK_WHOLE_SIZE;
						}
						else
						{
							auto internal_state = to_internal(&buffer);
							bufferInfos.back().buffer = internal_state->resource;
							if (graphics)
							{
								bufferInfos.back().range = pso_internal->uniform_buffer_sizes[original_binding];
							}
							else
							{
								bufferInfos.back().range = cs_internal->uniform_buffer_sizes[original_binding];
							}
							if (bufferInfos.back().range == 0ull)
							{
								bufferInfos.back().range = VK_WHOLE_SIZE;
							}
						}
					}
					break;

					case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
					{
						texelBufferViews.emplace_back();
						write.pTexelBufferView = &texelBufferViews.back();
						texelBufferViews.back() = {};

						const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_T;
						const GPUResource& resource = table.SRV[original_binding];
						if (!resource.IsValid() || !resource.IsBuffer())
						{
							texelBufferViews.back() = device->nullBufferView;
						}
						else
						{
							int subresource = table.SRV_index[original_binding];
							auto buffer_internal = to_internal((const GPUBuffer*)&resource);
							auto& subresource_descriptor = subresource >= 0 ? buffer_internal->subresources_srv[subresource] : buffer_internal->srv;
							texelBufferViews.back() = subresource_descriptor.buffer_view;
						}
					}
					break;

					case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
					{
						texelBufferViews.emplace_back();
						write.pTexelBufferView = &texelBufferViews.back();
						texelBufferViews.back() = {};

						const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_U;
						const GPUResource& resource = table.UAV[original_binding];
						if (!resource.IsValid() || !resource.IsBuffer())
						{
							texelBufferViews.back() = device->nullBufferView;
						}
						else
						{
							int subresource = table.UAV_index[original_binding];
							auto buffer_internal = to_internal((const GPUBuffer*)&resource);
							auto& subresource_descriptor = subresource >= 0 ? buffer_internal->subresources_uav[subresource] : buffer_internal->uav;
							texelBufferViews.back() = subresource_descriptor.buffer_view;
						}
					}
					break;

					case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
					{
						bufferInfos.emplace_back();
						write.pBufferInfo = &bufferInfos.back();
						bufferInfos.back() = {};

						if (x.binding < VULKAN_BINDING_SHIFT_U)
						{
							// SRV
							const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_T;
							const GPUResource& resource = table.SRV[original_binding];
							if (!resource.IsValid() || !resource.IsBuffer())
							{
								bufferInfos.back().buffer = device->nullBuffer;
								bufferInfos.back().range = VK_WHOLE_SIZE;
							}
							else
							{
								int subresource = table.SRV_index[original_binding];
								auto buffer_internal = to_internal((const GPUBuffer*)&resource);
								auto& subresource_descriptor = subresource >= 0 ? buffer_internal->subresources_srv[subresource] : buffer_internal->srv;
								bufferInfos.back() = subresource_descriptor.buffer_info;
							}
						}
						else
						{
							// UAV
							const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_U;
							const GPUResource& resource = table.UAV[original_binding];
							if (!resource.IsValid() || !resource.IsBuffer())
							{
								bufferInfos.back().buffer = device->nullBuffer;
								bufferInfos.back().range = VK_WHOLE_SIZE;
							}
							else
							{
								int subresource = table.UAV_index[original_binding];
								auto buffer_internal = to_internal((const GPUBuffer*)&resource);
								auto& subresource_descriptor = subresource >= 0 ? buffer_internal->subresources_uav[subresource] : buffer_internal->uav;
								bufferInfos.back() = subresource_descriptor.buffer_info;
							}
						}
					}
					break;

					case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
					{
						accelerationStructureViews.emplace_back();
						write.pNext = &accelerationStructureViews.back();
						accelerationStructureViews.back() = {};
						accelerationStructureViews.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
						accelerationStructureViews.back().accelerationStructureCount = 1;

						const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_T;
						const GPUResource& resource = table.SRV[original_binding];
						if (!resource.IsValid() || !resource.IsAccelerationStructure())
						{
							assert(0); // invalid acceleration structure!
						}
						else
						{
							auto as_internal = to_internal((const RaytracingAccelerationStructure*)&resource);
							accelerationStructureViews.back().pAccelerationStructures = &as_internal->resource;
						}
					}
					break;

					default: break;
					}

				}
			}

			vkUpdateDescriptorSets(
				device->device,
				(uint32_t)descriptorWrites.size(),
				descriptorWrites.data(),
				0,
				nullptr
			);
		}

		VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		if (!graphics)
		{
			bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

			if (commandlist.active_cs->stage == ShaderStage::LIB)
			{
				bindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
			}
		}
		vkCmdBindDescriptorSets(
			commandBuffer,
			bindPoint,
			pipelineLayout,
			0,
			1,
			&descriptorSet,
			uniform_buffer_dynamic_count,
			uniform_buffer_dynamic_offsets
		);

		// Save last used descriptor set handles:
		//	This is needed to handle the case when descriptorSet is not allocated, but only dynamic offsets are updated
		if (graphics)
		{
			descriptorSet_graphics = descriptorSet;
		}
		else
		{
			descriptorSet_compute = descriptorSet;
		}

		dirty = DIRTY_NONE;
	}

	void GraphicsDevice_Vulkan::pso_validate(CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		if (!commandlist.dirty_pso)
			return;

		const PipelineState* pso = commandlist.active_pso;
		size_t pipeline_hash = commandlist.prev_pipeline_hash;
		auto internal_state = to_internal(pso);

		VkPipeline pipeline = VK_NULL_HANDLE;
		auto it = pipelines_global.find(pipeline_hash);
		if (it == pipelines_global.end())
		{
			for (auto& x : commandlist.pipelines_worker)
			{
				if (pipeline_hash == x.first)
				{
					pipeline = x.second;
					break;
				}
			}

			if (pipeline == VK_NULL_HANDLE)
			{
				VkGraphicsPipelineCreateInfo pipelineInfo = internal_state->pipelineInfo; // make a copy here

				// MSAA:
				VkPipelineMultisampleStateCreateInfo multisampling = {};
				multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisampling.sampleShadingEnable = VK_FALSE;
				multisampling.rasterizationSamples = (VkSampleCountFlagBits)commandlist.renderpass_info.sample_count;
				if (pso->desc.rs != nullptr)
				{
					const RasterizerState& desc = *pso->desc.rs;
					if (desc.forced_sample_count > 1)
					{
						multisampling.rasterizationSamples = (VkSampleCountFlagBits)desc.forced_sample_count;
					}
				}
				multisampling.minSampleShading = 1.0f;
				VkSampleMask samplemask = internal_state->samplemask;
				samplemask = pso->desc.sample_mask;
				multisampling.pSampleMask = &samplemask;
				if (pso->desc.bs != nullptr)
				{
					multisampling.alphaToCoverageEnable = pso->desc.bs->alpha_to_coverage_enable ? VK_TRUE : VK_FALSE;
				}
				else
				{
					multisampling.alphaToCoverageEnable = VK_FALSE;
				}
				multisampling.alphaToOneEnable = VK_FALSE;

				pipelineInfo.pMultisampleState = &multisampling;


				// Blending:
				uint32_t numBlendAttachments = 0;
				VkPipelineColorBlendAttachmentState colorBlendAttachments[8] = {};
				for (size_t i = 0; i < commandlist.renderpass_info.rt_count; ++i)
				{
					size_t attachmentIndex = 0;
					if (pso->desc.bs->independent_blend_enable)
						attachmentIndex = i;

					const auto& desc = pso->desc.bs->render_target[attachmentIndex];
					VkPipelineColorBlendAttachmentState& attachment = colorBlendAttachments[numBlendAttachments];
					numBlendAttachments++;

					attachment.blendEnable = desc.blend_enable ? VK_TRUE : VK_FALSE;

					attachment.colorWriteMask = 0;
					if (has_flag(desc.render_target_write_mask, ColorWrite::ENABLE_RED))
					{
						attachment.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
					}
					if (has_flag(desc.render_target_write_mask, ColorWrite::ENABLE_GREEN))
					{
						attachment.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
					}
					if (has_flag(desc.render_target_write_mask, ColorWrite::ENABLE_BLUE))
					{
						attachment.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
					}
					if (has_flag(desc.render_target_write_mask, ColorWrite::ENABLE_ALPHA))
					{
						attachment.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
					}

					attachment.srcColorBlendFactor = _ConvertBlend(desc.src_blend);
					attachment.dstColorBlendFactor = _ConvertBlend(desc.dest_blend);
					attachment.colorBlendOp = _ConvertBlendOp(desc.blend_op);
					attachment.srcAlphaBlendFactor = _ConvertBlend(desc.src_blend_alpha);
					attachment.dstAlphaBlendFactor = _ConvertBlend(desc.dest_blend_alpha);
					attachment.alphaBlendOp = _ConvertBlendOp(desc.blend_op_alpha);
				}

				VkPipelineColorBlendStateCreateInfo colorBlending = {};
				colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				colorBlending.logicOpEnable = VK_FALSE;
				colorBlending.logicOp = VK_LOGIC_OP_COPY;
				colorBlending.attachmentCount = numBlendAttachments;
				colorBlending.pAttachments = colorBlendAttachments;
				colorBlending.blendConstants[0] = 1.0f;
				colorBlending.blendConstants[1] = 1.0f;
				colorBlending.blendConstants[2] = 1.0f;
				colorBlending.blendConstants[3] = 1.0f;

				pipelineInfo.pColorBlendState = &colorBlending;

				// Input layout:
				VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
				vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				wi::vector<VkVertexInputBindingDescription> bindings;
				wi::vector<VkVertexInputAttributeDescription> attributes;
				if (pso->desc.il != nullptr)
				{
					uint32_t lastBinding = 0xFFFFFFFF;
					for (auto& x : pso->desc.il->elements)
					{
						if (x.input_slot == lastBinding)
							continue;
						lastBinding = x.input_slot;
						VkVertexInputBindingDescription& bind = bindings.emplace_back();
						bind.binding = x.input_slot;
						bind.inputRate = x.input_slot_class == InputClassification::PER_VERTEX_DATA ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
						bind.stride = GetFormatStride(x.format);
					}

					uint32_t offset = 0;
					uint32_t i = 0;
					lastBinding = 0xFFFFFFFF;
					for (auto& x : pso->desc.il->elements)
					{
						VkVertexInputAttributeDescription attr = {};
						attr.binding = x.input_slot;
						if (attr.binding != lastBinding)
						{
							lastBinding = attr.binding;
							offset = 0;
						}
						attr.format = _ConvertFormat(x.format);
						attr.location = i;
						attr.offset = x.aligned_byte_offset;
						if (attr.offset == InputLayout::APPEND_ALIGNED_ELEMENT)
						{
							// need to manually resolve this from the format spec.
							attr.offset = offset;
							offset += GetFormatStride(x.format);
						}

						attributes.push_back(attr);

						i++;
					}

					vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
					vertexInputInfo.pVertexBindingDescriptions = bindings.data();
					vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
					vertexInputInfo.pVertexAttributeDescriptions = attributes.data();
				}
				pipelineInfo.pVertexInputState = &vertexInputInfo;

				pipelineInfo.renderPass = VK_NULL_HANDLE; // instead we use VkPipelineRenderingCreateInfo

				VkPipelineRenderingCreateInfo renderingInfo = {};
				renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
				renderingInfo.viewMask = 0;
				renderingInfo.colorAttachmentCount = commandlist.renderpass_info.rt_count;
				VkFormat formats[8] = {};
				for (uint32_t i = 0; i < commandlist.renderpass_info.rt_count; ++i)
				{
					formats[i] = _ConvertFormat(commandlist.renderpass_info.rt_formats[i]);
				}
				renderingInfo.pColorAttachmentFormats = formats;
				renderingInfo.depthAttachmentFormat = _ConvertFormat(commandlist.renderpass_info.ds_format);
				if (IsFormatStencilSupport(commandlist.renderpass_info.ds_format))
				{
					renderingInfo.stencilAttachmentFormat = renderingInfo.depthAttachmentFormat;
				}
				pipelineInfo.pNext = &renderingInfo;

				VkResult res = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &pipeline);
				assert(res == VK_SUCCESS);

				commandlist.pipelines_worker.push_back(std::make_pair(pipeline_hash, pipeline));
			}
		}
		else
		{
			pipeline = it->second;
		}
		assert(pipeline != VK_NULL_HANDLE);

		vkCmdBindPipeline(commandlist.GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		commandlist.dirty_pso = false;
	}

	void GraphicsDevice_Vulkan::predraw(CommandList cmd)
	{
		pso_validate(cmd);

		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.binder.flush(true, cmd);
	}
	void GraphicsDevice_Vulkan::predispatch(CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.binder.flush(false, cmd);
	}

	// Engine functions
	GraphicsDevice_Vulkan::GraphicsDevice_Vulkan(wi::platform::window_type window, ValidationMode validationMode_, GPUPreference preference)
	{
		wi::Timer timer;
		capabilities |= GraphicsDeviceCapability::ALIASING_GENERIC;

		// This functionalty is missing from Vulkan but might be added in the future:
		//	Issue: https://github.com/KhronosGroup/Vulkan-Docs/issues/2079
		capabilities |= GraphicsDeviceCapability::COPY_BETWEEN_DIFFERENT_IMAGE_ASPECTS_NOT_SUPPORTED;

		wi::unordered_map<uint32_t, std::shared_ptr<std::mutex>> queue_lockers;

		TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = sizeof(VkAccelerationStructureInstanceKHR);

		validationMode = validationMode_;

		VkResult res;

		res = volkInitialize();
		assert(res == VK_SUCCESS);
		if (res != VK_SUCCESS)
		{
			wi::helper::messageBox("volkInitialize failed! ERROR: " + std::to_string(res), "Error!");
			wi::platform::Exit();
		}

		// Fill out application info:
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Wicked Engine Application";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Wicked Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(wi::version::GetMajor(), wi::version::GetMinor(), wi::version::GetRevision());
		appInfo.apiVersion = VK_API_VERSION_1_3;

		// Enumerate available layers and extensions:
		uint32_t instanceLayerCount;
		res = vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
		assert(res == VK_SUCCESS);
		wi::vector<VkLayerProperties> availableInstanceLayers(instanceLayerCount);
		res = vkEnumerateInstanceLayerProperties(&instanceLayerCount, availableInstanceLayers.data());
		assert(res == VK_SUCCESS);

		uint32_t extensionCount = 0;
		res = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		assert(res == VK_SUCCESS);
		wi::vector<VkExtensionProperties> availableInstanceExtensions(extensionCount);
		res = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableInstanceExtensions.data());
		assert(res == VK_SUCCESS);

		wi::vector<const char*> instanceLayers;
		wi::vector<const char*> instanceExtensions;

		for (auto& availableExtension : availableInstanceExtensions)
		{
			if (strcmp(availableExtension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
			{
				debugUtils = true;
				instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}
			else if (strcmp(availableExtension.extensionName, VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME) == 0)
			{
				instanceExtensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
			}
		}

		instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#if defined(VK_USE_PLATFORM_WIN32_KHR)
		instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif SDL2
		{
			uint32_t extensionCount;
			SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
			wi::vector<const char *> extensionNames_sdl(extensionCount);
			SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensionNames_sdl.data());
			instanceExtensions.reserve(instanceExtensions.size() + extensionNames_sdl.size());
			for (auto& x : extensionNames_sdl)
			{
				instanceExtensions.push_back(x);
			}
		}
#endif // _WIN32
		
		if (validationMode != ValidationMode::Disabled)
		{
			// Determine the optimal validation layers to enable that are necessary for useful debugging
			static const wi::vector<const char*> validationLayerPriorityList[] =
			{
				// The preferred validation layer is "VK_LAYER_KHRONOS_validation"
				{"VK_LAYER_KHRONOS_validation"},

				// Otherwise we fallback to using the LunarG meta layer
				{"VK_LAYER_LUNARG_standard_validation"},

				// Otherwise we attempt to enable the individual layers that compose the LunarG meta layer since it doesn't exist
				{
					"VK_LAYER_GOOGLE_threading",
					"VK_LAYER_LUNARG_parameter_validation",
					"VK_LAYER_LUNARG_object_tracker",
					"VK_LAYER_LUNARG_core_validation",
					"VK_LAYER_GOOGLE_unique_objects",
				},

				// Otherwise as a last resort we fallback to attempting to enable the LunarG core layer
				{"VK_LAYER_LUNARG_core_validation"}
			};

			for (auto& validationLayers : validationLayerPriorityList)
			{
				if (ValidateLayers(validationLayers, availableInstanceLayers))
				{
					for (auto& x : validationLayers)
					{
						instanceLayers.push_back(x);
					}
					break;
				}
			}
		}

		// Create instance:
		{
			VkInstanceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pApplicationInfo = &appInfo;
			createInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
			createInfo.ppEnabledLayerNames = instanceLayers.data();
			createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
			createInfo.ppEnabledExtensionNames = instanceExtensions.data();

			VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

			if (validationMode != ValidationMode::Disabled && debugUtils)
			{
				debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
				debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

				if (validationMode == ValidationMode::Verbose)
				{
					debugUtilsCreateInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
					debugUtilsCreateInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
				}

				debugUtilsCreateInfo.pfnUserCallback = debugUtilsMessengerCallback;
				createInfo.pNext = &debugUtilsCreateInfo;
			}

			res = vkCreateInstance(&createInfo, nullptr, &instance);
			assert(res == VK_SUCCESS);
			if (res != VK_SUCCESS)
			{
				wi::helper::messageBox("vkCreateInstance failed! ERROR: " + std::to_string(res), "Error!");
				wi::platform::Exit();
			}

			volkLoadInstanceOnly(instance);

			if (validationMode != ValidationMode::Disabled && debugUtils)
			{
				res = vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsCreateInfo, nullptr, &debugUtilsMessenger);
				assert(res == VK_SUCCESS);
			}
		}

		// Enumerating and creating devices:
		{
			uint32_t deviceCount = 0;
			VkResult res = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
			assert(res == VK_SUCCESS);
			if (deviceCount == 0)
			{
				assert(0);
				wi::helper::messageBox("Failed to find GPU with Vulkan support!");
				wi::platform::Exit();
			}

			wi::vector<VkPhysicalDevice> devices(deviceCount);
			res = vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
			assert(res == VK_SUCCESS);

			const wi::vector<const char*> required_deviceExtensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			};
			wi::vector<const char*> enabled_deviceExtensions;

			bool h264_decode_extension = false;
			bool suitable = false;


			auto checkPhysicalDeviceAndFillProperties2 = [&](VkPhysicalDevice dev) {
				suitable = true;

				uint32_t extensionCount;
				VkResult res = vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);
				assert(res == VK_SUCCESS);
				wi::vector<VkExtensionProperties> available_deviceExtensions(extensionCount);
				res = vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, available_deviceExtensions.data());
				assert(res == VK_SUCCESS);

				for (auto& x : required_deviceExtensions)
				{
					if (!checkExtensionSupport(x, available_deviceExtensions))
					{
						suitable = false;
					}
				}
				if (!suitable)
					return;

				h264_decode_extension = false;

				features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
				features_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
				features_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
				features_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
				features2.pNext = &features_1_1;
				features_1_1.pNext = &features_1_2;
				features_1_2.pNext = &features_1_3;
				void** features_chain = &features_1_3.pNext;
				acceleration_structure_features = {};
				raytracing_features = {};
				raytracing_query_features = {};
				fragment_shading_rate_features = {};
				mesh_shader_features = {};
				conditional_rendering_features = {};
				depth_clip_enable_features = {};

				properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
				properties_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
				properties_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
				properties_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
				properties2.pNext = &properties_1_1;
				properties_1_1.pNext = &properties_1_2;
				properties_1_2.pNext = &properties_1_3;
				void** properties_chain = &properties_1_3.pNext;
				sampler_minmax_properties = {};
				acceleration_structure_properties = {};
				raytracing_properties = {};
				fragment_shading_rate_properties = {};
				mesh_shader_properties = {};

				sampler_minmax_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES;
				*properties_chain = &sampler_minmax_properties;
				properties_chain = &sampler_minmax_properties.pNext;

				depth_stencil_resolve_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES;
				*properties_chain = &depth_stencil_resolve_properties;
				properties_chain = &depth_stencil_resolve_properties.pNext;

				enabled_deviceExtensions = required_deviceExtensions;

				if (checkExtensionSupport(VK_EXT_IMAGE_VIEW_MIN_LOD_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_EXT_IMAGE_VIEW_MIN_LOD_EXTENSION_NAME);
					image_view_min_lod_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_MIN_LOD_FEATURES_EXT;
					*features_chain = &image_view_min_lod_features;
					features_chain = &image_view_min_lod_features.pNext;
				}
				if (checkExtensionSupport(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME);
					depth_clip_enable_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
					*features_chain = &depth_clip_enable_features;
					features_chain = &depth_clip_enable_features.pNext;
				}
				if (checkExtensionSupport(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
					assert(checkExtensionSupport(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, available_deviceExtensions));
					enabled_deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
					acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
					*features_chain = &acceleration_structure_features;
					features_chain = &acceleration_structure_features.pNext;
					acceleration_structure_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
					*properties_chain = &acceleration_structure_properties;
					properties_chain = &acceleration_structure_properties.pNext;

					if (checkExtensionSupport(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, available_deviceExtensions))
					{
						enabled_deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
						enabled_deviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
						raytracing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
						*features_chain = &raytracing_features;
						features_chain = &raytracing_features.pNext;
						raytracing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
						*properties_chain = &raytracing_properties;
						properties_chain = &raytracing_properties.pNext;
					}

					if (checkExtensionSupport(VK_KHR_RAY_QUERY_EXTENSION_NAME, available_deviceExtensions))
					{
						enabled_deviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
						raytracing_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
						*features_chain = &raytracing_query_features;
						features_chain = &raytracing_query_features.pNext;
					}
				}

				if (checkExtensionSupport(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
					fragment_shading_rate_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
					*features_chain = &fragment_shading_rate_features;
					features_chain = &fragment_shading_rate_features.pNext;
					fragment_shading_rate_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
					*properties_chain = &fragment_shading_rate_properties;
					properties_chain = &fragment_shading_rate_properties.pNext;
				}

				if (checkExtensionSupport(VK_EXT_MESH_SHADER_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
					mesh_shader_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
					*features_chain = &mesh_shader_features;
					features_chain = &mesh_shader_features.pNext;
					mesh_shader_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;
					*properties_chain = &mesh_shader_properties;
					properties_chain = &mesh_shader_properties.pNext;
				}

				if (checkExtensionSupport(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME);
					conditional_rendering_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT;
					*features_chain = &conditional_rendering_features;
					features_chain = &conditional_rendering_features.pNext;
				}

				if (checkExtensionSupport(VK_KHR_VIDEO_QUEUE_EXTENSION_NAME, available_deviceExtensions) &&
					checkExtensionSupport(VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME, available_deviceExtensions) &&
					checkExtensionSupport(VK_KHR_VIDEO_DECODE_H264_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_KHR_VIDEO_QUEUE_EXTENSION_NAME);
					enabled_deviceExtensions.push_back(VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME);
					enabled_deviceExtensions.push_back(VK_KHR_VIDEO_DECODE_H264_EXTENSION_NAME);
					h264_decode_extension = true;
				}

#if defined(VK_USE_PLATFORM_WIN32_KHR)
				if (checkExtensionSupport(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME, available_deviceExtensions) &&
					checkExtensionSupport(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
					enabled_deviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
				}
#elif defined(__linux__)
				if (checkExtensionSupport(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME, available_deviceExtensions) &&
					checkExtensionSupport(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
					enabled_deviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
				}
#endif

				*properties_chain = nullptr;
				*features_chain = nullptr;
				vkGetPhysicalDeviceProperties2(dev, &properties2);

			};

			bool properties2_matches_physical_device = false;

			for (const auto& dev : devices)
			{
				properties2_matches_physical_device = false;
				checkPhysicalDeviceAndFillProperties2(dev);

				if (!suitable)
					continue;

				bool priority = properties2.properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
				if (preference == GPUPreference::Integrated)
				{
					priority = properties2.properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
				}
				if (priority || physicalDevice == VK_NULL_HANDLE)
				{
					physicalDevice = dev;
					properties2_matches_physical_device = true;
					if (priority)
					{
						break; // if this is prioritized GPU type, look no further
					}
				}
			}

			if (physicalDevice == VK_NULL_HANDLE)
			{
				assert(0);
				wi::helper::messageBox("Failed to find a suitable GPU!");
				wi::platform::Exit();
			}

			if (!properties2_matches_physical_device) {
				// this redoes a few checks, but since this code path is only
				// executed once, it doesn't affect execution time all that much
				checkPhysicalDeviceAndFillProperties2(physicalDevice);
			}

			assert(properties2.properties.limits.timestampComputeAndGraphics == VK_TRUE);

			vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);

			assert(features2.features.imageCubeArray == VK_TRUE);
			assert(features2.features.independentBlend == VK_TRUE);
			assert(features2.features.geometryShader == VK_TRUE);
			assert(features2.features.samplerAnisotropy == VK_TRUE);
			assert(features2.features.shaderClipDistance == VK_TRUE);
			assert(features2.features.textureCompressionBC == VK_TRUE);
			assert(features2.features.occlusionQueryPrecise == VK_TRUE);
			assert(features_1_2.descriptorIndexing == VK_TRUE);
			assert(features_1_3.dynamicRendering == VK_TRUE);

			// Init adapter properties
			vendorId = properties2.properties.vendorID;
			deviceId = properties2.properties.deviceID;
			adapterName = properties2.properties.deviceName;

			driverDescription = properties_1_2.driverName;
			if (properties_1_2.driverInfo[0] != '\0')
			{
				driverDescription += std::string(": ") + properties_1_2.driverInfo;
			}

			switch (properties2.properties.deviceType)
			{
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
				adapterType = AdapterType::IntegratedGpu;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
				adapterType = AdapterType::DiscreteGpu;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
				adapterType = AdapterType::VirtualGpu;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_CPU:
				adapterType = AdapterType::Cpu;
				break;
			default:
				adapterType = AdapterType::Other;
				break;
			}

			if (features2.features.tessellationShader == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability::TESSELLATION;
			}
			if (features2.features.shaderStorageImageExtendedFormats == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability::UAV_LOAD_FORMAT_COMMON;
			}
			if (features_1_2.shaderOutputLayer == VK_TRUE && features_1_2.shaderOutputViewportIndex)
			{
				capabilities |= GraphicsDeviceCapability::RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS;
			}

			if (
				raytracing_features.rayTracingPipeline == VK_TRUE &&
				raytracing_query_features.rayQuery == VK_TRUE &&
				acceleration_structure_features.accelerationStructure == VK_TRUE &&
				features_1_2.bufferDeviceAddress == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability::RAYTRACING;
				SHADER_IDENTIFIER_SIZE = raytracing_properties.shaderGroupHandleSize;
			}
			if (mesh_shader_features.meshShader == VK_TRUE && mesh_shader_features.taskShader == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability::MESH_SHADER;
			}
			if (fragment_shading_rate_features.pipelineFragmentShadingRate == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability::VARIABLE_RATE_SHADING;
			}
			if (fragment_shading_rate_features.attachmentFragmentShadingRate == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability::VARIABLE_RATE_SHADING_TIER2;
				VARIABLE_RATE_SHADING_TILE_SIZE = std::min(fragment_shading_rate_properties.maxFragmentShadingRateAttachmentTexelSize.width, fragment_shading_rate_properties.maxFragmentShadingRateAttachmentTexelSize.height);
			}

			VkFormatProperties formatProperties = {};
			vkGetPhysicalDeviceFormatProperties(physicalDevice, _ConvertFormat(Format::R11G11B10_FLOAT), &formatProperties);
			if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)
			{
				capabilities |= GraphicsDeviceCapability::UAV_LOAD_FORMAT_R11G11B10_FLOAT;
			}

			if (conditional_rendering_features.conditionalRendering == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability::PREDICATION;
			}

			if (features_1_2.samplerFilterMinmax == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability::SAMPLER_MINMAX;
			}

			if (features2.features.depthBounds == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability::DEPTH_BOUNDS_TEST;
			}

			if (features2.features.sparseBinding == VK_TRUE && features2.features.sparseResidencyAliased == VK_TRUE)
			{
				if (properties2.properties.sparseProperties.residencyNonResidentStrict == VK_TRUE)
				{
					capabilities |= GraphicsDeviceCapability::SPARSE_NULL_MAPPING;
				}
				if (features2.features.sparseResidencyBuffer == VK_TRUE)
				{
					capabilities |= GraphicsDeviceCapability::SPARSE_BUFFER;
				}
				if (features2.features.sparseResidencyImage2D == VK_TRUE)
				{
					capabilities |= GraphicsDeviceCapability::SPARSE_TEXTURE2D;
				}
				if (features2.features.sparseResidencyImage3D == VK_TRUE)
				{
					capabilities |= GraphicsDeviceCapability::SPARSE_TEXTURE3D;
				}
			}

			if (
				(depth_stencil_resolve_properties.supportedDepthResolveModes & VK_RESOLVE_MODE_MIN_BIT) &&
				(depth_stencil_resolve_properties.supportedDepthResolveModes & VK_RESOLVE_MODE_MAX_BIT)
				)
			{
				capabilities |= GraphicsDeviceCapability::DEPTH_RESOLVE_MIN_MAX;
			}
			if (
				(depth_stencil_resolve_properties.supportedStencilResolveModes & VK_RESOLVE_MODE_MIN_BIT) &&
				(depth_stencil_resolve_properties.supportedStencilResolveModes & VK_RESOLVE_MODE_MAX_BIT)
				)
			{
				capabilities |= GraphicsDeviceCapability::STENCIL_RESOLVE_MIN_MAX;
			}

			if(h264_decode_extension)
			{
				decode_h264_profile.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR;
				decode_h264_profile.stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH;
				decode_h264_profile.pictureLayout = VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_INTERLACED_INTERLEAVED_LINES_BIT_KHR;

				decode_h264_capabilities.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_CAPABILITIES_KHR;

				video_capability_h264.profile.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR;
				video_capability_h264.profile.pNext = &decode_h264_profile;
				video_capability_h264.profile.videoCodecOperation = VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR;
				video_capability_h264.profile.lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
				video_capability_h264.profile.chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
				video_capability_h264.profile.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;

				video_capability_h264.decode_capabilities.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR;

				video_capability_h264.video_capabilities.sType = VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR;
				video_capability_h264.video_capabilities.pNext = &video_capability_h264.decode_capabilities;
				video_capability_h264.decode_capabilities.pNext = &decode_h264_capabilities;
				res = vkGetPhysicalDeviceVideoCapabilitiesKHR(physicalDevice, &video_capability_h264.profile, &video_capability_h264.video_capabilities);
				assert(res == VK_SUCCESS);

				if (video_capability_h264.decode_capabilities.flags)
				{
					capabilities |= GraphicsDeviceCapability::VIDEO_DECODE_H264;
					VIDEO_DECODE_BITSTREAM_ALIGNMENT = std::max(VIDEO_DECODE_BITSTREAM_ALIGNMENT, video_capability_h264.video_capabilities.minBitstreamBufferOffsetAlignment);
					VIDEO_DECODE_BITSTREAM_ALIGNMENT = std::max(VIDEO_DECODE_BITSTREAM_ALIGNMENT, video_capability_h264.video_capabilities.minBitstreamBufferSizeAlignment);
				}
			}

			// Find queue families:
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyCount, nullptr);

			queueFamilies.resize(queueFamilyCount);
			queueFamiliesVideo.resize(queueFamilyCount);
			for (uint32_t i = 0; i < queueFamilyCount; ++i)
			{
				queueFamilies[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
				queueFamilies[i].pNext = &queueFamiliesVideo[i];
				queueFamiliesVideo[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_VIDEO_PROPERTIES_KHR;
			}
			vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyCount, queueFamilies.data());
			
			// Query base queue families:
			for (uint32_t i = 0; i < queueFamilyCount; ++i)
			{
				auto& queueFamily = queueFamilies[i];
				auto& queueFamilyVideo = queueFamiliesVideo[i];

				if (graphicsFamily == VK_QUEUE_FAMILY_IGNORED && queueFamily.queueFamilyProperties.queueCount > 0 && queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					graphicsFamily = i;
					if (queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
					{
						queues[QUEUE_GRAPHICS].sparse_binding_supported = true;
					}
				}

				if (copyFamily == VK_QUEUE_FAMILY_IGNORED && queueFamily.queueFamilyProperties.queueCount > 0 && queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)
				{
					copyFamily = i;
				}

				if (computeFamily == VK_QUEUE_FAMILY_IGNORED && queueFamily.queueFamilyProperties.queueCount > 0 && queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
				{
					computeFamily = i;
				}

				if (videoFamily == VK_QUEUE_FAMILY_IGNORED &&
					queueFamily.queueFamilyProperties.queueCount > 0 &&
					(queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) &&
					(queueFamilyVideo.videoCodecOperations & VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR)
					)
				{
					videoFamily = i;
				}
			}

			// Now try to find dedicated compute and transfer queues:
			for (uint32_t i = 0; i < queueFamilyCount; ++i)
			{
				auto& queueFamily = queueFamilies[i];

				if (queueFamily.queueFamilyProperties.queueCount > 0 &&
					queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT &&
					!(queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
					!(queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
					)
				{
					copyFamily = i;

					if (queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
					{
						queues[QUEUE_COPY].sparse_binding_supported = true;
					}
				}

				if (queueFamily.queueFamilyProperties.queueCount > 0 &&
					queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT &&
					!(queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					)
				{
					computeFamily = i;

					if (queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
					{
						queues[QUEUE_COMPUTE].sparse_binding_supported = true;
					}
				}
			}

			// Now try to find dedicated transfer queue with only transfer and sparse flags:
			//	(This is a workaround for a driver bug with sparse updating from transfer queue)
			for (uint32_t i = 0; i < queueFamilyCount; ++i)
			{
				auto& queueFamily = queueFamilies[i];

				if (queueFamily.queueFamilyProperties.queueCount > 0 && queueFamily.queueFamilyProperties.queueFlags == (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT))
				{
					copyFamily = i;
					queues[QUEUE_COPY].sparse_binding_supported = true;
				}
			}

			wi::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			wi::unordered_set<uint32_t> uniqueQueueFamilies = { graphicsFamily,copyFamily,computeFamily };
			if (videoFamily != VK_QUEUE_FAMILY_IGNORED)
			{
				uniqueQueueFamilies.insert(videoFamily);
			}

			float queuePriority = 1.0f;
			for (uint32_t queueFamily : uniqueQueueFamilies)
			{
				queue_lockers.emplace(queueFamily, std::make_shared<std::mutex>());
				VkDeviceQueueCreateInfo queueCreateInfo = {};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueFamily;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueCreateInfo);
				families.push_back(queueFamily);
			}

			VkDeviceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
			createInfo.pQueueCreateInfos = queueCreateInfos.data();
			createInfo.pEnabledFeatures = nullptr;
			createInfo.pNext = &features2;
			createInfo.enabledExtensionCount = static_cast<uint32_t>(enabled_deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = enabled_deviceExtensions.data();

			res = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
			assert(res == VK_SUCCESS);
			if (res != VK_SUCCESS)
			{
				wi::helper::messageBox("vkCreateDevice failed! ERROR: " + std::to_string(res), "Error!");
				wi::platform::Exit();
			}

			volkLoadDevice(device);
		}

		// queues:
		{
			vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
			vkGetDeviceQueue(device, computeFamily, 0, &computeQueue);
			vkGetDeviceQueue(device, copyFamily, 0, &copyQueue);
			if (videoFamily != VK_QUEUE_FAMILY_IGNORED)
			{
				vkGetDeviceQueue(device, videoFamily, 0, &videoQueue);
			}

			queues[QUEUE_GRAPHICS].queue = graphicsQueue;
			queues[QUEUE_GRAPHICS].locker = queue_lockers[graphicsFamily];
			queues[QUEUE_COMPUTE].queue = computeQueue;
			queues[QUEUE_COMPUTE].locker = queue_lockers[computeFamily];
			queues[QUEUE_COPY].queue = copyQueue;
			queues[QUEUE_COPY].locker = queue_lockers[copyFamily];
			queues[QUEUE_VIDEO_DECODE].queue = videoQueue;
			if (videoFamily != VK_QUEUE_FAMILY_IGNORED) {
			    queues[QUEUE_VIDEO_DECODE].locker = queue_lockers[videoFamily];
			}

		}

		memory_properties_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
		vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &memory_properties_2);

		if (memory_properties_2.memoryProperties.memoryHeapCount == 1 &&
			memory_properties_2.memoryProperties.memoryHeaps[0].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
		{
			// https://registry.khronos.org/vulkan/specs/1.0-extensions/html/vkspec.html#memory-device
			//	"In a unified memory architecture (UMA) system there is often only a single memory heap which is
			//	considered to be equally “local” to the host and to the device, and such an implementation must advertise the heap as device-local."
			capabilities |= GraphicsDeviceCapability::CACHE_COHERENT_UMA;
		}

		allocationhandler = std::make_shared<AllocationHandler>();
		allocationhandler->device = device;
		allocationhandler->instance = instance;

		// Initialize Vulkan Memory Allocator helper:
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.device = device;
		allocatorInfo.instance = instance;

		// Core in 1.1
		allocatorInfo.flags =
			VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
			VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;

		if (features_1_2.bufferDeviceAddress)
		{
			allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		}

#if VMA_DYNAMIC_VULKAN_FUNCTIONS
		static VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
		allocatorInfo.pVulkanFunctions = &vulkanFunctions;
#endif
		res = vmaCreateAllocator(&allocatorInfo, &allocationhandler->allocator);
		assert(res == VK_SUCCESS);
		if (res != VK_SUCCESS)
		{
			wi::helper::messageBox("vmaCreateAllocator failed! ERROR: " + std::to_string(res), "Error!");
			wi::platform::Exit();
		}

		std::vector<VkExternalMemoryHandleTypeFlags> externalMemoryHandleTypes;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		externalMemoryHandleTypes.resize(memory_properties_2.memoryProperties.memoryTypeCount, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT);
		allocatorInfo.pTypeExternalMemoryHandleTypes = externalMemoryHandleTypes.data();
#elif defined(__linux__)
		externalMemoryHandleTypes.resize(memory_properties_2.memoryProperties.memoryTypeCount, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);
		allocatorInfo.pTypeExternalMemoryHandleTypes = externalMemoryHandleTypes.data();
#endif

		res = vmaCreateAllocator(&allocatorInfo, &allocationhandler->externalAllocator);
		assert(res == VK_SUCCESS);
		if (res != VK_SUCCESS)
		{
			wi::helper::messageBox("Failed to create Vulkan external memory allocator, ERROR: " + std::to_string(res), "Error!");
			wi::platform::Exit();
		}

		copyAllocator.init(this);

		// Create frame resources:
		for (uint32_t fr = 0; fr < BUFFERCOUNT; ++fr)
		{
			for (int queue = 0; queue < QUEUE_COUNT; ++queue)
			{
				if (queues[queue].queue == VK_NULL_HANDLE)
					continue;

				VkFenceCreateInfo fenceInfo = {};
				fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				//fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
				VkResult res = vkCreateFence(device, &fenceInfo, nullptr, &frame_fence[fr][queue]);
				assert(res == VK_SUCCESS);
				if (res != VK_SUCCESS)
				{
					wi::helper::messageBox("vkCreateFence[FRAME] failed! ERROR: " + std::to_string(res), "Error!");
					wi::platform::Exit();
				}
			}
		}

		// Create default null descriptors:
		{
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = 4;
			bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			bufferInfo.flags = 0;


			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

			res = vmaCreateBuffer(allocationhandler->allocator, &bufferInfo, &allocInfo, &nullBuffer, &nullBufferAllocation, nullptr);
			assert(res == VK_SUCCESS);
			
			VkBufferViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
			viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
			viewInfo.range = VK_WHOLE_SIZE;
			viewInfo.buffer = nullBuffer;
			res = vkCreateBufferView(device, &viewInfo, nullptr, &nullBufferView);
			assert(res == VK_SUCCESS);
		}
		{
			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.extent.width = 1;
			imageInfo.extent.height = 1;
			imageInfo.extent.depth = 1;
			imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
			imageInfo.arrayLayers = 1;
			imageInfo.mipLevels = 1;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
			imageInfo.flags = 0;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

			imageInfo.imageType = VK_IMAGE_TYPE_1D;
			res = vmaCreateImage(allocationhandler->allocator, &imageInfo, &allocInfo, &nullImage1D, &nullImageAllocation1D, nullptr);
			assert(res == VK_SUCCESS);

			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			imageInfo.arrayLayers = 6;
			res = vmaCreateImage(allocationhandler->allocator, &imageInfo, &allocInfo, &nullImage2D, &nullImageAllocation2D, nullptr);
			assert(res == VK_SUCCESS);

			imageInfo.imageType = VK_IMAGE_TYPE_3D;
			imageInfo.flags = 0;
			imageInfo.arrayLayers = 1;
			res = vmaCreateImage(allocationhandler->allocator, &imageInfo, &allocInfo, &nullImage3D, &nullImageAllocation3D, nullptr);
			assert(res == VK_SUCCESS);


			// Transitions:
			{
				CopyAllocator::CopyCMD cmd = copyAllocator.allocate(0);
				VkImageMemoryBarrier2 barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				barrier.oldLayout = imageInfo.initialLayout;
				barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				barrier.srcAccessMask = 0;
				barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
				barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = nullImage1D;
				barrier.subresourceRange.layerCount = 1;

				VkDependencyInfo dependencyInfo = {};
				dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
				dependencyInfo.imageMemoryBarrierCount = 1;
				dependencyInfo.pImageMemoryBarriers = &barrier;

				vkCmdPipelineBarrier2(cmd.transitionCommandBuffer, &dependencyInfo);

				barrier.image = nullImage2D;
				barrier.subresourceRange.layerCount = 6;
				vkCmdPipelineBarrier2(cmd.transitionCommandBuffer, &dependencyInfo);

				barrier.image = nullImage3D;
				barrier.subresourceRange.layerCount = 1;
				vkCmdPipelineBarrier2(cmd.transitionCommandBuffer, &dependencyInfo);

				copyAllocator.submit(cmd);
			}

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;

			viewInfo.image = nullImage1D;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_1D;
			res = vkCreateImageView(device, &viewInfo, nullptr, &nullImageView1D);
			assert(res == VK_SUCCESS);

			viewInfo.image = nullImage1D;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
			res = vkCreateImageView(device, &viewInfo, nullptr, &nullImageView1DArray);
			assert(res == VK_SUCCESS);

			viewInfo.image = nullImage2D;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			res = vkCreateImageView(device, &viewInfo, nullptr, &nullImageView2D);
			assert(res == VK_SUCCESS);

			viewInfo.image = nullImage2D;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			res = vkCreateImageView(device, &viewInfo, nullptr, &nullImageView2DArray);
			assert(res == VK_SUCCESS);

			viewInfo.image = nullImage2D;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			viewInfo.subresourceRange.layerCount = 6;
			res = vkCreateImageView(device, &viewInfo, nullptr, &nullImageViewCube);
			assert(res == VK_SUCCESS);

			viewInfo.image = nullImage2D;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			viewInfo.subresourceRange.layerCount = 6;
			res = vkCreateImageView(device, &viewInfo, nullptr, &nullImageViewCubeArray);
			assert(res == VK_SUCCESS);

			viewInfo.image = nullImage3D;
			viewInfo.subresourceRange.layerCount = 1;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
			res = vkCreateImageView(device, &viewInfo, nullptr, &nullImageView3D);
			assert(res == VK_SUCCESS);
		}
		{
			VkSamplerCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

			res = vkCreateSampler(device, &createInfo, nullptr, &nullSampler);
			assert(res == VK_SUCCESS);
		}

		TIMESTAMP_FREQUENCY = uint64_t(1.0 / double(properties2.properties.limits.timestampPeriod) * 1000 * 1000 * 1000);

		// Dynamic PSO states:
		pso_dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT);
		pso_dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT);
		pso_dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
		pso_dynamicStates.push_back(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
		if (CheckCapability(GraphicsDeviceCapability::DEPTH_BOUNDS_TEST))
		{
			pso_dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BOUNDS);
		}
		if (CheckCapability(GraphicsDeviceCapability::VARIABLE_RATE_SHADING))
		{
			pso_dynamicStates.push_back(VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR);
		}
		pso_dynamicStates.push_back(VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE);

		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.pDynamicStates = pso_dynamicStates.data();
		dynamicStateInfo.dynamicStateCount = (uint32_t)pso_dynamicStates.size();

		dynamicStateInfo_MeshShader.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo_MeshShader.pDynamicStates = pso_dynamicStates.data();
		dynamicStateInfo_MeshShader.dynamicStateCount = (uint32_t)pso_dynamicStates.size() - 1; // don't include VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE for mesh shader

		// Note: limiting descriptors by constant amount is needed, because the bindless sets are bound to multiple slots to match DX12 layout
		//	And binding to multiple slot adds up towards limits, so the limits will be quickly reached for some descriptor types
		//	But not all descriptor types have this problem, like storage buffers that are not bound for multiple slots usually
		//	Ideally, this shouldn't be the case, because Vulkan could have it's own layout in shaders
		const uint32_t limit_bindless_descriptors = 100000u;

		if (features_1_2.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE)
		{
			allocationhandler->bindlessSampledImages.init(device, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, std::min(limit_bindless_descriptors, properties_1_2.maxDescriptorSetUpdateAfterBindSampledImages / 4));
		}
		if (features_1_2.descriptorBindingUniformTexelBufferUpdateAfterBind == VK_TRUE)
		{
			allocationhandler->bindlessUniformTexelBuffers.init(device, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, std::min(limit_bindless_descriptors, properties_1_2.maxDescriptorSetUpdateAfterBindSampledImages / 4));
		}
		if (features_1_2.descriptorBindingStorageBufferUpdateAfterBind == VK_TRUE)
		{
			allocationhandler->bindlessStorageBuffers.init(device, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, properties_1_2.maxDescriptorSetUpdateAfterBindStorageBuffers / 4);
		}
		if (features_1_2.descriptorBindingStorageImageUpdateAfterBind == VK_TRUE)
		{
			allocationhandler->bindlessStorageImages.init(device, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, std::min(limit_bindless_descriptors, properties_1_2.maxDescriptorSetUpdateAfterBindStorageImages / 4));
		}
		if (features_1_2.descriptorBindingStorageTexelBufferUpdateAfterBind == VK_TRUE)
		{
			allocationhandler->bindlessStorageTexelBuffers.init(device, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, std::min(limit_bindless_descriptors, properties_1_2.maxDescriptorSetUpdateAfterBindStorageImages / 4));
		}
		if (features_1_2.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE)
		{
			allocationhandler->bindlessSamplers.init(device, VK_DESCRIPTOR_TYPE_SAMPLER, 256);
		}

		if (CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			allocationhandler->bindlessAccelerationStructures.init(device, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 32);
		}

		// Pipeline Cache
		{
			// Try to read pipeline cache file if exists.
			wi::vector<uint8_t> pipelineData;

			std::string cachePath = GetCachePath(); 
			if (!wi::helper::FileRead(cachePath, pipelineData))
			{
				pipelineData.clear();
			}

			// Verify cache validation.
			if (!pipelineData.empty())
			{
				uint32_t headerLength = 0;
				uint32_t cacheHeaderVersion = 0;
				uint32_t vendorID = 0;
				uint32_t deviceID = 0;
				uint8_t pipelineCacheUUID[VK_UUID_SIZE] = {};

				std::memcpy(&headerLength, (uint8_t*)pipelineData.data() + 0, 4);
				std::memcpy(&cacheHeaderVersion, (uint8_t*)pipelineData.data() + 4, 4);
				std::memcpy(&vendorID, (uint8_t*)pipelineData.data() + 8, 4);
				std::memcpy(&deviceID, (uint8_t*)pipelineData.data() + 12, 4);
				std::memcpy(pipelineCacheUUID, (uint8_t*)pipelineData.data() + 16, VK_UUID_SIZE);

				bool badCache = false;

				if (headerLength <= 0)
				{
					badCache = true;
				}

				if (cacheHeaderVersion != VK_PIPELINE_CACHE_HEADER_VERSION_ONE)
				{
					badCache = true;
				}

				if (vendorID != properties2.properties.vendorID)
				{
					badCache = true;
				}

				if (deviceID != properties2.properties.deviceID)
				{
					badCache = true;
				}

				if (memcmp(pipelineCacheUUID, properties2.properties.pipelineCacheUUID, sizeof(pipelineCacheUUID)) != 0)
				{
					badCache = true;
				}

				if (badCache)
				{
					// Don't submit initial cache data if any version info is incorrect
					pipelineData.clear();
				}
			}

			VkPipelineCacheCreateInfo createInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
			createInfo.initialDataSize = pipelineData.size();
			createInfo.pInitialData = pipelineData.data();

			// Create Vulkan pipeline cache
			res = vkCreatePipelineCache(device, &createInfo, nullptr, &pipelineCache);
			assert(res == VK_SUCCESS);
		}

		// Static samplers:
		{
			VkSamplerCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.compareEnable = false;
			createInfo.compareOp = VK_COMPARE_OP_NEVER;
			createInfo.minLod = 0;
			createInfo.maxLod = FLT_MAX;
			createInfo.mipLodBias = 0;
			createInfo.anisotropyEnable = false;
			createInfo.maxAnisotropy = 0;

			// sampler_linear_clamp:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			res = vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers.emplace_back());
			assert(res == VK_SUCCESS);

			// sampler_linear_wrap:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			res = vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers.emplace_back());
			assert(res == VK_SUCCESS);

			//sampler_linear_mirror:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			res = vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers.emplace_back());
			assert(res == VK_SUCCESS);

			// sampler_point_clamp:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			res = vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers.emplace_back());
			assert(res == VK_SUCCESS);

			// sampler_point_wrap:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			res = vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers.emplace_back());
			assert(res == VK_SUCCESS);

			// sampler_point_mirror:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			res = vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers.emplace_back());
			assert(res == VK_SUCCESS);

			// sampler_aniso_clamp:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.anisotropyEnable = true;
			createInfo.maxAnisotropy = 16;
			res = vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers.emplace_back());
			assert(res == VK_SUCCESS);

			// sampler_aniso_wrap:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.anisotropyEnable = true;
			createInfo.maxAnisotropy = 16;
			res = vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers.emplace_back());
			assert(res == VK_SUCCESS);

			// sampler_aniso_mirror:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			createInfo.anisotropyEnable = true;
			createInfo.maxAnisotropy = 16;
			res = vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers.emplace_back());
			assert(res == VK_SUCCESS);

			// sampler_cmp_depth:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.anisotropyEnable = false;
			createInfo.maxAnisotropy = 0;
			createInfo.compareEnable = true;
			createInfo.compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
			createInfo.minLod = 0;
			createInfo.maxLod = 0;
			res = vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers.emplace_back());
			assert(res == VK_SUCCESS);
		}

		wi::backlog::post("Created GraphicsDevice_Vulkan (" + std::to_string((int)std::round(timer.elapsed())) + " ms)\nAdapter: " + adapterName);
	}
	GraphicsDevice_Vulkan::~GraphicsDevice_Vulkan()
	{
		VkResult res = vkDeviceWaitIdle(device);
		assert(res == VK_SUCCESS);

		for (uint32_t fr = 0; fr < BUFFERCOUNT; ++fr)
		{
			for (int queue = 0; queue < QUEUE_COUNT; ++queue)
			{
				vkDestroyFence(device, frame_fence[fr][queue], nullptr);
			}
		}

		copyAllocator.destroy();

		for (auto& x : pso_layout_cache)
		{
			vkDestroyPipelineLayout(device, x.second.pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, x.second.descriptorSetLayout, nullptr);
		}

		for (auto& commandlist : commandlists)
		{
			for (int buffer = 0; buffer < BUFFERCOUNT; ++buffer)
			{
				for (int queue = 0; queue < QUEUE_COUNT; ++queue)
				{
					vkDestroyCommandPool(device, commandlist->commandPools[buffer][queue], nullptr);
				}
			}
			for (auto& x : commandlist->pipelines_worker)
			{
				vkDestroyPipeline(device, x.second, nullptr);
			}
			for (auto& x : commandlist->binder_pools)
			{
				x.destroy();
			}
		}
		for (auto& x : pipelines_global)
		{
			vkDestroyPipeline(device, x.second, nullptr);
		}

		for (auto& x : semaphore_pool)
		{
			vkDestroySemaphore(device, x, nullptr);
		}

		vmaDestroyBuffer(allocationhandler->allocator, nullBuffer, nullBufferAllocation);
		vkDestroyBufferView(device, nullBufferView, nullptr);
		vmaDestroyImage(allocationhandler->allocator, nullImage1D, nullImageAllocation1D);
		vmaDestroyImage(allocationhandler->allocator, nullImage2D, nullImageAllocation2D);
		vmaDestroyImage(allocationhandler->allocator, nullImage3D, nullImageAllocation3D);
		vkDestroyImageView(device, nullImageView1D, nullptr);
		vkDestroyImageView(device, nullImageView1DArray, nullptr);
		vkDestroyImageView(device, nullImageView2D, nullptr);
		vkDestroyImageView(device, nullImageView2DArray, nullptr);
		vkDestroyImageView(device, nullImageViewCube, nullptr);
		vkDestroyImageView(device, nullImageViewCubeArray, nullptr);
		vkDestroyImageView(device, nullImageView3D, nullptr);
		vkDestroySampler(device, nullSampler, nullptr);

		for (VkSampler sam : immutable_samplers)
		{
			vkDestroySampler(device, sam, nullptr);
		}

		if (pipelineCache != VK_NULL_HANDLE)
		{
			// Get size of pipeline cache
			size_t size{};
			res = vkGetPipelineCacheData(device, pipelineCache, &size, nullptr);
			assert(res == VK_SUCCESS);

			// Get data of pipeline cache 
			wi::vector<uint8_t> data(size);
			res = vkGetPipelineCacheData(device, pipelineCache, &size, data.data());
			assert(res == VK_SUCCESS);

			// Write pipeline cache data to a file in binary format
			std::string cachePath = GetCachePath();
			wi::helper::FileWrite(cachePath, data.data(), size);

			// Destroy Vulkan pipeline cache 
			vkDestroyPipelineCache(device, pipelineCache, nullptr);
			pipelineCache = VK_NULL_HANDLE;
		}

		if (debugUtilsMessenger != VK_NULL_HANDLE)
		{
			vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
		}
	}

	bool GraphicsDevice_Vulkan::CreateSwapChain(const SwapChainDesc* desc, wi::platform::window_type window, SwapChain* swapchain) const
	{
		auto internal_state = std::static_pointer_cast<SwapChain_Vulkan>(swapchain->internal_state);
		if (swapchain->internal_state == nullptr)
		{
			internal_state = std::make_shared<SwapChain_Vulkan>();
		}
		internal_state->allocationhandler = allocationhandler;
		internal_state->desc = *desc;
		swapchain->internal_state = internal_state;
		swapchain->desc = *desc;

		VkResult res;

		// Surface creation:
		if(internal_state->surface == VK_NULL_HANDLE)
		{
#ifdef _WIN32
			VkWin32SurfaceCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			createInfo.hwnd = window;
			createInfo.hinstance = GetModuleHandle(nullptr);

			res = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &internal_state->surface);
			assert(res == VK_SUCCESS);
#elif SDL2
			if (!SDL_Vulkan_CreateSurface(window, instance, &internal_state->surface))
			{
				throw sdl2::SDLError("Error creating a vulkan surface");
			}
#else
#error WICKEDENGINE VULKAN DEVICE ERROR: PLATFORM NOT SUPPORTED
#endif // _WIN32
		}

		uint32_t presentFamily = VK_QUEUE_FAMILY_IGNORED;
		uint32_t familyIndex = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			VkBool32 presentSupport = false;
			res = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, (uint32_t)familyIndex, internal_state->surface, &presentSupport);
			assert(res == VK_SUCCESS);

			if (presentFamily == VK_QUEUE_FAMILY_IGNORED && queueFamily.queueFamilyProperties.queueCount > 0 && presentSupport)
			{
				presentFamily = familyIndex;
				break;
			}

			familyIndex++;
		}

		// Present family not found, we cannot create SwapChain
		if (presentFamily == VK_QUEUE_FAMILY_IGNORED)
		{
			return false;
		}

		bool success = CreateSwapChainInternal(internal_state.get(), physicalDevice, device, allocationhandler);
		assert(success);

		// The requested swapchain format is overridden if it wasn't supported:
		swapchain->desc.format = internal_state->desc.format;

		return success;
	}
	bool GraphicsDevice_Vulkan::CreateBuffer2(const GPUBufferDesc* desc, const std::function<void(void*)>& init_callback, GPUBuffer* buffer, const GPUResource* alias, uint64_t alias_offset) const
	{
		auto internal_state = std::make_shared<Buffer_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		buffer->internal_state = internal_state;
		buffer->type = GPUResource::Type::BUFFER;
		buffer->mapped_data = nullptr;
		buffer->mapped_size = 0;
		buffer->desc = *desc;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = buffer->desc.size;
		bufferInfo.usage = 0;
		if (has_flag(buffer->desc.bind_flags, BindFlag::VERTEX_BUFFER))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}
		if (has_flag(buffer->desc.bind_flags, BindFlag::INDEX_BUFFER))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}
		if (has_flag(buffer->desc.bind_flags, BindFlag::CONSTANT_BUFFER))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		}
		if (has_flag(buffer->desc.bind_flags, BindFlag::SHADER_RESOURCE))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; // read only ByteAddressBuffer is also storage buffer
			bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
		}
		if (has_flag(buffer->desc.bind_flags, BindFlag::UNORDERED_ACCESS))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
		}
		if (has_flag(buffer->desc.misc_flags, ResourceMiscFlag::BUFFER_RAW))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}
		if (has_flag(buffer->desc.misc_flags, ResourceMiscFlag::BUFFER_STRUCTURED))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}
		if (has_flag(buffer->desc.misc_flags, ResourceMiscFlag::INDIRECT_ARGS))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		}
		if (has_flag(buffer->desc.misc_flags, ResourceMiscFlag::RAY_TRACING))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
			bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
		}
		if (has_flag(buffer->desc.misc_flags, ResourceMiscFlag::PREDICATION))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT;
		}
		if (features_1_2.bufferDeviceAddress == VK_TRUE)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		}
		if (has_flag(buffer->desc.misc_flags, ResourceMiscFlag::VIDEO_DECODE))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR;
		}
		bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		VkVideoProfileListInfoKHR profile_list_info = {};
		profile_list_info.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR;
		profile_list_info.pProfiles = &video_capability_h264.profile;
		profile_list_info.profileCount = 1;
		if ((bufferInfo.usage & VK_BUFFER_USAGE_VIDEO_DECODE_DST_BIT_KHR) ||
			(bufferInfo.usage & VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR)
			)
		{
			bufferInfo.pNext = &profile_list_info;
		}

		bufferInfo.flags = 0;

		if (families.size() > 1)
		{
			bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
			bufferInfo.queueFamilyIndexCount = (uint32_t)families.size();
			bufferInfo.pQueueFamilyIndices = families.data();
		}
		else
		{
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		VkResult res;

		if (has_flag(desc->misc_flags, ResourceMiscFlag::ALIASING_BUFFER) ||
			has_flag(desc->misc_flags, ResourceMiscFlag::ALIASING_TEXTURE_NON_RT_DS) ||
			has_flag(desc->misc_flags, ResourceMiscFlag::ALIASING_TEXTURE_RT_DS))
		{
			VkMemoryRequirements memory_requirements = {};
			memory_requirements.alignment = desc->alignment;
			memory_requirements.size = AlignTo(desc->size, memory_requirements.alignment);
			memory_requirements.memoryTypeBits = ~0u;

			VmaAllocationCreateInfo create_info = {};
			create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

			VmaAllocationInfo allocation_info = {};

			res = vmaAllocateMemory(
				allocationhandler->allocator,
				&memory_requirements,
				&create_info,
				&internal_state->allocation,
				&allocation_info
			);
			assert(res == VK_SUCCESS);

			res = vkCreateBuffer(
				device,
				&bufferInfo,
				nullptr,
				&internal_state->resource
			);
			assert(res == VK_SUCCESS);

			res = vkBindBufferMemory(
				device,
				internal_state->resource,
				internal_state->allocation->GetMemory(),
				internal_state->allocation->GetOffset()
			);
			assert(res == VK_SUCCESS);
		}
		else if (has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE))
		{
			assert(CheckCapability(GraphicsDeviceCapability::SPARSE_BUFFER));
			bufferInfo.flags |= VK_BUFFER_CREATE_SPARSE_BINDING_BIT;
			bufferInfo.flags |= VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT;
			bufferInfo.flags |= VK_BUFFER_CREATE_SPARSE_ALIASED_BIT;

			res = vkCreateBuffer(device, &bufferInfo, nullptr, &internal_state->resource);
			assert(res == VK_SUCCESS);

			VkMemoryRequirements memory_requirements = {};
			vkGetBufferMemoryRequirements(
				device,
				internal_state->resource,
				&memory_requirements
			);
			buffer->sparse_page_size = memory_requirements.alignment;
		}
		else
		{
			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			if (desc->usage == Usage::READBACK)
			{
				bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			}
			else if (desc->usage == Usage::UPLOAD)
			{
				bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			}

			if (alias == nullptr)
			{
				res = vmaCreateBuffer(
					allocationhandler->allocator,
					&bufferInfo,
					&allocInfo,
					&internal_state->resource,
					&internal_state->allocation,
					nullptr
				);
			}
			else
			{
				// Aliasing: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/resource_aliasing.html
				if (alias->IsTexture())
				{
					auto alias_internal = to_internal((const Texture*)alias);
					res = vmaCreateAliasingBuffer2(
						allocationhandler->allocator,
						alias_internal->allocation,
						alias_offset,
						&bufferInfo,
						&internal_state->resource
					);
				}
				else
				{
					auto alias_internal = to_internal((const GPUBuffer*)alias);
					res = vmaCreateAliasingBuffer2(
						allocationhandler->allocator,
						alias_internal->allocation,
						alias_offset,
						&bufferInfo,
						&internal_state->resource
					);
				}
			}
			assert(res == VK_SUCCESS);
		}

		if (desc->usage == Usage::READBACK || desc->usage == Usage::UPLOAD)
		{
			buffer->mapped_data = internal_state->allocation->GetMappedData();
			buffer->mapped_size = internal_state->allocation->GetSize();
		}

		if (bufferInfo.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			VkBufferDeviceAddressInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			info.buffer = internal_state->resource;
			internal_state->address = vkGetBufferDeviceAddress(device, &info);
		}

		// Issue data copy on request:
		if (init_callback != nullptr)
		{
			CopyAllocator::CopyCMD cmd;
			void* mapped_data = nullptr;
			if (desc->usage == Usage::UPLOAD)
			{
				mapped_data = buffer->mapped_data;
			}
			else
			{
				cmd = copyAllocator.allocate(desc->size);
				mapped_data = cmd.uploadbuffer.mapped_data;
			}

			init_callback(mapped_data);

			if(cmd.IsValid())
			{
				VkBufferCopy copyRegion = {};
				copyRegion.size = buffer->desc.size;
				copyRegion.srcOffset = 0;
				copyRegion.dstOffset = 0;

				vkCmdCopyBuffer(
					cmd.transferCommandBuffer,
					to_internal(&cmd.uploadbuffer)->resource,
					internal_state->resource,
					1,
					&copyRegion
				);

				VkBufferMemoryBarrier2 barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
				barrier.buffer = internal_state->resource;
				barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
				barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
				barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
				barrier.size = VK_WHOLE_SIZE;

				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				if (has_flag(buffer->desc.bind_flags, BindFlag::CONSTANT_BUFFER))
				{
					barrier.dstAccessMask |= VK_ACCESS_2_UNIFORM_READ_BIT;
				}
				if (has_flag(buffer->desc.bind_flags, BindFlag::VERTEX_BUFFER))
				{
					barrier.dstStageMask |= VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
					barrier.dstAccessMask |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
				}
				if (has_flag(buffer->desc.bind_flags, BindFlag::INDEX_BUFFER))
				{
					barrier.dstStageMask |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
					barrier.dstAccessMask |= VK_ACCESS_2_INDEX_READ_BIT;
				}
				if (has_flag(buffer->desc.bind_flags, BindFlag::SHADER_RESOURCE))
				{
					barrier.dstAccessMask |= VK_ACCESS_2_SHADER_READ_BIT;
				}
				if (has_flag(buffer->desc.bind_flags, BindFlag::UNORDERED_ACCESS))
				{
					barrier.dstAccessMask |= VK_ACCESS_2_SHADER_READ_BIT;
					barrier.dstAccessMask |= VK_ACCESS_2_SHADER_WRITE_BIT;
				}
				if (has_flag(buffer->desc.misc_flags, ResourceMiscFlag::INDIRECT_ARGS))
				{
					barrier.dstAccessMask |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
				}
				if (has_flag(buffer->desc.misc_flags, ResourceMiscFlag::RAY_TRACING))
				{
					barrier.dstAccessMask |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
				}
				if (has_flag(buffer->desc.misc_flags, ResourceMiscFlag::VIDEO_DECODE))
				{
					barrier.dstAccessMask |= VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR;
				}

				VkDependencyInfo dependencyInfo = {};
				dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
				dependencyInfo.bufferMemoryBarrierCount = 1;
				dependencyInfo.pBufferMemoryBarriers = &barrier;

				vkCmdPipelineBarrier2(cmd.transitionCommandBuffer, &dependencyInfo);
				
				copyAllocator.submit(cmd);
			}
		}

		// Create resource views if needed
		if (!has_flag(desc->misc_flags, ResourceMiscFlag::NO_DEFAULT_DESCRIPTORS))
		{
			if (has_flag(desc->bind_flags, BindFlag::SHADER_RESOURCE))
			{
				CreateSubresource(buffer, SubresourceType::SRV, 0);
			}
			if (has_flag(desc->bind_flags, BindFlag::UNORDERED_ACCESS))
			{
				CreateSubresource(buffer, SubresourceType::UAV, 0);
			}
		}

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateTexture(const TextureDesc* desc, const SubresourceData* initial_data, Texture* texture, const GPUResource* alias, uint64_t alias_offset) const
	{
		auto internal_state = std::make_shared<Texture_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		internal_state->defaultLayout = _ConvertImageLayout(desc->layout);
		texture->internal_state = internal_state;
		texture->type = GPUResource::Type::TEXTURE;
		texture->mapped_data = nullptr;
		texture->mapped_size = 0;
		texture->mapped_subresources = nullptr;
		texture->mapped_subresource_count = 0;
		texture->sparse_properties = nullptr;
		texture->desc = *desc;

		if (texture->desc.mip_levels == 0)
		{
			texture->desc.mip_levels = GetMipCount(texture->desc.width, texture->desc.height, texture->desc.depth);
		}

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.extent.width = texture->desc.width;
		imageInfo.extent.height = texture->desc.height;
		imageInfo.extent.depth = texture->desc.depth;
		imageInfo.format = _ConvertFormat(texture->desc.format);
		imageInfo.arrayLayers = texture->desc.array_size;
		imageInfo.mipLevels = texture->desc.mip_levels;
		imageInfo.samples = (VkSampleCountFlagBits)texture->desc.sample_count;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = 0;
		if (has_flag(texture->desc.bind_flags, BindFlag::SHADER_RESOURCE))
		{
			imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		if (has_flag(texture->desc.bind_flags, BindFlag::UNORDERED_ACCESS))
		{
			imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

			if (IsFormatSRGB(texture->desc.format))
			{
				imageInfo.flags |= VK_IMAGE_CREATE_EXTENDED_USAGE_BIT;
			}
		}
		if (has_flag(texture->desc.bind_flags, BindFlag::RENDER_TARGET))
		{
			imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
		if (has_flag(texture->desc.bind_flags, BindFlag::DEPTH_STENCIL))
		{
			imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}
		if (has_flag(texture->desc.bind_flags, BindFlag::SHADING_RATE))
		{
			imageInfo.usage |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
		}
		if (has_flag(texture->desc.misc_flags, ResourceMiscFlag::TRANSIENT_ATTACHMENT))
		{
			imageInfo.usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
		}
		else
		{
			imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		imageInfo.flags = 0;
		if (has_flag(texture->desc.misc_flags, ResourceMiscFlag::TEXTURECUBE))
		{
			imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		}
		if (has_flag(texture->desc.misc_flags, ResourceMiscFlag::TYPED_FORMAT_CASTING) || has_flag(texture->desc.misc_flags, ResourceMiscFlag::TYPELESS_FORMAT_CASTING))
		{
			imageInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
		}
		if (has_flag(texture->desc.misc_flags, ResourceMiscFlag::VIDEO_DECODE))
		{
			imageInfo.usage |= VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR;
			imageInfo.usage |= VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR;
			imageInfo.usage |= VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;
		}

		if (desc->format == Format::NV12 && has_flag(texture->desc.bind_flags, BindFlag::SHADER_RESOURCE))
		{
			imageInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
		}

		VkVideoProfileListInfoKHR profile_list_info = {};
		profile_list_info.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR;
		profile_list_info.pProfiles = &video_capability_h264.profile;
		profile_list_info.profileCount = 1;
		if ((imageInfo.usage & VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR) ||
			(imageInfo.usage & VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR) ||
			(imageInfo.usage & VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR)
			)
		{
			imageInfo.pNext = &profile_list_info;

			VkPhysicalDeviceVideoFormatInfoKHR video_format_info = {};
			video_format_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VIDEO_FORMAT_INFO_KHR;
			video_format_info.imageUsage = imageInfo.usage;
			video_format_info.pNext = &profile_list_info;
			uint32_t format_property_count = 0;
			VkResult res = vkGetPhysicalDeviceVideoFormatPropertiesKHR(physicalDevice, &video_format_info, &format_property_count, nullptr);
			assert(res == VK_SUCCESS);

			wi::vector<VkVideoFormatPropertiesKHR> video_format_properties(format_property_count);
			for (auto& x : video_format_properties)
			{
				x.sType = VK_STRUCTURE_TYPE_VIDEO_FORMAT_PROPERTIES_KHR;
			}
			res = vkGetPhysicalDeviceVideoFormatPropertiesKHR(physicalDevice, &video_format_info, &format_property_count, video_format_properties.data());
			assert(res == VK_SUCCESS);

			//assert(imageInfo.flags == 0 || (!video_format_properties.empty() && video_format_properties[0].imageCreateFlags & imageInfo.flags));
			//assert(!video_format_properties.empty() && video_format_properties[0].imageUsageFlags & imageInfo.usage);
		}

		if (families.size() > 1)
		{
			imageInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
			imageInfo.queueFamilyIndexCount = (uint32_t)families.size();
			imageInfo.pQueueFamilyIndices = families.data();
		}
		else
		{
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		switch (texture->desc.type)
		{
		case TextureDesc::Type::TEXTURE_1D:
			imageInfo.imageType = VK_IMAGE_TYPE_1D;
			break;
		case TextureDesc::Type::TEXTURE_2D:
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			break;
		case TextureDesc::Type::TEXTURE_3D:
			imageInfo.imageType = VK_IMAGE_TYPE_3D;
			break;
		default:
			assert(0);
			break;
		}

		VkResult res;

		if (has_flag(texture->desc.misc_flags, ResourceMiscFlag::SPARSE))
		{
			assert(CheckCapability(GraphicsDeviceCapability::SPARSE_TEXTURE2D) || imageInfo.imageType != VK_IMAGE_TYPE_2D);
			assert(CheckCapability(GraphicsDeviceCapability::SPARSE_TEXTURE3D) || imageInfo.imageType != VK_IMAGE_TYPE_3D);
			imageInfo.flags |= VK_IMAGE_CREATE_SPARSE_BINDING_BIT;
			imageInfo.flags |= VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;
			imageInfo.flags |= VK_IMAGE_CREATE_SPARSE_ALIASED_BIT;

			res = vkCreateImage(device, &imageInfo, nullptr, &internal_state->resource);
			assert(res == VK_SUCCESS);

			VkMemoryRequirements memory_requirements = {};
			vkGetImageMemoryRequirements(
				device,
				internal_state->resource,
				&memory_requirements
			);
			texture->sparse_page_size = memory_requirements.alignment;

			uint32_t sparse_requirement_count = 0;

			vkGetImageSparseMemoryRequirements(
				device,
				internal_state->resource,
				&sparse_requirement_count,
				nullptr
			);

			wi::vector<VkSparseImageMemoryRequirements> sparse_requirements(sparse_requirement_count);
			texture->sparse_properties = &internal_state->sparse_texture_properties;

			vkGetImageSparseMemoryRequirements(
				device,
				internal_state->resource,
				&sparse_requirement_count,
				sparse_requirements.data()
			);

			SparseTextureProperties& out_sparse = internal_state->sparse_texture_properties;
			out_sparse.total_tile_count = uint32_t(memory_requirements.size / memory_requirements.alignment);

			for (size_t i = 0; i < sparse_requirements.size(); ++i)
			{
				const VkSparseImageMemoryRequirements& in_sparse = sparse_requirements[i];
				if (i == 0)
				{
					// These should be common for all subresources right? Like in DX12?
					out_sparse.tile_width = in_sparse.formatProperties.imageGranularity.width;
					out_sparse.tile_height = in_sparse.formatProperties.imageGranularity.height;
					out_sparse.tile_depth = in_sparse.formatProperties.imageGranularity.depth;
					out_sparse.packed_mip_start = in_sparse.imageMipTailFirstLod;
					out_sparse.packed_mip_count = texture->desc.mip_levels - in_sparse.imageMipTailFirstLod;
					out_sparse.packed_mip_tile_offset = uint32_t(in_sparse.imageMipTailOffset / memory_requirements.alignment);
					out_sparse.packed_mip_tile_count = uint32_t(in_sparse.imageMipTailSize / memory_requirements.alignment);
				}
			}

		}
		else
		{
			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

			if (texture->desc.usage == Usage::READBACK || texture->desc.usage == Usage::UPLOAD)
			{
				// Note: we are creating a buffer instead of linear image because linear image cannot have mips
				//	With a buffer, we can tightly pack mips linearly into a buffer so it won't have that limitation
				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = 0;
				const uint32_t data_stride = GetFormatStride(texture->desc.format);
				const uint32_t block_size = GetFormatBlockSize(texture->desc.format);
				const uint32_t num_blocks_x = texture->desc.width / block_size;
				const uint32_t num_blocks_y = texture->desc.height / block_size;
				uint32_t mip_width = num_blocks_x;
				uint32_t mip_height = num_blocks_y;
				uint32_t mip_depth = texture->desc.depth;
				for (uint32_t mip = 0; mip < texture->desc.mip_levels; ++mip)
				{
					bufferInfo.size += mip_width * mip_height * mip_depth;
					mip_width = std::max(1u, mip_width / 2);
					mip_height = std::max(1u, mip_height / 2);
					mip_depth = std::max(1u, mip_depth / 2);
				}
				bufferInfo.size *= imageInfo.arrayLayers;
				bufferInfo.size *= data_stride;

				allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
				if (texture->desc.usage == Usage::READBACK)
				{
					allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
					bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				}
				else if (texture->desc.usage == Usage::UPLOAD)
				{
					allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
					bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				}

				res = vmaCreateBuffer(allocationhandler->allocator, &bufferInfo, &allocInfo, &internal_state->staging_resource, &internal_state->allocation, nullptr);
				assert(res == VK_SUCCESS);

				if (texture->desc.usage == Usage::READBACK || texture->desc.usage == Usage::UPLOAD)
				{
					texture->mapped_data = internal_state->allocation->GetMappedData();
					texture->mapped_size = internal_state->allocation->GetSize();

					internal_state->mapped_subresources.resize(texture->desc.array_size * texture->desc.mip_levels);
					size_t subresourceIndex = 0;
					size_t subresourceDataOffset = 0;
					for (uint32_t layer = 0; layer < texture->desc.array_size; ++layer)
					{
						uint32_t rowpitch = (uint32_t)num_blocks_x * GetFormatStride(desc->format); // the buffer is tightly packed, no padding in row pitch
						uint32_t slicepitch = (uint32_t)rowpitch * num_blocks_y;
						uint32_t mip_width = num_blocks_x;
						for (uint32_t mip = 0; mip < texture->desc.mip_levels; ++mip)
						{
							SubresourceData& subresourcedata = internal_state->mapped_subresources[subresourceIndex++];
							subresourcedata.data_ptr = (uint8_t*)texture->mapped_data + subresourceDataOffset;
							subresourcedata.row_pitch = rowpitch;
							subresourcedata.slice_pitch = slicepitch;
							subresourceDataOffset += std::max(rowpitch * mip_width, slicepitch);
							rowpitch = std::max(data_stride, rowpitch / 2);
							slicepitch = std::max(data_stride, slicepitch / 4); // squared reduction
							mip_width = std::max(1u, mip_width / 2);
						}
					}
					texture->mapped_subresources = internal_state->mapped_subresources.data();
					texture->mapped_subresource_count = internal_state->mapped_subresources.size();
				}
			}
			else
			{
				VmaAllocator allocator = allocationhandler->allocator;
				VkExternalMemoryImageCreateInfo externalMemImageCreateInfo = {};

				if (has_flag(texture->desc.misc_flags, ResourceMiscFlag::SHARED))
				{
					externalMemImageCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
					// TODO: Expose this? should we use VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT?
					externalMemImageCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
					externalMemImageCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
					imageInfo.pNext = &externalMemImageCreateInfo;

					// We have to use a dedicated allocator for external handles that has been created with VkExportMemoryAllocateInfo
					allocator = allocationhandler->externalAllocator;

					allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
				}

				if (alias == nullptr)
				{
					res = vmaCreateImage(
						allocator,
						&imageInfo,
						&allocInfo,
						&internal_state->resource,
						&internal_state->allocation,
						nullptr
					);
				}
				else
				{
					// Aliasing: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/resource_aliasing.html
					if (alias->IsTexture())
					{
						auto alias_internal = to_internal((const Texture*)alias);
						res = vmaCreateAliasingImage2(
							allocator,
							alias_internal->allocation,
							alias_offset,
							&imageInfo,
							&internal_state->resource
						);
					}
					else
					{
						auto alias_internal = to_internal((const GPUBuffer*)alias);
						res = vmaCreateAliasingImage2(
							allocator,
							alias_internal->allocation,
							alias_offset,
							&imageInfo,
							&internal_state->resource
						);
					}
				}
				assert(res == VK_SUCCESS);

				if (has_flag(texture->desc.misc_flags, ResourceMiscFlag::SHARED))
				{
					VmaAllocationInfo allocationInfo;
					vmaGetAllocationInfo(allocator, internal_state->allocation, &allocationInfo);

#if defined(VK_USE_PLATFORM_WIN32_KHR)
					VkMemoryGetWin32HandleInfoKHR getWin32HandleInfoKHR = {};
					getWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
					getWin32HandleInfoKHR.pNext = nullptr;
					getWin32HandleInfoKHR.memory = allocationInfo.deviceMemory;
					getWin32HandleInfoKHR.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
					res = vkGetMemoryWin32HandleKHR(allocationhandler->device, &getWin32HandleInfoKHR, &texture->shared_handle);
					assert(res == VK_SUCCESS);
#elif defined(__linux__)
					VkMemoryGetFdInfoKHR memoryGetFdInfoKHR = {};
					memoryGetFdInfoKHR.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
					memoryGetFdInfoKHR.pNext = nullptr;
					memoryGetFdInfoKHR.memory = allocationInfo.deviceMemory;
					memoryGetFdInfoKHR.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
					res = vkGetMemoryFdKHR(allocationhandler->device, &memoryGetFdInfoKHR, &texture->shared_handle);
					assert(res == VK_SUCCESS);
#endif
				}
			}
		}

		// Issue data copy on request:
		if (initial_data != nullptr)
		{
			CopyAllocator::CopyCMD cmd;
			void* mapped_data = nullptr;
			if (desc->usage == Usage::UPLOAD)
			{
				mapped_data = texture->mapped_data;
			}
			else
			{
				cmd = copyAllocator.allocate(internal_state->allocation->GetSize());
				mapped_data = cmd.uploadbuffer.mapped_data;
			}

			wi::vector<VkBufferImageCopy> copyRegions;

			VkDeviceSize copyOffset = 0;
			uint32_t initDataIdx = 0;
			for (uint32_t layer = 0; layer < desc->array_size; ++layer)
			{
				uint32_t width = imageInfo.extent.width;
				uint32_t height = imageInfo.extent.height;
				uint32_t depth = imageInfo.extent.depth;
				for (uint32_t mip = 0; mip < desc->mip_levels; ++mip)
				{
					const SubresourceData& subresourceData = initial_data[initDataIdx++];
					const uint32_t block_size = GetFormatBlockSize(desc->format);
					const uint32_t num_blocks_x = std::max(1u, width / block_size);
					const uint32_t num_blocks_y = std::max(1u, height / block_size);
					const uint32_t dst_rowpitch = num_blocks_x * GetFormatStride(desc->format);
					const uint32_t dst_slicepitch = dst_rowpitch * num_blocks_y;
					const uint32_t src_rowpitch = subresourceData.row_pitch;
					const uint32_t src_slicepitch = subresourceData.slice_pitch;
					for (uint32_t z = 0; z < depth; ++z)
					{
						uint8_t* dst_slice = (uint8_t*)mapped_data + copyOffset + dst_slicepitch * z;
						uint8_t* src_slice = (uint8_t*)subresourceData.data_ptr + src_slicepitch * z;
						for (uint32_t y = 0; y < num_blocks_y; ++y)
						{
							std::memcpy(
								dst_slice + dst_rowpitch * y,
								src_slice + src_rowpitch * y,
								dst_rowpitch
							);
						}
					}

					if (cmd.IsValid())
					{
						VkBufferImageCopy copyRegion = {};
						copyRegion.bufferOffset = copyOffset;
						copyRegion.bufferRowLength = 0;
						copyRegion.bufferImageHeight = 0;

						copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						copyRegion.imageSubresource.mipLevel = mip;
						copyRegion.imageSubresource.baseArrayLayer = layer;
						copyRegion.imageSubresource.layerCount = 1;

						copyRegion.imageOffset = { 0, 0, 0 };
						copyRegion.imageExtent = {
							width,
							height,
							depth
						};

						copyRegions.push_back(copyRegion);
					}

					copyOffset += dst_slicepitch * depth;
					copyOffset = AlignTo(copyOffset, VkDeviceSize(4)); // fix for validation: on transfer queue the srcOffset must be 4-byte aligned

					width = std::max(1u, width / 2);
					height = std::max(1u, height / 2);
					depth = std::max(1u, depth / 2);
				}
			}

			if(cmd.IsValid())
			{
				VkImageMemoryBarrier2 barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				barrier.image = internal_state->resource;
				barrier.oldLayout = imageInfo.initialLayout;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
				barrier.srcAccessMask = 0;
				barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				VkDependencyInfo dependencyInfo = {};
				dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
				dependencyInfo.imageMemoryBarrierCount = 1;
				dependencyInfo.pImageMemoryBarriers = &barrier;

				vkCmdPipelineBarrier2(cmd.transferCommandBuffer, &dependencyInfo);

				vkCmdCopyBufferToImage(
					cmd.transferCommandBuffer,
					to_internal(&cmd.uploadbuffer)->resource,
					internal_state->resource,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					(uint32_t)copyRegions.size(),
					copyRegions.data()
				);

				std::swap(barrier.srcStageMask, barrier.dstStageMask);
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = _ConvertImageLayout(texture->desc.layout);
				barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = _ParseResourceState(texture->desc.layout);
				vkCmdPipelineBarrier2(cmd.transitionCommandBuffer, &dependencyInfo);

				copyAllocator.submit(cmd);
			}
		}
		else if(texture->desc.layout != ResourceState::UNDEFINED && internal_state->resource != VK_NULL_HANDLE)
		{
			VkImageMemoryBarrier2 barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			barrier.image = internal_state->resource;
			barrier.oldLayout = imageInfo.initialLayout;
			barrier.newLayout = _ConvertImageLayout(texture->desc.layout);
			barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			barrier.srcAccessMask = 0;
			barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			barrier.dstAccessMask = _ParseResourceState(texture->desc.layout);
			if (IsFormatDepthSupport(texture->desc.format))
			{
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				if (IsFormatStencilSupport(texture->desc.format))
				{
					barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
				}
			}
			else
			{
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = imageInfo.arrayLayers;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = imageInfo.mipLevels;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			CopyAllocator::CopyCMD cmd = copyAllocator.allocate(0);

			VkDependencyInfo dependencyInfo = {};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.imageMemoryBarrierCount = 1;
			dependencyInfo.pImageMemoryBarriers = &barrier;

			vkCmdPipelineBarrier2(cmd.transitionCommandBuffer, &dependencyInfo);
			copyAllocator.submit(cmd);
		}

		if (!has_flag(desc->misc_flags, ResourceMiscFlag::NO_DEFAULT_DESCRIPTORS))
		{
			if (has_flag(texture->desc.bind_flags, BindFlag::RENDER_TARGET))
			{
				CreateSubresource(texture, SubresourceType::RTV, 0, -1, 0, -1);
			}
			if (has_flag(texture->desc.bind_flags, BindFlag::DEPTH_STENCIL))
			{
				CreateSubresource(texture, SubresourceType::DSV, 0, -1, 0, -1);
			}
			if (has_flag(texture->desc.bind_flags, BindFlag::SHADER_RESOURCE))
			{
				CreateSubresource(texture, SubresourceType::SRV, 0, -1, 0, -1);
			}
			if (has_flag(texture->desc.bind_flags, BindFlag::UNORDERED_ACCESS))
			{
				CreateSubresource(texture, SubresourceType::UAV, 0, -1, 0, -1);
			}
		}

		if (has_flag(texture->desc.misc_flags, ResourceMiscFlag::VIDEO_DECODE))
		{
			VkImageViewCreateInfo view_desc = {};
			view_desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view_desc.flags = 0;
			view_desc.image = internal_state->resource;
			view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			view_desc.subresourceRange.baseArrayLayer = 0;
			view_desc.subresourceRange.layerCount = imageInfo.arrayLayers;
			view_desc.subresourceRange.baseMipLevel = 0;
			view_desc.subresourceRange.levelCount = imageInfo.mipLevels;
			view_desc.format = imageInfo.format;
			view_desc.viewType = imageInfo.arrayLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;

			VkImageViewUsageCreateInfo viewUsageInfo = {};
			viewUsageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
			viewUsageInfo.usage = VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR | VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR | VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;
			view_desc.pNext = &viewUsageInfo;

			res = vkCreateImageView(device, &view_desc, nullptr, &internal_state->video_decode_view);
			assert(res == VK_SUCCESS);
		}

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateShader(ShaderStage stage, const void* shadercode, size_t shadercode_size, Shader* shader) const
	{
		auto internal_state = std::make_shared<Shader_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		shader->internal_state = internal_state;
		shader->stage = stage;

		VkResult res = VK_SUCCESS;

		VkShaderModuleCreateInfo moduleInfo = {};
		moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleInfo.codeSize = shadercode_size;
		moduleInfo.pCode = (const uint32_t*)shadercode;
		res = vkCreateShaderModule(device, &moduleInfo, nullptr, &internal_state->shaderModule);
		assert(res == VK_SUCCESS);

		internal_state->stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		internal_state->stageInfo.module = internal_state->shaderModule;
		internal_state->stageInfo.pName = "main";
		switch (stage)
		{
		case ShaderStage::MS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_MESH_BIT_EXT;
			break;
		case ShaderStage::AS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_TASK_BIT_EXT;
			break;
		case ShaderStage::VS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			break;
		case ShaderStage::HS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			break;
		case ShaderStage::DS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			break;
		case ShaderStage::GS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			break;
		case ShaderStage::PS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			break;
		case ShaderStage::CS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			break;
		default:
			// also means library shader (ray tracing)
			internal_state->stageInfo.stage = VK_SHADER_STAGE_ALL;
			break;
		}

		{
			SpvReflectShaderModule module;
			SpvReflectResult result = spvReflectCreateShaderModule(moduleInfo.codeSize, moduleInfo.pCode, &module);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			uint32_t binding_count = 0;
			result = spvReflectEnumerateDescriptorBindings(
				&module, &binding_count, nullptr
			);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			wi::vector<SpvReflectDescriptorBinding*> bindings(binding_count);
			result = spvReflectEnumerateDescriptorBindings(
				&module, &binding_count, bindings.data()
			);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			uint32_t push_count = 0;
			result = spvReflectEnumeratePushConstantBlocks(&module, &push_count, nullptr);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			wi::vector<SpvReflectBlockVariable*> pushconstants(push_count);
			result = spvReflectEnumeratePushConstantBlocks(&module, &push_count, pushconstants.data());
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			for (auto& x : pushconstants)
			{
				auto& push = internal_state->pushconstants;
				push.stageFlags = internal_state->stageInfo.stage;
				push.offset = x->offset;
				push.size = x->size;
			}

			for (auto& x : bindings)
			{
				// This has some issues atm with some specific shader, recheck in the future with different compiler version
				//if (x->accessed == 0)
				//	continue;
				const bool bindless = x->set > 0;

				if (bindless)
				{
					// There can be padding between bindless spaces because sets need to be bound contiguously
					internal_state->bindlessBindings.resize(std::max(internal_state->bindlessBindings.size(), (size_t)x->set));
					internal_state->bindlessBindings[x->set - 1].used = true;
				}

				auto& descriptor = bindless ? internal_state->bindlessBindings[x->set - 1].binding : internal_state->layoutBindings.emplace_back();
				descriptor.stageFlags = internal_state->stageInfo.stage;
				descriptor.binding = x->binding;
				descriptor.descriptorCount = x->count;
				descriptor.descriptorType = (VkDescriptorType)x->descriptor_type;

				if (bindless)
					continue;

				auto& imageViewType = internal_state->imageViewTypes.emplace_back();
				imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;

				if (x->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER && x->binding >= VULKAN_BINDING_SHIFT_S + immutable_sampler_slot_begin)
				{
					descriptor.pImmutableSamplers = immutable_samplers.data() + x->binding - VULKAN_BINDING_SHIFT_S - immutable_sampler_slot_begin;
					continue;
				}

				if (descriptor.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
				{
					// For now, always replace VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER with VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
					//	It would be quite messy to track which buffer is dynamic and which is not in the binding code, consider multiple pipeline bind points too
					//	But maybe the dynamic uniform buffer is not always best because it occupies more registers (like DX12 root descriptor)?
					descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
					for (uint32_t i = 0; i < descriptor.descriptorCount; ++i)
					{
						internal_state->uniform_buffer_sizes[descriptor.binding + i] = x->block.size;
						internal_state->uniform_buffer_dynamic_slots.push_back(descriptor.binding + i);
					}
				}

				switch (x->descriptor_type)
				{
				default:
					break;
				case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					switch (x->image.dim)
					{
					default:
					case SpvDim1D:
						if (x->image.arrayed == 0)
						{
							imageViewType = VK_IMAGE_VIEW_TYPE_1D;
						}
						else
						{
							imageViewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
						}
						break;
					case SpvDim2D:
						if (x->image.arrayed == 0)
						{
							imageViewType = VK_IMAGE_VIEW_TYPE_2D;
						}
						else
						{
							imageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
						}
						break;
					case SpvDim3D:
						imageViewType = VK_IMAGE_VIEW_TYPE_3D;
						break;
					case SpvDimCube:
						if (x->image.arrayed == 0)
						{
							imageViewType = VK_IMAGE_VIEW_TYPE_CUBE;
						}
						else
						{
							imageViewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
						}
						break;
					}
					break;
				}
			}

			spvReflectDestroyShaderModule(&module);

			if (stage == ShaderStage::CS || stage == ShaderStage::LIB)
			{
				internal_state->binding_hash = 0;
				size_t i = 0;
				for (auto& x : internal_state->layoutBindings)
				{
					wi::helper::hash_combine(internal_state->binding_hash, x.binding);
					wi::helper::hash_combine(internal_state->binding_hash, x.descriptorCount);
					wi::helper::hash_combine(internal_state->binding_hash, x.descriptorType);
					wi::helper::hash_combine(internal_state->binding_hash, x.stageFlags);
					wi::helper::hash_combine(internal_state->binding_hash, internal_state->imageViewTypes[i++]);
				}
				for (auto& x : internal_state->bindlessBindings)
				{
					wi::helper::hash_combine(internal_state->binding_hash, x.used);
					wi::helper::hash_combine(internal_state->binding_hash, x.binding.binding);
					wi::helper::hash_combine(internal_state->binding_hash, x.binding.descriptorCount);
					wi::helper::hash_combine(internal_state->binding_hash, x.binding.descriptorType);
					wi::helper::hash_combine(internal_state->binding_hash, x.binding.stageFlags);
				}
				wi::helper::hash_combine(internal_state->binding_hash, internal_state->pushconstants.offset);
				wi::helper::hash_combine(internal_state->binding_hash, internal_state->pushconstants.size);
				wi::helper::hash_combine(internal_state->binding_hash, internal_state->pushconstants.stageFlags);

				pso_layout_cache_mutex.lock();
				if (pso_layout_cache[internal_state->binding_hash].pipelineLayout == VK_NULL_HANDLE)
				{
					wi::vector<VkDescriptorSetLayout> layouts;

					{
						VkDescriptorSetLayoutCreateInfo descriptorSetlayoutInfo = {};
						descriptorSetlayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
						descriptorSetlayoutInfo.pBindings = internal_state->layoutBindings.data();
						descriptorSetlayoutInfo.bindingCount = uint32_t(internal_state->layoutBindings.size());
						res = vkCreateDescriptorSetLayout(device, &descriptorSetlayoutInfo, nullptr, &internal_state->descriptorSetLayout);
						assert(res == VK_SUCCESS);
						layouts.push_back(internal_state->descriptorSetLayout);
					}

					internal_state->bindlessFirstSet = (uint32_t)layouts.size();
					for (auto& x : internal_state->bindlessBindings)
					{
						switch (x.binding.descriptorType)
						{
						case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
							assert(0); // not supported, use the raw buffers for same functionality
							break;
						case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
							layouts.push_back(allocationhandler->bindlessSampledImages.descriptorSetLayout);
							internal_state->bindlessSets.push_back(allocationhandler->bindlessSampledImages.descriptorSet);
							break;
						case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
							layouts.push_back(allocationhandler->bindlessUniformTexelBuffers.descriptorSetLayout);
							internal_state->bindlessSets.push_back(allocationhandler->bindlessUniformTexelBuffers.descriptorSet);
							break;
						case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
							layouts.push_back(allocationhandler->bindlessStorageBuffers.descriptorSetLayout);
							internal_state->bindlessSets.push_back(allocationhandler->bindlessStorageBuffers.descriptorSet);
							break;
						case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
							layouts.push_back(allocationhandler->bindlessStorageImages.descriptorSetLayout);
							internal_state->bindlessSets.push_back(allocationhandler->bindlessStorageImages.descriptorSet);
							break;
						case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
							layouts.push_back(allocationhandler->bindlessStorageTexelBuffers.descriptorSetLayout);
							internal_state->bindlessSets.push_back(allocationhandler->bindlessStorageTexelBuffers.descriptorSet);
							break;
						case VK_DESCRIPTOR_TYPE_SAMPLER:
							layouts.push_back(allocationhandler->bindlessSamplers.descriptorSetLayout);
							internal_state->bindlessSets.push_back(allocationhandler->bindlessSamplers.descriptorSet);
							break;
						case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
							layouts.push_back(allocationhandler->bindlessAccelerationStructures.descriptorSetLayout);
							internal_state->bindlessSets.push_back(allocationhandler->bindlessAccelerationStructures.descriptorSet);
							break;
						default:
							break;
						}
					}

					VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
					pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
					pipelineLayoutInfo.pSetLayouts = layouts.data();
					pipelineLayoutInfo.setLayoutCount = (uint32_t)layouts.size();
					if (internal_state->pushconstants.size > 0)
					{
						pipelineLayoutInfo.pushConstantRangeCount = 1;
						pipelineLayoutInfo.pPushConstantRanges = &internal_state->pushconstants;
					}
					else
					{
						pipelineLayoutInfo.pushConstantRangeCount = 0;
						pipelineLayoutInfo.pPushConstantRanges = nullptr;
					}

					res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &internal_state->pipelineLayout_cs);
					assert(res == VK_SUCCESS);
					if (res == VK_SUCCESS)
					{
						pso_layout_cache[internal_state->binding_hash].descriptorSetLayout = internal_state->descriptorSetLayout;
						pso_layout_cache[internal_state->binding_hash].pipelineLayout = internal_state->pipelineLayout_cs;
						pso_layout_cache[internal_state->binding_hash].bindlessSets = internal_state->bindlessSets;
						pso_layout_cache[internal_state->binding_hash].bindlessFirstSet = internal_state->bindlessFirstSet;
					}
				}
				else
				{
					internal_state->descriptorSetLayout = pso_layout_cache[internal_state->binding_hash].descriptorSetLayout;
					internal_state->pipelineLayout_cs = pso_layout_cache[internal_state->binding_hash].pipelineLayout;
					internal_state->bindlessSets = pso_layout_cache[internal_state->binding_hash].bindlessSets;
					internal_state->bindlessFirstSet = pso_layout_cache[internal_state->binding_hash].bindlessFirstSet;
				}
				pso_layout_cache_mutex.unlock();
			}
		}

		if (stage == ShaderStage::CS)
		{
			// sort because dynamic offsets array is tightly packed to match slot numbers:
			std::sort(internal_state->uniform_buffer_dynamic_slots.begin(), internal_state->uniform_buffer_dynamic_slots.end());

			VkComputePipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipelineInfo.layout = internal_state->pipelineLayout_cs;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

			// Create compute pipeline state in place:
			pipelineInfo.stage = internal_state->stageInfo;

			res = vkCreateComputePipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &internal_state->pipeline_cs);
			assert(res == VK_SUCCESS);
		}

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateSampler(const SamplerDesc* desc, Sampler* sampler) const
	{
		auto internal_state = std::make_shared<Sampler_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		sampler->internal_state = internal_state;
		sampler->desc = *desc;

		VkSamplerCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.flags = 0;
		createInfo.pNext = nullptr;

		switch (desc->filter)
		{
		case Filter::MIN_MAG_MIP_POINT:
		case Filter::MINIMUM_MIN_MAG_MIP_POINT:
		case Filter::MAXIMUM_MIN_MAG_MIP_POINT:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case Filter::MIN_MAG_POINT_MIP_LINEAR:
		case Filter::MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
		case Filter::MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case Filter::MIN_POINT_MAG_LINEAR_MIP_POINT:
		case Filter::MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
		case Filter::MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case Filter::MIN_POINT_MAG_MIP_LINEAR:
		case Filter::MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
		case Filter::MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case Filter::MIN_LINEAR_MAG_MIP_POINT:
		case Filter::MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
		case Filter::MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case Filter::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		case Filter::MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		case Filter::MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case Filter::MIN_MAG_LINEAR_MIP_POINT:
		case Filter::MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
		case Filter::MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case Filter::MIN_MAG_MIP_LINEAR:
		case Filter::MINIMUM_MIN_MAG_MIP_LINEAR:
		case Filter::MAXIMUM_MIN_MAG_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case Filter::ANISOTROPIC:
		case Filter::MINIMUM_ANISOTROPIC:
		case Filter::MAXIMUM_ANISOTROPIC:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = true;
			createInfo.maxAnisotropy = std::min(16.0f, std::max(1.0f, static_cast<float>(desc->max_anisotropy)));
			createInfo.compareEnable = false;
			break;
		case Filter::COMPARISON_MIN_MAG_MIP_POINT:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case Filter::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case Filter::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case Filter::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case Filter::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case Filter::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case Filter::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case Filter::COMPARISON_MIN_MAG_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case Filter::COMPARISON_ANISOTROPIC:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = true;
			createInfo.maxAnisotropy = std::min(16.0f, std::max(1.0f, static_cast<float>(desc->max_anisotropy)));
			createInfo.compareEnable = true;
			break;
		default:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		}

		VkSamplerReductionModeCreateInfo reductionmode = {};
		reductionmode.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
		if (CheckCapability(GraphicsDeviceCapability::SAMPLER_MINMAX))
		{
			switch (desc->filter)
			{
			case Filter::MINIMUM_MIN_MAG_MIP_POINT:
			case Filter::MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
			case Filter::MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter::MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
			case Filter::MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
			case Filter::MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			case Filter::MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
			case Filter::MINIMUM_MIN_MAG_MIP_LINEAR:
			case Filter::MINIMUM_ANISOTROPIC:
				reductionmode.reductionMode = VK_SAMPLER_REDUCTION_MODE_MIN;
				createInfo.pNext = &reductionmode;
				break;
			case Filter::MAXIMUM_MIN_MAG_MIP_POINT:
			case Filter::MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
			case Filter::MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter::MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
			case Filter::MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
			case Filter::MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			case Filter::MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
			case Filter::MAXIMUM_MIN_MAG_MIP_LINEAR:
			case Filter::MAXIMUM_ANISOTROPIC:
				reductionmode.reductionMode = VK_SAMPLER_REDUCTION_MODE_MAX;
				createInfo.pNext = &reductionmode;
				break;
			default:
				break;
			}
		}

		createInfo.addressModeU = _ConvertTextureAddressMode(desc->address_u, features_1_2);
		createInfo.addressModeV = _ConvertTextureAddressMode(desc->address_v, features_1_2);
		createInfo.addressModeW = _ConvertTextureAddressMode(desc->address_w, features_1_2);
		createInfo.compareOp = _ConvertComparisonFunc(desc->comparison_func);
		createInfo.minLod = desc->min_lod;
		createInfo.maxLod = desc->max_lod;
		createInfo.mipLodBias = desc->mip_lod_bias;
		createInfo.borderColor = _ConvertSamplerBorderColor(desc->border_color);
		createInfo.unnormalizedCoordinates = VK_FALSE;

		VkResult res = vkCreateSampler(device, &createInfo, nullptr, &internal_state->resource);
		assert(res == VK_SUCCESS);

		internal_state->index = allocationhandler->bindlessSamplers.allocate();
		if (internal_state->index >= 0)
		{
			VkDescriptorImageInfo imageInfo = {};
			imageInfo.sampler = internal_state->resource;
			VkWriteDescriptorSet write = {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			write.dstBinding = 0;
			write.dstArrayElement = internal_state->index;
			write.descriptorCount = 1;
			write.dstSet = allocationhandler->bindlessSamplers.descriptorSet;
			write.pImageInfo = &imageInfo;
			vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
		}
		else
		{
			assert(0);
		}

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateQueryHeap(const GPUQueryHeapDesc* desc, GPUQueryHeap* queryheap) const
	{
		auto internal_state = std::make_shared<QueryHeap_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		queryheap->internal_state = internal_state;
		queryheap->desc = *desc;

		VkQueryPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		poolInfo.queryCount = desc->query_count;

		switch (desc->type)
		{
		case GpuQueryType::TIMESTAMP:
			poolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
			break;
		case GpuQueryType::OCCLUSION:
		case GpuQueryType::OCCLUSION_BINARY:
			poolInfo.queryType = VK_QUERY_TYPE_OCCLUSION;
			break;
		}

		VkResult res = vkCreateQueryPool(device, &poolInfo, nullptr, &internal_state->pool);
		assert(res == VK_SUCCESS);

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreatePipelineState(const PipelineStateDesc* desc, PipelineState* pso, const RenderPassInfo* renderpass_info) const
	{
		auto internal_state = std::make_shared<PipelineState_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		pso->internal_state = internal_state;
		pso->desc = *desc;

		internal_state->hash = 0;
		wi::helper::hash_combine(internal_state->hash, desc->ms);
		wi::helper::hash_combine(internal_state->hash, desc->as);
		wi::helper::hash_combine(internal_state->hash, desc->vs);
		wi::helper::hash_combine(internal_state->hash, desc->ps);
		wi::helper::hash_combine(internal_state->hash, desc->hs);
		wi::helper::hash_combine(internal_state->hash, desc->ds);
		wi::helper::hash_combine(internal_state->hash, desc->gs);
		wi::helper::hash_combine(internal_state->hash, desc->il);
		wi::helper::hash_combine(internal_state->hash, desc->rs);
		wi::helper::hash_combine(internal_state->hash, desc->bs);
		wi::helper::hash_combine(internal_state->hash, desc->dss);
		wi::helper::hash_combine(internal_state->hash, desc->pt);
		wi::helper::hash_combine(internal_state->hash, desc->sample_mask);

		VkResult res = VK_SUCCESS;

		{
			auto insert_shader = [&](const Shader* shader) {
				if (shader == nullptr)
					return;
				auto shader_internal = to_internal(shader);

				uint32_t i = 0;
				size_t check_max = internal_state->layoutBindings.size(); // dont't check for duplicates within self table
				for (auto& x : shader_internal->layoutBindings)
				{
					bool found = false;
					size_t j = 0;
					for (auto& y : internal_state->layoutBindings)
					{
						if (x.binding == y.binding)
						{
							// If the asserts fire, it means there are overlapping bindings between shader stages
							//	This is not supported now for performance reasons (less binding management)!
							//	(Overlaps between s/b/t bind points are not a problem because those are shifted by the compiler)
							assert(x.descriptorCount == y.descriptorCount);
							assert(x.descriptorType == y.descriptorType);
							found = true;
							y.stageFlags |= x.stageFlags;
							break;
						}
						if (j++ >= check_max)
							break;
					}

					if (!found)
					{
						internal_state->layoutBindings.push_back(x);
						internal_state->imageViewTypes.push_back(shader_internal->imageViewTypes[i]);
						if (x.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || x.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
						{
							for (uint32_t k = 0; k < x.descriptorCount; ++k)
							{
								internal_state->uniform_buffer_sizes[x.binding + k] = shader_internal->uniform_buffer_sizes[x.binding + k];
							}
						}
						if (x.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
						{
							for (uint32_t k = 0; k < x.descriptorCount; ++k)
							{
								internal_state->uniform_buffer_dynamic_slots.push_back(x.binding + k);
							}
						}
					}
					i++;
				}

				if (shader_internal->pushconstants.size > 0)
				{
					internal_state->pushconstants.offset = std::min(internal_state->pushconstants.offset, shader_internal->pushconstants.offset);
					internal_state->pushconstants.size = std::max(internal_state->pushconstants.size, shader_internal->pushconstants.size);
					internal_state->pushconstants.stageFlags |= shader_internal->pushconstants.stageFlags;
				}
			};

			insert_shader(desc->ms);
			insert_shader(desc->as);
			insert_shader(desc->vs);
			insert_shader(desc->hs);
			insert_shader(desc->ds);
			insert_shader(desc->gs);
			insert_shader(desc->ps);

			// sort because dynamic offsets array is tightly packed to match slot numbers:
			std::sort(internal_state->uniform_buffer_dynamic_slots.begin(), internal_state->uniform_buffer_dynamic_slots.end());

			auto insert_shader_bindless = [&](const Shader* shader) {
				if (shader == nullptr)
					return;
				auto shader_internal = to_internal(shader);

				internal_state->bindlessBindings.resize(std::max(internal_state->bindlessBindings.size(), shader_internal->bindlessBindings.size()));

				int i = 0;
				for (auto& x : shader_internal->bindlessBindings)
				{
					if (x.used)
					{
						if (internal_state->bindlessBindings[i].binding.descriptorType != x.binding.descriptorType)
						{
							internal_state->bindlessBindings[i] = x;
						}
						else
						{
							internal_state->bindlessBindings[i].binding.stageFlags |= x.binding.stageFlags;
						}
					}
					i++;
				}
			};

			insert_shader_bindless(desc->ms);
			insert_shader_bindless(desc->as);
			insert_shader_bindless(desc->vs);
			insert_shader_bindless(desc->hs);
			insert_shader_bindless(desc->ds);
			insert_shader_bindless(desc->gs);
			insert_shader_bindless(desc->ps);

			internal_state->binding_hash = 0;
			size_t i = 0;
			for (auto& x : internal_state->layoutBindings)
			{
				wi::helper::hash_combine(internal_state->binding_hash, x.binding);
				wi::helper::hash_combine(internal_state->binding_hash, x.descriptorCount);
				wi::helper::hash_combine(internal_state->binding_hash, x.descriptorType);
				wi::helper::hash_combine(internal_state->binding_hash, x.stageFlags);
				wi::helper::hash_combine(internal_state->binding_hash, internal_state->imageViewTypes[i++]);
			}
			for (auto& x : internal_state->bindlessBindings)
			{
				wi::helper::hash_combine(internal_state->binding_hash, x.used);
				wi::helper::hash_combine(internal_state->binding_hash, x.binding.binding);
				wi::helper::hash_combine(internal_state->binding_hash, x.binding.descriptorCount);
				wi::helper::hash_combine(internal_state->binding_hash, x.binding.descriptorType);
				wi::helper::hash_combine(internal_state->binding_hash, x.binding.stageFlags);
			}
			wi::helper::hash_combine(internal_state->binding_hash, internal_state->pushconstants.offset);
			wi::helper::hash_combine(internal_state->binding_hash, internal_state->pushconstants.size);
			wi::helper::hash_combine(internal_state->binding_hash, internal_state->pushconstants.stageFlags);


			pso_layout_cache_mutex.lock();
			if (pso_layout_cache[internal_state->binding_hash].pipelineLayout == VK_NULL_HANDLE)
			{
				wi::vector<VkDescriptorSetLayout> layouts;
				{
					VkDescriptorSetLayoutCreateInfo descriptorSetlayoutInfo = {};
					descriptorSetlayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					descriptorSetlayoutInfo.pBindings = internal_state->layoutBindings.data();
					descriptorSetlayoutInfo.bindingCount = static_cast<uint32_t>(internal_state->layoutBindings.size());
					res = vkCreateDescriptorSetLayout(device, &descriptorSetlayoutInfo, nullptr, &internal_state->descriptorSetLayout);
					assert(res == VK_SUCCESS);
					layouts.push_back(internal_state->descriptorSetLayout);
				}

				internal_state->bindlessFirstSet = (uint32_t)layouts.size();
				for (auto& x : internal_state->bindlessBindings)
				{
					switch (x.binding.descriptorType)
					{
					case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
						assert(0); // not supported, use the raw buffers for same functionality
						break;
					case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
						layouts.push_back(allocationhandler->bindlessSampledImages.descriptorSetLayout);
						internal_state->bindlessSets.push_back(allocationhandler->bindlessSampledImages.descriptorSet);
						break;
					case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
						layouts.push_back(allocationhandler->bindlessUniformTexelBuffers.descriptorSetLayout);
						internal_state->bindlessSets.push_back(allocationhandler->bindlessUniformTexelBuffers.descriptorSet);
						break;
					case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
						layouts.push_back(allocationhandler->bindlessStorageBuffers.descriptorSetLayout);
						internal_state->bindlessSets.push_back(allocationhandler->bindlessStorageBuffers.descriptorSet);
						break;
					case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
						layouts.push_back(allocationhandler->bindlessStorageImages.descriptorSetLayout);
						internal_state->bindlessSets.push_back(allocationhandler->bindlessStorageImages.descriptorSet);
						break;
					case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
						layouts.push_back(allocationhandler->bindlessStorageTexelBuffers.descriptorSetLayout);
						internal_state->bindlessSets.push_back(allocationhandler->bindlessStorageTexelBuffers.descriptorSet);
						break;
					case VK_DESCRIPTOR_TYPE_SAMPLER:
						layouts.push_back(allocationhandler->bindlessSamplers.descriptorSetLayout);
						internal_state->bindlessSets.push_back(allocationhandler->bindlessSamplers.descriptorSet);
						break;
					case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
						layouts.push_back(allocationhandler->bindlessAccelerationStructures.descriptorSetLayout);
						internal_state->bindlessSets.push_back(allocationhandler->bindlessAccelerationStructures.descriptorSet);
						break;
					default:
						break;
					}
					
				}

				VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
				pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutInfo.pSetLayouts = layouts.data();
				pipelineLayoutInfo.setLayoutCount = (uint32_t)layouts.size();
				if (internal_state->pushconstants.size > 0)
				{
					pipelineLayoutInfo.pushConstantRangeCount = 1;
					pipelineLayoutInfo.pPushConstantRanges = &internal_state->pushconstants;
				}
				else
				{
					pipelineLayoutInfo.pushConstantRangeCount = 0;
					pipelineLayoutInfo.pPushConstantRanges = nullptr;
				}

				res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &internal_state->pipelineLayout);
				assert(res == VK_SUCCESS);
				if (res == VK_SUCCESS)
				{
					pso_layout_cache[internal_state->binding_hash].descriptorSetLayout = internal_state->descriptorSetLayout;
					pso_layout_cache[internal_state->binding_hash].pipelineLayout = internal_state->pipelineLayout;
					pso_layout_cache[internal_state->binding_hash].bindlessSets = internal_state->bindlessSets;
					pso_layout_cache[internal_state->binding_hash].bindlessFirstSet = internal_state->bindlessFirstSet;
				}
			}
			else
			{
				internal_state->descriptorSetLayout = pso_layout_cache[internal_state->binding_hash].descriptorSetLayout;
				internal_state->pipelineLayout = pso_layout_cache[internal_state->binding_hash].pipelineLayout;
				internal_state->bindlessSets = pso_layout_cache[internal_state->binding_hash].bindlessSets;
				internal_state->bindlessFirstSet = pso_layout_cache[internal_state->binding_hash].bindlessFirstSet;
			}
			pso_layout_cache_mutex.unlock();
		}


		VkGraphicsPipelineCreateInfo& pipelineInfo = internal_state->pipelineInfo;
		//pipelineInfo.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = internal_state->pipelineLayout;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		// Shaders:

		uint32_t shaderStageCount = 0;
		auto& shaderStages = internal_state->shaderStages;
		if (pso->desc.ms != nullptr && pso->desc.ms->IsValid())
		{
			shaderStages[shaderStageCount++] = to_internal(pso->desc.ms)->stageInfo;
		}
		if (pso->desc.as != nullptr && pso->desc.as->IsValid())
		{
			shaderStages[shaderStageCount++] = to_internal(pso->desc.as)->stageInfo;
		}
		if (pso->desc.vs != nullptr && pso->desc.vs->IsValid())
		{
			shaderStages[shaderStageCount++] = to_internal(pso->desc.vs)->stageInfo;
		}
		if (pso->desc.hs != nullptr && pso->desc.hs->IsValid())
		{
			shaderStages[shaderStageCount++] = to_internal(pso->desc.hs)->stageInfo;
		}
		if (pso->desc.ds != nullptr && pso->desc.ds->IsValid())
		{
			shaderStages[shaderStageCount++] = to_internal(pso->desc.ds)->stageInfo;
		}
		if (pso->desc.gs != nullptr && pso->desc.gs->IsValid())
		{
			shaderStages[shaderStageCount++] = to_internal(pso->desc.gs)->stageInfo;
		}
		if (pso->desc.ps != nullptr && pso->desc.ps->IsValid())
		{
			shaderStages[shaderStageCount++] = to_internal(pso->desc.ps)->stageInfo;
		}
		pipelineInfo.stageCount = shaderStageCount;
		pipelineInfo.pStages = shaderStages;


		// Fixed function states:

		// Primitive type:
		VkPipelineInputAssemblyStateCreateInfo& inputAssembly = internal_state->inputAssembly;
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		switch (pso->desc.pt)
		{
		case PrimitiveTopology::POINTLIST:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			break;
		case PrimitiveTopology::LINELIST:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			break;
		case PrimitiveTopology::LINESTRIP:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
			break;
		case PrimitiveTopology::TRIANGLESTRIP:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			break;
		case PrimitiveTopology::TRIANGLELIST:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			break;
		case PrimitiveTopology::PATCHLIST:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
			break;
		default:
			break;
		}
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		pipelineInfo.pInputAssemblyState = &inputAssembly;


		// Rasterizer:
		VkPipelineRasterizationStateCreateInfo& rasterizer = internal_state->rasterizer;
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_TRUE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		// depth clip will be enabled via Vulkan 1.1 extension VK_EXT_depth_clip_enable:
		VkPipelineRasterizationDepthClipStateCreateInfoEXT& depthclip = internal_state->depthclip;
		depthclip.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;
		depthclip.depthClipEnable = VK_TRUE;
		if (depth_clip_enable_features.depthClipEnable == VK_TRUE)
		{
			rasterizer.pNext = &depthclip;
		}

		if (pso->desc.rs != nullptr)
		{
			const RasterizerState& desc = *pso->desc.rs;

			switch (desc.fill_mode)
			{
			case FillMode::WIREFRAME:
				rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
				break;
			case FillMode::SOLID:
			default:
				rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
				break;
			}

			switch (desc.cull_mode)
			{
			case CullMode::BACK:
				rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
				break;
			case CullMode::FRONT:
				rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
				break;
			case CullMode::NONE:
			default:
				rasterizer.cullMode = VK_CULL_MODE_NONE;
				break;
			}

			rasterizer.frontFace = desc.front_counter_clockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
			rasterizer.depthBiasEnable = desc.depth_bias != 0 || desc.slope_scaled_depth_bias != 0;
			rasterizer.depthBiasConstantFactor = static_cast<float>(desc.depth_bias);
			rasterizer.depthBiasClamp = desc.depth_bias_clamp;
			rasterizer.depthBiasSlopeFactor = desc.slope_scaled_depth_bias;

			// depth clip is extension in Vulkan 1.1:
			depthclip.depthClipEnable = desc.depth_clip_enable ? VK_TRUE : VK_FALSE;
		}

		pipelineInfo.pRasterizationState = &rasterizer;

		VkPipelineViewportStateCreateInfo& viewportState = internal_state->viewportState;
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 0;
		viewportState.pViewports = nullptr;
		viewportState.scissorCount = 0;
		viewportState.pScissors = nullptr;

		pipelineInfo.pViewportState = &viewportState;


		// Depth-Stencil:
		VkPipelineDepthStencilStateCreateInfo& depthstencil = internal_state->depthstencil;
		depthstencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		if (pso->desc.dss != nullptr)
		{
			depthstencil.depthTestEnable = pso->desc.dss->depth_enable ? VK_TRUE : VK_FALSE;
			depthstencil.depthWriteEnable = pso->desc.dss->depth_write_mask == DepthWriteMask::ZERO ? VK_FALSE : VK_TRUE;
			depthstencil.depthCompareOp = _ConvertComparisonFunc(pso->desc.dss->depth_func);

			if (pso->desc.dss->stencil_enable)
			{
				depthstencil.stencilTestEnable = VK_TRUE;

				depthstencil.front.compareMask = pso->desc.dss->stencil_read_mask;
				depthstencil.front.writeMask = pso->desc.dss->stencil_write_mask;
				depthstencil.front.reference = 0; // runtime supplied
				depthstencil.front.compareOp = _ConvertComparisonFunc(pso->desc.dss->front_face.stencil_func);
				depthstencil.front.passOp = _ConvertStencilOp(pso->desc.dss->front_face.stencil_pass_op);
				depthstencil.front.failOp = _ConvertStencilOp(pso->desc.dss->front_face.stencil_fail_op);
				depthstencil.front.depthFailOp = _ConvertStencilOp(pso->desc.dss->front_face.stencil_depth_fail_op);

				depthstencil.back.compareMask = pso->desc.dss->stencil_read_mask;
				depthstencil.back.writeMask = pso->desc.dss->stencil_write_mask;
				depthstencil.back.reference = 0; // runtime supplied
				depthstencil.back.compareOp = _ConvertComparisonFunc(pso->desc.dss->back_face.stencil_func);
				depthstencil.back.passOp = _ConvertStencilOp(pso->desc.dss->back_face.stencil_pass_op);
				depthstencil.back.failOp = _ConvertStencilOp(pso->desc.dss->back_face.stencil_fail_op);
				depthstencil.back.depthFailOp = _ConvertStencilOp(pso->desc.dss->back_face.stencil_depth_fail_op);
			}
			else
			{
				depthstencil.stencilTestEnable = VK_FALSE;

				depthstencil.front.compareMask = 0;
				depthstencil.front.writeMask = 0;
				depthstencil.front.reference = 0;
				depthstencil.front.compareOp = VK_COMPARE_OP_NEVER;
				depthstencil.front.passOp = VK_STENCIL_OP_KEEP;
				depthstencil.front.failOp = VK_STENCIL_OP_KEEP;
				depthstencil.front.depthFailOp = VK_STENCIL_OP_KEEP;

				depthstencil.back.compareMask = 0;
				depthstencil.back.writeMask = 0;
				depthstencil.back.reference = 0; // runtime supplied
				depthstencil.back.compareOp = VK_COMPARE_OP_NEVER;
				depthstencil.back.passOp = VK_STENCIL_OP_KEEP;
				depthstencil.back.failOp = VK_STENCIL_OP_KEEP;
				depthstencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
			}

			if (CheckCapability(GraphicsDeviceCapability::DEPTH_BOUNDS_TEST))
			{
				depthstencil.depthBoundsTestEnable = pso->desc.dss->depth_bounds_test_enable ? VK_TRUE : VK_FALSE;
			}
			else
			{
				depthstencil.depthBoundsTestEnable = VK_FALSE;
			}
		}

		pipelineInfo.pDepthStencilState = &depthstencil;


		// Tessellation:
		VkPipelineTessellationStateCreateInfo& tessellationInfo = internal_state->tessellationInfo;
		tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		tessellationInfo.patchControlPoints = desc->patch_control_points;

		pipelineInfo.pTessellationState = &tessellationInfo;

		if (pso->desc.ms == nullptr)
		{
			pipelineInfo.pDynamicState = &dynamicStateInfo;
		}
		else
		{
			pipelineInfo.pDynamicState = &dynamicStateInfo_MeshShader;
		}

		if (renderpass_info != nullptr)
		{
			// MSAA:
			VkPipelineMultisampleStateCreateInfo multisampling = {};
			multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable = VK_FALSE;
			multisampling.rasterizationSamples = (VkSampleCountFlagBits)renderpass_info->sample_count;
			if (pso->desc.rs != nullptr)
			{
				const RasterizerState& desc = *pso->desc.rs;
				if (desc.forced_sample_count > 1)
				{
					multisampling.rasterizationSamples = (VkSampleCountFlagBits)desc.forced_sample_count;
				}
			}
			multisampling.minSampleShading = 1.0f;
			VkSampleMask samplemask = internal_state->samplemask;
			samplemask = pso->desc.sample_mask;
			multisampling.pSampleMask = &samplemask;
			if (pso->desc.bs != nullptr)
			{
				multisampling.alphaToCoverageEnable = pso->desc.bs->alpha_to_coverage_enable ? VK_TRUE : VK_FALSE;
			}
			else
			{
				multisampling.alphaToCoverageEnable = VK_FALSE;
			}
			multisampling.alphaToOneEnable = VK_FALSE;

			pipelineInfo.pMultisampleState = &multisampling;


			// Blending:
			uint32_t numBlendAttachments = 0;
			VkPipelineColorBlendAttachmentState colorBlendAttachments[8] = {};
			for (size_t i = 0; i < renderpass_info->rt_count; ++i)
			{
				size_t attachmentIndex = 0;
				if (pso->desc.bs->independent_blend_enable)
					attachmentIndex = i;

				const auto& desc = pso->desc.bs->render_target[attachmentIndex];
				VkPipelineColorBlendAttachmentState& attachment = colorBlendAttachments[numBlendAttachments];
				numBlendAttachments++;

				attachment.blendEnable = desc.blend_enable ? VK_TRUE : VK_FALSE;

				attachment.colorWriteMask = 0;
				if (has_flag(desc.render_target_write_mask, ColorWrite::ENABLE_RED))
				{
					attachment.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
				}
				if (has_flag(desc.render_target_write_mask, ColorWrite::ENABLE_GREEN))
				{
					attachment.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
				}
				if (has_flag(desc.render_target_write_mask, ColorWrite::ENABLE_BLUE))
				{
					attachment.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
				}
				if (has_flag(desc.render_target_write_mask, ColorWrite::ENABLE_ALPHA))
				{
					attachment.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
				}

				attachment.srcColorBlendFactor = _ConvertBlend(desc.src_blend);
				attachment.dstColorBlendFactor = _ConvertBlend(desc.dest_blend);
				attachment.colorBlendOp = _ConvertBlendOp(desc.blend_op);
				attachment.srcAlphaBlendFactor = _ConvertBlend(desc.src_blend_alpha);
				attachment.dstAlphaBlendFactor = _ConvertBlend(desc.dest_blend_alpha);
				attachment.alphaBlendOp = _ConvertBlendOp(desc.blend_op_alpha);
			}

			VkPipelineColorBlendStateCreateInfo colorBlending = {};
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.logicOp = VK_LOGIC_OP_COPY;
			colorBlending.attachmentCount = numBlendAttachments;
			colorBlending.pAttachments = colorBlendAttachments;
			colorBlending.blendConstants[0] = 1.0f;
			colorBlending.blendConstants[1] = 1.0f;
			colorBlending.blendConstants[2] = 1.0f;
			colorBlending.blendConstants[3] = 1.0f;

			pipelineInfo.pColorBlendState = &colorBlending;

			// Input layout:
			VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			wi::vector<VkVertexInputBindingDescription> bindings;
			wi::vector<VkVertexInputAttributeDescription> attributes;
			if (pso->desc.il != nullptr)
			{
				uint32_t lastBinding = 0xFFFFFFFF;
				for (auto& x : pso->desc.il->elements)
				{
					if (x.input_slot == lastBinding)
						continue;
					lastBinding = x.input_slot;
					VkVertexInputBindingDescription& bind = bindings.emplace_back();
					bind.binding = x.input_slot;
					bind.inputRate = x.input_slot_class == InputClassification::PER_VERTEX_DATA ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
					bind.stride = GetFormatStride(x.format);
				}

				uint32_t offset = 0;
				uint32_t i = 0;
				lastBinding = 0xFFFFFFFF;
				for (auto& x : pso->desc.il->elements)
				{
					VkVertexInputAttributeDescription attr = {};
					attr.binding = x.input_slot;
					if (attr.binding != lastBinding)
					{
						lastBinding = attr.binding;
						offset = 0;
					}
					attr.format = _ConvertFormat(x.format);
					attr.location = i;
					attr.offset = x.aligned_byte_offset;
					if (attr.offset == InputLayout::APPEND_ALIGNED_ELEMENT)
					{
						// need to manually resolve this from the format spec.
						attr.offset = offset;
						offset += GetFormatStride(x.format);
					}

					attributes.push_back(attr);

					i++;
				}

				vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
				vertexInputInfo.pVertexBindingDescriptions = bindings.data();
				vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
				vertexInputInfo.pVertexAttributeDescriptions = attributes.data();
			}
			pipelineInfo.pVertexInputState = &vertexInputInfo;

			pipelineInfo.renderPass = VK_NULL_HANDLE; // instead we use VkPipelineRenderingCreateInfo

			VkPipelineRenderingCreateInfo renderingInfo = {};
			renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
			renderingInfo.viewMask = 0;
			renderingInfo.colorAttachmentCount = renderpass_info->rt_count;
			VkFormat formats[8] = {};
			for (uint32_t i = 0; i < renderpass_info->rt_count; ++i)
			{
				formats[i] = _ConvertFormat(renderpass_info->rt_formats[i]);
			}
			renderingInfo.pColorAttachmentFormats = formats;
			renderingInfo.depthAttachmentFormat = _ConvertFormat(renderpass_info->ds_format);
			if (IsFormatStencilSupport(renderpass_info->ds_format))
			{
				renderingInfo.stencilAttachmentFormat = renderingInfo.depthAttachmentFormat;
			}
			pipelineInfo.pNext = &renderingInfo;

			VkResult res = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &internal_state->pipeline);
			assert(res == VK_SUCCESS);
		}

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* desc, RaytracingAccelerationStructure* bvh) const
	{
		auto internal_state = std::make_shared<BVH_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		bvh->internal_state = internal_state;
		bvh->type = GPUResource::Type::RAYTRACING_ACCELERATION_STRUCTURE;
		bvh->desc = *desc;
		bvh->size = 0;

		internal_state->buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		internal_state->buildInfo.flags = 0;
		if (bvh->desc.flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE)
		{
			internal_state->buildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		}
		if (bvh->desc.flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_COMPACTION)
		{
			internal_state->buildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
		}
		if (bvh->desc.flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_TRACE)
		{
			internal_state->buildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		}
		if (bvh->desc.flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD)
		{
			internal_state->buildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
		}
		if (bvh->desc.flags & RaytracingAccelerationStructureDesc::FLAG_MINIMIZE_MEMORY)
		{
			internal_state->buildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;
		}

		switch (desc->type)
		{
		case RaytracingAccelerationStructureDesc::Type::BOTTOMLEVEL:
		{
			internal_state->buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

			for (auto& x : desc->bottom_level.geometries)
			{
				internal_state->geometries.emplace_back();
				auto& geometry = internal_state->geometries.back();
				geometry = {};
				geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;

				internal_state->primitiveCounts.emplace_back();
				uint32_t& primitiveCount = internal_state->primitiveCounts.back();

				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type::TRIANGLES)
				{
					geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
					geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
					geometry.geometry.triangles.indexType = x.triangles.index_format == IndexBufferFormat::UINT16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
					geometry.geometry.triangles.maxVertex = x.triangles.vertex_count;
					geometry.geometry.triangles.vertexStride = x.triangles.vertex_stride;
					geometry.geometry.triangles.vertexFormat = _ConvertFormat(x.triangles.vertex_format);

					primitiveCount = x.triangles.index_count / 3;
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type::PROCEDURAL_AABBS)
				{
					geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
					geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
					geometry.geometry.aabbs.stride = sizeof(float) * 6; // min - max corners

					primitiveCount = x.aabbs.count;
				}
			}


		}
		break;
		case RaytracingAccelerationStructureDesc::Type::TOPLEVEL:
		{
			internal_state->buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

			internal_state->geometries.emplace_back();
			auto& geometry = internal_state->geometries.back();
			geometry = {};
			geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
			geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
			geometry.geometry.instances.arrayOfPointers = VK_FALSE;

			internal_state->primitiveCounts.emplace_back();
			uint32_t& primitiveCount = internal_state->primitiveCounts.back();
			primitiveCount = desc->top_level.count;
		}
		break;
		}

		internal_state->buildInfo.geometryCount = (uint32_t)internal_state->geometries.size();
		internal_state->buildInfo.pGeometries = internal_state->geometries.data();

		internal_state->sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		// Compute memory requirements:
		vkGetAccelerationStructureBuildSizesKHR(
			device,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&internal_state->buildInfo,
			internal_state->primitiveCounts.data(),
			&internal_state->sizeInfo
		);

		// Backing memory as buffer:
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = internal_state->sizeInfo.accelerationStructureSize +
			std::max(internal_state->sizeInfo.buildScratchSize, internal_state->sizeInfo.updateScratchSize);
		bufferInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
		bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; // scratch
		bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		bufferInfo.flags = 0;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		VkResult res = vmaCreateBuffer(
			allocationhandler->allocator,
			&bufferInfo,
			&allocInfo,
			&internal_state->buffer,
			&internal_state->allocation,
			nullptr
		);
		assert(res == VK_SUCCESS);

		// Create the acceleration structure:
		internal_state->createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		internal_state->createInfo.type = internal_state->buildInfo.type;
		internal_state->createInfo.buffer = internal_state->buffer;
		internal_state->createInfo.size = internal_state->sizeInfo.accelerationStructureSize;

		res = vkCreateAccelerationStructureKHR(
			device,
			&internal_state->createInfo,
			nullptr,
			&internal_state->resource
		);
		assert(res == VK_SUCCESS);

		// Get the device address for the acceleration structure:
		VkAccelerationStructureDeviceAddressInfoKHR addrinfo = {};
		addrinfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		addrinfo.accelerationStructure = internal_state->resource;
		internal_state->as_address = vkGetAccelerationStructureDeviceAddressKHR(device, &addrinfo);

		// Get scratch address:
		VkBufferDeviceAddressInfo addressinfo = {};
		addressinfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		addressinfo.buffer = internal_state->buffer;
		internal_state->scratch_address = vkGetBufferDeviceAddress(device, &addressinfo)
			+ internal_state->sizeInfo.accelerationStructureSize;


		if (desc->type == RaytracingAccelerationStructureDesc::Type::TOPLEVEL)
		{
			internal_state->index = allocationhandler->bindlessAccelerationStructures.allocate();
			if (internal_state->index >= 0)
			{
				VkWriteDescriptorSetAccelerationStructureKHR acc = {};
				acc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
				acc.accelerationStructureCount = 1;
				acc.pAccelerationStructures = &internal_state->resource;

				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
				write.dstBinding = 0;
				write.dstArrayElement = internal_state->index;
				write.descriptorCount = 1;
				write.dstSet = allocationhandler->bindlessAccelerationStructures.descriptorSet;
				write.pNext = &acc;
				vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
			}
		}

#if 0
		buildAccelerationStructuresKHR(
			device,
			VK_NULL_HANDLE,
			1,
			&info,
			&pRangeInfo
		);
#endif

		bvh->size = bufferInfo.size;

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* desc, RaytracingPipelineState* rtpso) const
	{
		auto internal_state = std::make_shared<RTPipelineState_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		rtpso->internal_state = internal_state;
		rtpso->desc = *desc;

		VkRayTracingPipelineCreateInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		info.flags = 0;

		wi::vector<VkPipelineShaderStageCreateInfo> stages;
		for (auto& x : desc->shader_libraries)
		{
			stages.emplace_back();
			auto& stage = stages.back();
			stage = {};
			stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage.module = to_internal(x.shader)->shaderModule;
			switch (x.type)
			{
			default:
			case ShaderLibrary::Type::RAYGENERATION:
				stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
				break;
			case ShaderLibrary::Type::MISS:
				stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
				break;
			case ShaderLibrary::Type::CLOSESTHIT:
				stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
				break;
			case ShaderLibrary::Type::ANYHIT:
				stage.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
				break;
			case ShaderLibrary::Type::INTERSECTION:
				stage.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
				break;
			}
			stage.pName = x.function_name.c_str();
		}
		info.stageCount = (uint32_t)stages.size();
		info.pStages = stages.data();

		wi::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;
		groups.reserve(desc->hit_groups.size());
		for (auto& x : desc->hit_groups)
		{
			groups.emplace_back();
			auto& group = groups.back();
			group = {};
			group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			switch (x.type)
			{
			default:
			case ShaderHitGroup::Type::GENERAL:
				group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
				break;
			case ShaderHitGroup::Type::TRIANGLES:
				group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
				break;
			case ShaderHitGroup::Type::PROCEDURAL:
				group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
				break;
			}
			group.generalShader = x.general_shader;
			group.closestHitShader = x.closest_hit_shader;
			group.anyHitShader = x.any_hit_shader;
			group.intersectionShader = x.intersection_shader;
		}
		info.groupCount = (uint32_t)groups.size();
		info.pGroups = groups.data();

		info.maxPipelineRayRecursionDepth = desc->max_trace_recursion_depth;

		info.layout = to_internal(desc->shader_libraries.front().shader)->pipelineLayout_cs;

		//VkRayTracingPipelineInterfaceCreateInfoKHR library_interface = {};
		//library_interface.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_INTERFACE_CREATE_INFO_KHR;
		//library_interface.maxPipelineRayPayloadSize = pDesc->max_payload_size_in_bytes;
		//library_interface.maxPipelineRayHitAttributeSize = pDesc->max_attribute_size_in_bytes;
		//info.pLibraryInterface = &library_interface;

		info.basePipelineHandle = VK_NULL_HANDLE;
		info.basePipelineIndex = 0;

		VkResult res = vkCreateRayTracingPipelinesKHR(
			device,
			VK_NULL_HANDLE,
			pipelineCache,
			1,
			&info,
			nullptr,
			&internal_state->pipeline
		);
		assert(res == VK_SUCCESS);

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateVideoDecoder(const VideoDesc* desc, VideoDecoder* video_decoder) const
	{
		VkResult res = VK_SUCCESS;

		auto internal_state = std::make_shared<VideoDecoder_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		video_decoder->internal_state = internal_state;
		video_decoder->desc = *desc;

		wi::vector<StdVideoH264PictureParameterSet> pps_array_h264(desc->pps_count);
		wi::vector<StdVideoH264ScalingLists> scalinglist_array_h264(desc->pps_count);
		for (uint32_t i = 0; i < desc->pps_count; ++i)
		{
			const h264::PPS* pps = (const h264::PPS*)desc->pps_datas + i;
			StdVideoH264PictureParameterSet& vk_pps = pps_array_h264[i];
			StdVideoH264ScalingLists& vk_scalinglist = scalinglist_array_h264[i];

			vk_pps.flags.transform_8x8_mode_flag = pps->transform_8x8_mode_flag;
			vk_pps.flags.redundant_pic_cnt_present_flag = pps->redundant_pic_cnt_present_flag;
			vk_pps.flags.constrained_intra_pred_flag = pps->constrained_intra_pred_flag;
			vk_pps.flags.deblocking_filter_control_present_flag = pps->deblocking_filter_control_present_flag;
			vk_pps.flags.weighted_pred_flag = pps->weighted_pred_flag;
			vk_pps.flags.bottom_field_pic_order_in_frame_present_flag = pps->pic_order_present_flag;
			vk_pps.flags.entropy_coding_mode_flag = pps->entropy_coding_mode_flag;
			vk_pps.flags.pic_scaling_matrix_present_flag = pps->pic_scaling_matrix_present_flag;

			vk_pps.seq_parameter_set_id = pps->seq_parameter_set_id;
			vk_pps.pic_parameter_set_id = pps->pic_parameter_set_id;
			vk_pps.num_ref_idx_l0_default_active_minus1 = pps->num_ref_idx_l0_active_minus1;
			vk_pps.num_ref_idx_l1_default_active_minus1 = pps->num_ref_idx_l1_active_minus1;
			vk_pps.weighted_bipred_idc = (StdVideoH264WeightedBipredIdc)pps->weighted_bipred_idc;
			vk_pps.pic_init_qp_minus26 = pps->pic_init_qp_minus26;
			vk_pps.pic_init_qs_minus26 = pps->pic_init_qs_minus26;
			vk_pps.chroma_qp_index_offset = pps->chroma_qp_index_offset;
			vk_pps.second_chroma_qp_index_offset = pps->second_chroma_qp_index_offset;

			vk_pps.pScalingLists = &vk_scalinglist;
			for (int j = 0; j < arraysize(pps->pic_scaling_list_present_flag); ++j)
			{
				vk_scalinglist.scaling_list_present_mask |= pps->pic_scaling_list_present_flag[j] << j;
			}
			for (int j = 0; j < arraysize(pps->UseDefaultScalingMatrix4x4Flag); ++j)
			{
				vk_scalinglist.use_default_scaling_matrix_mask |= pps->UseDefaultScalingMatrix4x4Flag[j] << j;
			}
			for (int j = 0; j < arraysize(pps->ScalingList4x4); ++j)
			{
				for (int k = 0; k < arraysize(pps->ScalingList4x4[j]); ++k)
				{
					vk_scalinglist.ScalingList4x4[j][k] = (uint8_t)pps->ScalingList4x4[j][k];
				}
			}
			for (int j = 0; j < arraysize(pps->ScalingList8x8); ++j)
			{
				for (int k = 0; k < arraysize(pps->ScalingList8x8[j]); ++k)
				{
					vk_scalinglist.ScalingList8x8[j][k] = (uint8_t)pps->ScalingList8x8[j][k];
				}
			}
		}

		uint32_t num_reference_frames = 0;
		wi::vector<StdVideoH264SequenceParameterSet> sps_array_h264(desc->sps_count);
		wi::vector<StdVideoH264SequenceParameterSetVui> vui_array_h264(desc->sps_count);
		wi::vector<StdVideoH264HrdParameters> hrd_array_h264(desc->sps_count);
		for (uint32_t i = 0; i < desc->sps_count; ++i)
		{
			const h264::SPS* sps = (const h264::SPS*)desc->sps_datas + i;
			StdVideoH264SequenceParameterSet& vk_sps = sps_array_h264[i];

			vk_sps.flags.constraint_set0_flag = sps->constraint_set0_flag;
			vk_sps.flags.constraint_set1_flag = sps->constraint_set1_flag;
			vk_sps.flags.constraint_set2_flag = sps->constraint_set2_flag;
			vk_sps.flags.constraint_set3_flag = sps->constraint_set3_flag;
			vk_sps.flags.constraint_set4_flag = sps->constraint_set4_flag;
			vk_sps.flags.constraint_set5_flag = sps->constraint_set5_flag;
			vk_sps.flags.direct_8x8_inference_flag = sps->direct_8x8_inference_flag;
			vk_sps.flags.mb_adaptive_frame_field_flag = sps->mb_adaptive_frame_field_flag;
			vk_sps.flags.frame_mbs_only_flag = sps->frame_mbs_only_flag;
			vk_sps.flags.delta_pic_order_always_zero_flag = sps->delta_pic_order_always_zero_flag;
			vk_sps.flags.separate_colour_plane_flag = sps->separate_colour_plane_flag;
			vk_sps.flags.gaps_in_frame_num_value_allowed_flag = sps->gaps_in_frame_num_value_allowed_flag;
			vk_sps.flags.qpprime_y_zero_transform_bypass_flag = sps->qpprime_y_zero_transform_bypass_flag;
			vk_sps.flags.frame_cropping_flag = sps->frame_cropping_flag;
			vk_sps.flags.seq_scaling_matrix_present_flag = sps->seq_scaling_matrix_present_flag;
			vk_sps.flags.vui_parameters_present_flag = sps->vui_parameters_present_flag;

			if (vk_sps.flags.vui_parameters_present_flag)
			{
				StdVideoH264SequenceParameterSetVui& vk_vui = vui_array_h264[i];
				vk_sps.pSequenceParameterSetVui = &vk_vui;
				vk_vui.flags.aspect_ratio_info_present_flag = sps->vui.aspect_ratio_info_present_flag;
				vk_vui.flags.overscan_info_present_flag = sps->vui.overscan_info_present_flag;
				vk_vui.flags.overscan_appropriate_flag = sps->vui.overscan_appropriate_flag;
				vk_vui.flags.video_signal_type_present_flag = sps->vui.video_signal_type_present_flag;
				vk_vui.flags.video_full_range_flag = sps->vui.video_full_range_flag;
				vk_vui.flags.color_description_present_flag = sps->vui.colour_description_present_flag;
				vk_vui.flags.chroma_loc_info_present_flag = sps->vui.chroma_loc_info_present_flag;
				vk_vui.flags.timing_info_present_flag = sps->vui.timing_info_present_flag;
				vk_vui.flags.fixed_frame_rate_flag = sps->vui.fixed_frame_rate_flag;
				vk_vui.flags.bitstream_restriction_flag = sps->vui.bitstream_restriction_flag;
				vk_vui.flags.nal_hrd_parameters_present_flag = sps->vui.nal_hrd_parameters_present_flag;
				vk_vui.flags.vcl_hrd_parameters_present_flag = sps->vui.vcl_hrd_parameters_present_flag;

				vk_vui.aspect_ratio_idc = (StdVideoH264AspectRatioIdc)sps->vui.aspect_ratio_idc;
				vk_vui.sar_width = sps->vui.sar_width;
				vk_vui.sar_height = sps->vui.sar_height;
				vk_vui.video_format = sps->vui.video_format;
				vk_vui.colour_primaries = sps->vui.colour_primaries;
				vk_vui.transfer_characteristics = sps->vui.transfer_characteristics;
				vk_vui.matrix_coefficients = sps->vui.matrix_coefficients;
				vk_vui.num_units_in_tick = sps->vui.num_units_in_tick;
				vk_vui.time_scale = sps->vui.time_scale;
				vk_vui.max_num_reorder_frames = sps->vui.num_reorder_frames;
				vk_vui.max_dec_frame_buffering = sps->vui.max_dec_frame_buffering;
				vk_vui.chroma_sample_loc_type_top_field = sps->vui.chroma_sample_loc_type_top_field;
				vk_vui.chroma_sample_loc_type_bottom_field = sps->vui.chroma_sample_loc_type_bottom_field;

				StdVideoH264HrdParameters& vk_hrd = hrd_array_h264[i];
				vk_vui.pHrdParameters = &vk_hrd;
				vk_hrd.cpb_cnt_minus1 = sps->hrd.cpb_cnt_minus1;
				vk_hrd.bit_rate_scale = sps->hrd.bit_rate_scale;
				vk_hrd.cpb_size_scale = sps->hrd.cpb_size_scale;
				for (int j = 0; j < arraysize(sps->hrd.bit_rate_value_minus1); ++j)
				{
					vk_hrd.bit_rate_value_minus1[j] = sps->hrd.bit_rate_value_minus1[j];
					vk_hrd.cpb_size_value_minus1[j] = sps->hrd.cpb_size_value_minus1[j];
					vk_hrd.cbr_flag[j] = sps->hrd.cbr_flag[j];
				}
				vk_hrd.initial_cpb_removal_delay_length_minus1 = sps->hrd.initial_cpb_removal_delay_length_minus1;
				vk_hrd.cpb_removal_delay_length_minus1 = sps->hrd.cpb_removal_delay_length_minus1;
				vk_hrd.dpb_output_delay_length_minus1 = sps->hrd.dpb_output_delay_length_minus1;
				vk_hrd.time_offset_length = sps->hrd.time_offset_length;
			}

			vk_sps.profile_idc = (StdVideoH264ProfileIdc)sps->profile_idc;
			switch (sps->level_idc)
			{
			case 0:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_1_0;
				break;
			case 11:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_1_1;
				break;
			case 12:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_1_2;
				break;
			case 13:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_1_3;
				break;
			case 20:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_2_0;
				break;
			case 21:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_2_1;
				break;
			case 22:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_2_2;
				break;
			case 30:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_3_0;
				break;
			case 31:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_3_1;
				break;
			case 32:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_3_2;
				break;
			case 40:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_4_0;
				break;
			case 41:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_4_1;
				break;
			case 42:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_4_2;
				break;
			case 50:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_5_0;
				break;
			case 51:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_5_1;
				break;
			case 52:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_5_2;
				break;
			case 60:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_6_0;
				break;
			case 61:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_6_1;
				break;
			case 62:
				vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_6_2;
				break;
			default:
				assert(0);
				break;
			}
			assert(vk_sps.level_idc <= decode_h264_capabilities.maxLevelIdc);
			//vk_sps.chroma_format_idc = (StdVideoH264ChromaFormatIdc)sps->chroma_format_idc;
			vk_sps.chroma_format_idc = STD_VIDEO_H264_CHROMA_FORMAT_IDC_420; // only one we support currently
			vk_sps.seq_parameter_set_id = sps->seq_parameter_set_id;
			vk_sps.bit_depth_luma_minus8 = sps->bit_depth_luma_minus8;
			vk_sps.bit_depth_chroma_minus8 = sps->bit_depth_chroma_minus8;
			vk_sps.log2_max_frame_num_minus4 = sps->log2_max_frame_num_minus4;
			vk_sps.pic_order_cnt_type = (StdVideoH264PocType)sps->pic_order_cnt_type;
			vk_sps.offset_for_non_ref_pic = sps->offset_for_non_ref_pic;
			vk_sps.offset_for_top_to_bottom_field = sps->offset_for_top_to_bottom_field;
			vk_sps.log2_max_pic_order_cnt_lsb_minus4 = sps->log2_max_pic_order_cnt_lsb_minus4;
			vk_sps.num_ref_frames_in_pic_order_cnt_cycle = sps->num_ref_frames_in_pic_order_cnt_cycle;
			vk_sps.max_num_ref_frames = sps->num_ref_frames;
			vk_sps.pic_width_in_mbs_minus1 = sps->pic_width_in_mbs_minus1;
			vk_sps.pic_height_in_map_units_minus1 = sps->pic_height_in_map_units_minus1;
			vk_sps.frame_crop_left_offset = sps->frame_crop_left_offset;
			vk_sps.frame_crop_right_offset = sps->frame_crop_right_offset;
			vk_sps.frame_crop_top_offset = sps->frame_crop_top_offset;
			vk_sps.frame_crop_bottom_offset = sps->frame_crop_bottom_offset;
			vk_sps.pOffsetForRefFrame = sps->offset_for_ref_frame;

			num_reference_frames = std::max(num_reference_frames, (uint32_t)sps->num_ref_frames);
		}

		num_reference_frames = std::min(num_reference_frames, video_capability_h264.video_capabilities.maxActiveReferencePictures);

		VkVideoDecodeH264SessionParametersAddInfoKHR session_parameters_add_info_h264 = {};
		session_parameters_add_info_h264.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR;
		session_parameters_add_info_h264.stdPPSCount = (uint32_t)desc->pps_count;
		session_parameters_add_info_h264.pStdPPSs = pps_array_h264.data();
		session_parameters_add_info_h264.stdSPSCount = (uint32_t)desc->sps_count;
		session_parameters_add_info_h264.pStdSPSs = sps_array_h264.data();

		VkVideoSessionCreateInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR;
		info.queueFamilyIndex = videoFamily;
		info.maxActiveReferencePictures = num_reference_frames * 2; // *2: top and bottom field counts as two I think: https://vulkan.lunarg.com/doc/view/1.3.239.0/windows/1.3-extensions/vkspec.html#_video_decode_commands
		info.maxDpbSlots = std::min(desc->num_dpb_slots, video_capability_h264.video_capabilities.maxDpbSlots);
		info.maxCodedExtent.width = std::min(desc->width, video_capability_h264.video_capabilities.maxCodedExtent.width);
		info.maxCodedExtent.height = std::min(desc->height, video_capability_h264.video_capabilities.maxCodedExtent.height);
		info.pictureFormat = _ConvertFormat(desc->format);
		info.referencePictureFormat = info.pictureFormat;
		info.pVideoProfile = &video_capability_h264.profile;
		info.pStdHeaderVersion = &video_capability_h264.video_capabilities.stdHeaderVersion;
		
		res = vkCreateVideoSessionKHR(device, &info, nullptr, &internal_state->video_session);
		assert(res == VK_SUCCESS);

		uint32_t requirement_count = 0;
		res = vkGetVideoSessionMemoryRequirementsKHR(device, internal_state->video_session, &requirement_count, nullptr);
		assert(res == VK_SUCCESS);

		wi::vector<VkVideoSessionMemoryRequirementsKHR> requirements(requirement_count);
		for (auto& x : requirements)
		{
			x.sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_MEMORY_REQUIREMENTS_KHR;
		}
		res = vkGetVideoSessionMemoryRequirementsKHR(device, internal_state->video_session, &requirement_count, requirements.data());
		assert(res == VK_SUCCESS);

		internal_state->allocations.resize(requirement_count);
		wi::vector<VkBindVideoSessionMemoryInfoKHR> bind_session_memory_infos(requirement_count);
		for (uint32_t i = 0; i < requirement_count; ++i)
		{
			const VkVideoSessionMemoryRequirementsKHR& video_req = requirements[i];
			VmaAllocationCreateInfo alloc_create_info = {};
			alloc_create_info.memoryTypeBits = video_req.memoryRequirements.memoryTypeBits;
			VmaAllocationInfo alloc_info = {};

			res = vmaAllocateMemory(
				allocationhandler->allocator,
				&video_req.memoryRequirements,
				&alloc_create_info,
				&internal_state->allocations[i],
				&alloc_info
			);
			assert(res == VK_SUCCESS);

			VkBindVideoSessionMemoryInfoKHR& bind_info = bind_session_memory_infos[i];
			bind_info.sType = VK_STRUCTURE_TYPE_BIND_VIDEO_SESSION_MEMORY_INFO_KHR;
			bind_info.memory = alloc_info.deviceMemory;
			bind_info.memoryBindIndex = video_req.memoryBindIndex;
			bind_info.memoryOffset = alloc_info.offset;
			bind_info.memorySize = alloc_info.size;
		}
		res = vkBindVideoSessionMemoryKHR(device, internal_state->video_session, requirement_count, bind_session_memory_infos.data());
		assert(res == VK_SUCCESS);

		VkVideoDecodeH264SessionParametersCreateInfoKHR session_parameters_info_h264 = {};
		session_parameters_info_h264.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR;
		session_parameters_info_h264.maxStdPPSCount = (uint32_t)desc->pps_count;
		session_parameters_info_h264.maxStdSPSCount = (uint32_t)desc->sps_count;
		session_parameters_info_h264.pParametersAddInfo = &session_parameters_add_info_h264;

		VkVideoSessionParametersCreateInfoKHR session_parameters_info = {};
		session_parameters_info.sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR;
		session_parameters_info.videoSession = internal_state->video_session;
		session_parameters_info.videoSessionParametersTemplate = VK_NULL_HANDLE;
		session_parameters_info.pNext = &session_parameters_info_h264;
		res = vkCreateVideoSessionParametersKHR(device, &session_parameters_info, nullptr, &internal_state->session_parameters);
		assert(res == VK_SUCCESS);

		assert(video_capability_h264.decode_capabilities.flags & VK_VIDEO_DECODE_CAPABILITY_DPB_AND_OUTPUT_COINCIDE_BIT_KHR); // Currently the only method supported

		return true;
	}

	int GraphicsDevice_Vulkan::CreateSubresource(Texture* texture, SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount, const Format* format_change, const ImageAspect* aspect, const Swizzle* swizzle, float min_lod_clamp) const
	{
		auto internal_state = to_internal(texture);

		Format format = texture->GetDesc().format;
		if (format_change != nullptr)
		{
			format = *format_change;
		}

		Texture_Vulkan::TextureSubresource subresource;
		subresource.firstMip = firstMip;
		subresource.mipCount = mipCount;
		subresource.firstSlice = firstSlice;
		subresource.sliceCount = sliceCount;

		VkImageViewCreateInfo view_desc = {};
		view_desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_desc.flags = 0;
		view_desc.image = internal_state->resource;
		view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		if (aspect != nullptr)
		{
			view_desc.subresourceRange.aspectMask = _ConvertImageAspect(*aspect);
		}
		view_desc.subresourceRange.baseArrayLayer = firstSlice;
		view_desc.subresourceRange.layerCount = sliceCount;
		view_desc.subresourceRange.baseMipLevel = firstMip;
		view_desc.subresourceRange.levelCount = mipCount;
		if (type == SubresourceType::SRV)
		{
			view_desc.components = _ConvertSwizzle(swizzle == nullptr ? texture->desc.swizzle : *swizzle);
		}
		switch (format)
		{
		case Format::NV12:
			if (view_desc.subresourceRange.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
			{
				view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
				view_desc.format = VK_FORMAT_R8_UNORM;
			}
			else if (view_desc.subresourceRange.aspectMask == VK_IMAGE_ASPECT_PLANE_0_BIT)
			{
				view_desc.format = VK_FORMAT_R8_UNORM;
			}
			else if (view_desc.subresourceRange.aspectMask == VK_IMAGE_ASPECT_PLANE_1_BIT)
			{
				view_desc.format = VK_FORMAT_R8G8_UNORM;
			}
			break;
		default:
			view_desc.format = _ConvertFormat(format);
			break;
		}

		if (texture->desc.type == TextureDesc::Type::TEXTURE_1D)
		{
			if (texture->desc.array_size > 1)
			{
				view_desc.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
			}
			else
			{
				view_desc.viewType = VK_IMAGE_VIEW_TYPE_1D;
			}
		}
		else if (texture->desc.type == TextureDesc::Type::TEXTURE_2D)
		{
			if (texture->desc.array_size > 1)
			{
				if (has_flag(texture->desc.misc_flags, ResourceMiscFlag::TEXTURECUBE))
				{
					if (texture->desc.array_size > 6 && sliceCount > 6)
					{
						view_desc.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
					}
					else
					{
						view_desc.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
					}
				}
				else
				{
					view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
				}
			}
			else
			{
				view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
			}
		}
		else if (texture->desc.type == TextureDesc::Type::TEXTURE_3D)
		{
			view_desc.viewType = VK_IMAGE_VIEW_TYPE_3D;
		}

		switch (type)
		{
		case SubresourceType::SRV:
		{
			if (IsFormatDepthSupport(format))
			{
				view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			}

			// This is required in cases where image was created with eg. USAGE_STORAGE, but
			//	the view format that we are creating doesn't support USAGE_STORAGE (for examplle: SRGB formats)
			VkImageViewUsageCreateInfo viewUsageInfo = {};
			viewUsageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
			viewUsageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
			view_desc.pNext = &viewUsageInfo;

			VkImageViewMinLodCreateInfoEXT min_lod_info = {};
			min_lod_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_MIN_LOD_CREATE_INFO_EXT;
			min_lod_info.minLod = min_lod_clamp;
			if (min_lod_clamp > 0 && image_view_min_lod_features.minLod == VK_TRUE)
			{
				viewUsageInfo.pNext = &min_lod_info;
			}

			VkResult res = vkCreateImageView(device, &view_desc, nullptr, &subresource.image_view);

			subresource.index = allocationhandler->bindlessSampledImages.allocate();
			if (subresource.index >= 0)
			{
				VkDescriptorImageInfo imageInfo = {};
				imageInfo.imageView = subresource.image_view;
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				write.dstBinding = 0;
				write.dstArrayElement = subresource.index;
				write.descriptorCount = 1;
				write.dstSet = allocationhandler->bindlessSampledImages.descriptorSet;
				write.pImageInfo = &imageInfo;
				vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
			}

			if (res == VK_SUCCESS)
			{
				if (!internal_state->srv.IsValid())
				{
					internal_state->srv = subresource;
					return -1;
				}
				internal_state->subresources_srv.push_back(subresource);
				return int(internal_state->subresources_srv.size() - 1);
			}
			else
			{
				assert(0);
			}
		}
		break;
		case SubresourceType::UAV:
		{
			if (view_desc.viewType == VK_IMAGE_VIEW_TYPE_CUBE || view_desc.viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
			{
				view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			}

			VkResult res = vkCreateImageView(device, &view_desc, nullptr, &subresource.image_view);

			subresource.index = allocationhandler->bindlessStorageImages.allocate();
			if (subresource.index >= 0)
			{
				VkDescriptorImageInfo imageInfo = {};
				imageInfo.imageView = subresource.image_view;
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				write.dstBinding = 0;
				write.dstArrayElement = subresource.index;
				write.descriptorCount = 1;
				write.dstSet = allocationhandler->bindlessStorageImages.descriptorSet;
				write.pImageInfo = &imageInfo;
				vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
			}

			if (res == VK_SUCCESS)
			{
				if (!internal_state->uav.IsValid())
				{
					internal_state->uav = subresource;
					return -1;
				}
				internal_state->subresources_uav.push_back(subresource);
				return int(internal_state->subresources_uav.size() - 1);
			}
			else
			{
				assert(0);
			}
		}
		break;
		case SubresourceType::RTV:
		{
			view_desc.subresourceRange.levelCount = 1;
			VkResult res = vkCreateImageView(device, &view_desc, nullptr, &subresource.image_view);

			if (res == VK_SUCCESS)
			{
				if (!internal_state->rtv.IsValid())
				{
					internal_state->rtv = subresource;
					internal_state->framebuffer_layercount = view_desc.subresourceRange.layerCount;
					return -1;
				}
				internal_state->subresources_rtv.push_back(subresource);
				return int(internal_state->subresources_rtv.size() - 1);
			}
			else
			{
				assert(0);
			}
		}
		break;
		case SubresourceType::DSV:
		{
			view_desc.subresourceRange.levelCount = 1;
			view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			VkResult res = vkCreateImageView(device, &view_desc, nullptr, &subresource.image_view);

			if (res == VK_SUCCESS)
			{
				if (!internal_state->dsv.IsValid())
				{
					internal_state->dsv = subresource;
					internal_state->framebuffer_layercount = view_desc.subresourceRange.layerCount;
					return -1;
				}
				internal_state->subresources_dsv.push_back(subresource);
				return int(internal_state->subresources_dsv.size() - 1);
			}
			else
			{
				assert(0);
			}
		}
		break;
		default:
			break;
		}
		return -1;
	}
	int GraphicsDevice_Vulkan::CreateSubresource(GPUBuffer* buffer, SubresourceType type, uint64_t offset, uint64_t size, const Format* format_change, const uint32_t* structuredbuffer_stride_change) const
	{
		auto internal_state = to_internal(buffer);
		const GPUBufferDesc& desc = buffer->GetDesc();
		VkResult res;

		Buffer_Vulkan::BufferSubresource subresource;

		Format format = desc.format;
		if (format_change != nullptr)
		{
			format = *format_change;
		}
		if (type == SubresourceType::UAV)
		{
			// RW resource can't be SRGB
			format = GetFormatNonSRGB(format);
		}

		switch (type)
		{

		case SubresourceType::SRV:
		case SubresourceType::UAV:
		{
			if (format == Format::UNKNOWN)
			{
				// Raw buffer
				subresource.index = allocationhandler->bindlessStorageBuffers.allocate();
				subresource.is_typed = false;
				if (subresource.IsValid())
				{
					subresource.buffer_info.buffer = internal_state->resource;
					subresource.buffer_info.offset = offset;
					subresource.buffer_info.range = size;

					VkWriteDescriptorSet write = {};
					write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					write.dstBinding = 0;
					write.dstArrayElement = subresource.index;
					write.descriptorCount = 1;
					write.dstSet = allocationhandler->bindlessStorageBuffers.descriptorSet;
					write.pBufferInfo = &subresource.buffer_info;
					vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
				}

				if (type == SubresourceType::SRV)
				{
					if (!internal_state->srv.IsValid())
					{
						internal_state->srv = subresource;
						return -1;
					}
					internal_state->subresources_srv.push_back(subresource);
					return int(internal_state->subresources_srv.size() - 1);
				}
				else
				{
					if (!internal_state->uav.IsValid())
					{
						internal_state->uav = subresource;
						return -1;
					}
					internal_state->subresources_uav.push_back(subresource);
					return int(internal_state->subresources_uav.size() - 1);
				}
			}
			else
			{
				// Typed buffer
				VkBufferViewCreateInfo srv_desc = {};
				srv_desc.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
				srv_desc.buffer = internal_state->resource;
				srv_desc.flags = 0;
				srv_desc.format = _ConvertFormat(format);
				srv_desc.offset = offset;
				srv_desc.range = std::min(size, (uint64_t)desc.size - srv_desc.offset);

				res = vkCreateBufferView(device, &srv_desc, nullptr, &subresource.buffer_view);

				if (res == VK_SUCCESS)
				{
					subresource.is_typed = true;

					if (type == SubresourceType::SRV)
					{
						subresource.index = allocationhandler->bindlessUniformTexelBuffers.allocate();
						if (subresource.IsValid())
						{
							VkWriteDescriptorSet write = {};
							write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
							write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
							write.dstBinding = 0;
							write.dstArrayElement = subresource.index;
							write.descriptorCount = 1;
							write.dstSet = allocationhandler->bindlessUniformTexelBuffers.descriptorSet;
							write.pTexelBufferView = &subresource.buffer_view;
							vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
						}

						if (!internal_state->srv.IsValid())
						{
							internal_state->srv = subresource;
							return -1;
						}
						internal_state->subresources_srv.push_back(subresource);
						return int(internal_state->subresources_srv.size() - 1);
					}
					else
					{
						subresource.index = allocationhandler->bindlessStorageTexelBuffers.allocate();
						if (subresource.IsValid())
						{
							VkWriteDescriptorSet write = {};
							write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
							write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
							write.dstBinding = 0;
							write.dstArrayElement = subresource.index;
							write.descriptorCount = 1;
							write.dstSet = allocationhandler->bindlessStorageTexelBuffers.descriptorSet;
							write.pTexelBufferView = &subresource.buffer_view;
							vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
						}

						if (!internal_state->uav.IsValid())
						{
							internal_state->uav = subresource;
							return -1;
						}
						internal_state->subresources_uav.push_back(subresource);
						return int(internal_state->subresources_uav.size() - 1);
					}
				}
				else
				{
					assert(0);
				}
			}
		}
		break;
		default:
			assert(0);
			break;
		}
		return -1;
	}

	void GraphicsDevice_Vulkan::DeleteSubresources(GPUResource* resource)
	{
		if (resource->IsTexture())
		{
			auto internal_state = to_internal((Texture*)resource);
			internal_state->allocationhandler->destroylocker.lock();
			internal_state->destroy_subresources();
			internal_state->allocationhandler->destroylocker.unlock();
		}
		else if (resource->IsBuffer())
		{
			auto internal_state = to_internal((GPUBuffer*)resource);
			internal_state->allocationhandler->destroylocker.lock();
			internal_state->destroy_subresources();
			internal_state->allocationhandler->destroylocker.unlock();
		}
	}

	int GraphicsDevice_Vulkan::GetDescriptorIndex(const GPUResource* resource, SubresourceType type, int subresource) const
	{
		if (resource == nullptr || !resource->IsValid())
			return -1;

		switch (type)
		{
		default:
		case SubresourceType::SRV:
			if (resource->IsBuffer())
			{
				auto internal_state = to_internal((const GPUBuffer*)resource);
				if (subresource < 0)
				{
					return internal_state->srv.index;
				}
				else
				{
					return internal_state->subresources_srv[subresource].index;
				}
			}
			else if(resource->IsTexture())
			{
				auto internal_state = to_internal((const Texture*)resource);
				if (subresource < 0)
				{
					return internal_state->srv.index;
				}
				else
				{
					return internal_state->subresources_srv[subresource].index;
				}
			}
			else if (resource->IsAccelerationStructure())
			{
				auto internal_state = to_internal((const RaytracingAccelerationStructure*)resource);
				return internal_state->index;
			}
			break;
		case SubresourceType::UAV:
			if (resource->IsBuffer())
			{
				auto internal_state = to_internal((const GPUBuffer*)resource);
				if (subresource < 0)
				{
					return internal_state->uav.index;
				}
				else
				{
					return internal_state->subresources_uav[subresource].index;
				}
			}
			else if (resource->IsTexture())
			{
				auto internal_state = to_internal((const Texture*)resource);
				if (subresource < 0)
				{
					return internal_state->uav.index;
				}
				else
				{
					return internal_state->subresources_uav[subresource].index;
				}
			}
			break;
		}

		return -1;
	}
	int GraphicsDevice_Vulkan::GetDescriptorIndex(const Sampler* sampler) const
	{
		if (sampler == nullptr || !sampler->IsValid())
			return -1;

		auto internal_state = to_internal(sampler);
		return internal_state->index;
	}

	void GraphicsDevice_Vulkan::WriteShadingRateValue(ShadingRate rate, void* dest) const
	{
		// How to compute shading rate value texel data:
		// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#primsrast-fragment-shading-rate-attachment

		switch (rate)
		{
		default:
		case ShadingRate::RATE_1X1:
			*(uint8_t*)dest = 0;
			break;
		case ShadingRate::RATE_1X2:
			*(uint8_t*)dest = 0x1;
			break;
		case ShadingRate::RATE_2X1:
			*(uint8_t*)dest = 0x4;
			break;
		case ShadingRate::RATE_2X2:
			*(uint8_t*)dest = 0x5;
			break;
		case ShadingRate::RATE_2X4:
			*(uint8_t*)dest = 0x6;
			break;
		case ShadingRate::RATE_4X2:
			*(uint8_t*)dest = 0x9;
			break;
		case ShadingRate::RATE_4X4:
			*(uint8_t*)dest = 0xa;
			break;
		}

	}
	void GraphicsDevice_Vulkan::WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) const
	{
		VkAccelerationStructureInstanceKHR tmp;
		tmp.transform = *(VkTransformMatrixKHR*)&instance->transform;
		tmp.instanceCustomIndex = instance->instance_id;
		tmp.mask = instance->instance_mask;
		tmp.instanceShaderBindingTableRecordOffset = instance->instance_contribution_to_hit_group_index;
		tmp.flags = instance->flags;

		assert(instance->bottom_level->IsAccelerationStructure());
		auto internal_state = to_internal((RaytracingAccelerationStructure*)instance->bottom_level);
		tmp.accelerationStructureReference = internal_state->as_address;

		std::memcpy(dest, &tmp, sizeof(VkAccelerationStructureInstanceKHR)); // memcpy whole structure into mapped pointer to avoid read from uncached memory
	}
	void GraphicsDevice_Vulkan::WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const
	{
		VkResult res = vkGetRayTracingShaderGroupHandlesKHR(device, to_internal(rtpso)->pipeline, group_index, 1, SHADER_IDENTIFIER_SIZE, dest);
		assert(res == VK_SUCCESS);
	}
	
	void GraphicsDevice_Vulkan::SetName(GPUResource* pResource, const char* name) const
	{
		if (!debugUtils || pResource == nullptr || !pResource->IsValid())
			return;

		VkDebugUtilsObjectNameInfoEXT info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		info.pObjectName = name;
		if (pResource->IsTexture())
		{
			info.objectType = VK_OBJECT_TYPE_IMAGE;
			info.objectHandle = (uint64_t)to_internal((const Texture*)pResource)->resource;
		}
		else if (pResource->IsBuffer())
		{
			info.objectType = VK_OBJECT_TYPE_BUFFER;
			info.objectHandle = (uint64_t)to_internal((const GPUBuffer*)pResource)->resource;
		}
		else if (pResource->IsAccelerationStructure())
		{
			info.objectType = VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR;
			info.objectHandle = (uint64_t)to_internal((const RaytracingAccelerationStructure*)pResource)->resource;
		}

		if (info.objectHandle == (uint64_t)VK_NULL_HANDLE)
			return;

		VkResult res = vkSetDebugUtilsObjectNameEXT(device, &info);
		assert(res == VK_SUCCESS);
	}
	void GraphicsDevice_Vulkan::SetName(Shader* shader, const char* name) const
	{
		if (!debugUtils || shader == nullptr || !shader->IsValid())
			return;

		VkDebugUtilsObjectNameInfoEXT info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		info.pObjectName = name;
		info.objectType = VK_OBJECT_TYPE_SHADER_MODULE;
		info.objectHandle = (uint64_t)to_internal(shader)->shaderModule;

		if (info.objectHandle == (uint64_t)VK_NULL_HANDLE)
			return;

		VkResult res = vkSetDebugUtilsObjectNameEXT(device, &info);
		assert(res == VK_SUCCESS);
	}

	CommandList GraphicsDevice_Vulkan::BeginCommandList(QUEUE_TYPE queue)
	{
		VkResult res;

		cmd_locker.lock();
		uint32_t cmd_current = cmd_count++;
		if (cmd_current >= commandlists.size())
		{
			commandlists.push_back(std::make_unique<CommandList_Vulkan>());
		}
		CommandList cmd;
		cmd.internal_state = commandlists[cmd_current].get();
		cmd_locker.unlock();

		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.reset(GetBufferIndex());
		commandlist.queue = queue;
		commandlist.id = cmd_current;

		if (commandlist.GetCommandBuffer() == VK_NULL_HANDLE)
		{
			// need to create one more command list:

			for (uint32_t buffer = 0; buffer < BUFFERCOUNT; ++buffer)
			{
				VkCommandPoolCreateInfo poolInfo = {};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				switch (queue)
				{
				case wi::graphics::QUEUE_GRAPHICS:
					poolInfo.queueFamilyIndex = graphicsFamily;
					break;
				case wi::graphics::QUEUE_COMPUTE:
					poolInfo.queueFamilyIndex = computeFamily;
					break;
				case wi::graphics::QUEUE_COPY:
					poolInfo.queueFamilyIndex = copyFamily;
					break;
				case wi::graphics::QUEUE_VIDEO_DECODE:
					poolInfo.queueFamilyIndex = videoFamily;
					break;
				default:
					assert(0); // queue type not handled
					break;
				}
				poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

				res = vkCreateCommandPool(device, &poolInfo, nullptr, &commandlist.commandPools[buffer][queue]);
				assert(res == VK_SUCCESS);

				VkCommandBufferAllocateInfo commandBufferInfo = {};
				commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				commandBufferInfo.commandBufferCount = 1;
				commandBufferInfo.commandPool = commandlist.commandPools[buffer][queue];
				commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

				res = vkAllocateCommandBuffers(device, &commandBufferInfo, &commandlist.commandBuffers[buffer][queue]);
				assert(res == VK_SUCCESS);

				commandlist.binder_pools[buffer].init(this);
			}

			commandlist.binder.init(this);
		}

		res = vkResetCommandPool(device, commandlist.GetCommandPool(), 0);
		assert(res == VK_SUCCESS);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		res = vkBeginCommandBuffer(commandlist.GetCommandBuffer(), &beginInfo);
		assert(res == VK_SUCCESS);

		if (queue == QUEUE_GRAPHICS)
		{
			vkCmdSetRasterizerDiscardEnable(commandlist.GetCommandBuffer(), VK_FALSE);

			VkViewport vp = {};
			vp.width = 1;
			vp.height = 1;
			vp.maxDepth = 1;
			vkCmdSetViewportWithCount(commandlist.GetCommandBuffer(), 1, &vp);

			VkRect2D scissor;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			scissor.extent.width = 65535;
			scissor.extent.height = 65535;
			vkCmdSetScissorWithCount(commandlist.GetCommandBuffer(), 1, &scissor);

			float blendConstants[] = { 1,1,1,1 };
			vkCmdSetBlendConstants(commandlist.GetCommandBuffer(), blendConstants);

			vkCmdSetStencilReference(commandlist.GetCommandBuffer(), VK_STENCIL_FRONT_AND_BACK, ~0u);

			if (features2.features.depthBounds == VK_TRUE)
			{
				vkCmdSetDepthBounds(commandlist.GetCommandBuffer(), 0.0f, 1.0f);
			}

			const VkDeviceSize zero = {};
			vkCmdBindVertexBuffers2(commandlist.GetCommandBuffer(), 0, 1, &nullBuffer, &zero, &zero, &zero);
		}

		return cmd;
	}
	void GraphicsDevice_Vulkan::SubmitCommandLists()
	{
		VkResult res;

		// Submit current frame:
		{
			uint32_t cmd_last = cmd_count;
			cmd_count = 0;
			for (uint32_t cmd = 0; cmd < cmd_last; ++cmd)
			{
				CommandList_Vulkan& commandlist = *commandlists[cmd].get();
				res = vkEndCommandBuffer(commandlist.GetCommandBuffer());
				assert(res == VK_SUCCESS);

				CommandQueue& queue = queues[commandlist.queue];
				const bool dependency = !commandlist.signals.empty() || !commandlist.waits.empty() || !commandlist.wait_queues.empty();

				if (dependency)
				{
					// If the current commandlist must resolve a dependency, then previous ones will be submitted before doing that:
					//	This improves GPU utilization because not the whole batch of command lists will need to synchronize, but only the one that handles it
					queue.submit(this, VK_NULL_HANDLE);
				}

				VkCommandBufferSubmitInfo& cbSubmitInfo = queue.submit_cmds.emplace_back();
				cbSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
				cbSubmitInfo.commandBuffer = commandlist.GetCommandBuffer();

				queue.swapchain_updates = commandlist.prev_swapchains;
				for (auto& swapchain : commandlist.prev_swapchains)
				{
					auto internal_state = to_internal(&swapchain);

					queue.submit_swapchains.push_back(internal_state->swapChain);
					queue.submit_swapChainImageIndices.push_back(internal_state->swapChainImageIndex);

					VkSemaphoreSubmitInfo& waitSemaphore = queue.submit_waitSemaphoreInfos.emplace_back();
					waitSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
					waitSemaphore.semaphore = internal_state->swapchainAcquireSemaphores[internal_state->swapChainAcquireSemaphoreIndex];
					waitSemaphore.value = 0; // not a timeline semaphore
					waitSemaphore.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

					queue.submit_signalSemaphores.push_back(internal_state->swapchainReleaseSemaphore);
					VkSemaphoreSubmitInfo& signalSemaphore = queue.submit_signalSemaphoreInfos.emplace_back();
					signalSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
					signalSemaphore.semaphore = internal_state->swapchainReleaseSemaphore;
					signalSemaphore.value = 0; // not a timeline semaphore
				}

				if (dependency)
				{
					for (auto& wait : commandlist.wait_queues)
					{
						CommandQueue& waitqueue = queues[wait.first];
						VkSemaphore semaphore = wait.second;

						// The WaitQueue operation will submit and signal the specified dependency queue:
						waitqueue.signal(semaphore); // signal recorded, will be executed at submit
						waitqueue.submit(this, VK_NULL_HANDLE);

						// The current queue will be waiting for the dependency queue to complete:
						queue.wait(semaphore);

						// recycle semaphore
						free_semaphore(semaphore);
					}
					commandlist.wait_queues.clear();

					for (auto& semaphore : commandlist.waits)
					{
						// Wait for command list dependency:
						queue.wait(semaphore);

						// semaphore is not recycled here, only the signals recycle themselves vecause wait will use the same
					}
					commandlist.waits.clear();

					for (auto& semaphore : commandlist.signals)
					{
						// Signal this command list's completion:
						queue.signal(semaphore);

						// recycle semaphore
						free_semaphore(semaphore);
					}
					commandlist.signals.clear();

					queue.submit(this, VK_NULL_HANDLE);
				}

				for (auto& x : commandlist.pipelines_worker)
				{
					if (pipelines_global.count(x.first) == 0)
					{
						pipelines_global[x.first] = x.second;
					}
					else
					{
						allocationhandler->destroylocker.lock();
						allocationhandler->destroyer_pipelines.push_back(std::make_pair(x.second, FRAMECOUNT));
						allocationhandler->destroylocker.unlock();
					}
				}
				commandlist.pipelines_worker.clear();
			}

			// final submits with fences:
			for (int q = 0; q < QUEUE_COUNT; ++q)
			{
				queues[q].submit(this, frame_fence[GetBufferIndex()][q]);
			}
		}

		// From here, we begin a new frame, this affects GetBufferIndex()!
		FRAMECOUNT++;

		// Initiate stalling CPU when GPU is not yet finished with next frame:
		if (FRAMECOUNT >= BUFFERCOUNT)
		{
			const uint32_t bufferindex = GetBufferIndex();
			for (int queue = 0; queue < QUEUE_COUNT; ++queue)
			{
				if (frame_fence[bufferindex][queue] == VK_NULL_HANDLE)
					continue;

				res = vkWaitForFences(device, 1, &frame_fence[bufferindex][queue], VK_TRUE, 0xFFFFFFFFFFFFFFFF);
				assert(res == VK_SUCCESS);

				res = vkResetFences(device, 1, &frame_fence[bufferindex][queue]);
				assert(res == VK_SUCCESS);
			}
		}

		allocationhandler->Update(FRAMECOUNT, BUFFERCOUNT);
	}

	void GraphicsDevice_Vulkan::WaitForGPU() const
	{
		VkResult res = vkDeviceWaitIdle(device);
		assert(res == VK_SUCCESS);
	}
	void GraphicsDevice_Vulkan::ClearPipelineStateCache()
	{
		allocationhandler->destroylocker.lock();

		pso_layout_cache_mutex.lock();
		for (auto& x : pso_layout_cache)
		{
			if (x.second.pipelineLayout) allocationhandler->destroyer_pipelineLayouts.push_back(std::make_pair(x.second.pipelineLayout, FRAMECOUNT));
			if (x.second.descriptorSetLayout) allocationhandler->destroyer_descriptorSetLayouts.push_back(std::make_pair(x.second.descriptorSetLayout, FRAMECOUNT));
		}
		pso_layout_cache.clear();
		pso_layout_cache_mutex.unlock();

		for (auto& x : pipelines_global)
		{
			allocationhandler->destroyer_pipelines.push_back(std::make_pair(x.second, FRAMECOUNT));
		}
		pipelines_global.clear();

		for (auto& x : commandlists)
		{
			for (auto& y : x->pipelines_worker)
			{
				allocationhandler->destroyer_pipelines.push_back(std::make_pair(y.second, FRAMECOUNT));
			}
			x->pipelines_worker.clear();
		}
		allocationhandler->destroylocker.unlock();

		// Destroy Vulkan pipeline cache 
		vkDestroyPipelineCache(device, pipelineCache, nullptr);
		pipelineCache = VK_NULL_HANDLE;

		VkPipelineCacheCreateInfo createInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
		createInfo.initialDataSize = 0;
		createInfo.pInitialData = nullptr;

		// Create Vulkan pipeline cache
		VkResult res = vkCreatePipelineCache(device, &createInfo, nullptr, &pipelineCache);
		assert(res == VK_SUCCESS);
	}

	Texture GraphicsDevice_Vulkan::GetBackBuffer(const SwapChain* swapchain) const
	{
		auto swapchain_internal = to_internal(swapchain);

		auto internal_state = std::make_shared<Texture_Vulkan>();
		internal_state->resource = swapchain_internal->swapChainImages[swapchain_internal->swapChainImageIndex];

		Texture result;
		result.type = GPUResource::Type::TEXTURE;
		result.internal_state = internal_state;
		result.desc.type = TextureDesc::Type::TEXTURE_2D;
		result.desc.width = swapchain_internal->swapChainExtent.width;
		result.desc.height = swapchain_internal->swapChainExtent.height;
		result.desc.format = swapchain->desc.format;
		return result;
	}
	ColorSpace GraphicsDevice_Vulkan::GetSwapChainColorSpace(const SwapChain* swapchain) const
	{
		auto internal_state = to_internal(swapchain);
		return internal_state->colorSpace;
	}
	bool GraphicsDevice_Vulkan::IsSwapChainSupportsHDR(const SwapChain* swapchain) const
	{
		auto internal_state = to_internal(swapchain);

		uint32_t formatCount;
		VkResult res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, internal_state->surface, &formatCount, nullptr);
		if (res == VK_SUCCESS)
		{
			wi::vector<VkSurfaceFormatKHR> swapchain_formats(formatCount);
			res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, internal_state->surface, &formatCount, swapchain_formats.data());
			if (res == VK_SUCCESS)
			{
				for (const auto& format : swapchain_formats)
				{
					if (format.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	void GraphicsDevice_Vulkan::SparseUpdate(QUEUE_TYPE queue, const SparseUpdateCommand* commands, uint32_t command_count)
	{
		thread_local wi::vector<VkBindSparseInfo> sparse_infos;
		struct DataPerBind
		{
			VkSparseBufferMemoryBindInfo buffer_bind_info;
			VkSparseImageOpaqueMemoryBindInfo image_opaque_bind_info;
			VkSparseImageMemoryBindInfo image_bind_info;
			wi::vector<VkSparseMemoryBind> memory_binds;
			wi::vector<VkSparseImageMemoryBind> image_memory_binds;
		};
		thread_local wi::vector<DataPerBind> sparse_binds;

		sparse_infos.resize(command_count);
		sparse_binds.resize(command_count);

		for (uint32_t i = 0; i < command_count; ++i)
		{
			const SparseUpdateCommand& in_command = commands[i];
			VkBindSparseInfo& out_info = sparse_infos[i];
			out_info = {};
			out_info.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;

			DataPerBind& out_bind = sparse_binds[i];

			VkDeviceMemory tile_pool_memory = VK_NULL_HANDLE;
			VkDeviceSize tile_pool_offset = 0;
			if (in_command.tile_pool != nullptr)
			{
				auto internal_tile_pool = to_internal(in_command.tile_pool);
				tile_pool_memory = internal_tile_pool->allocation->GetMemory();
				tile_pool_offset = internal_tile_pool->allocation->GetOffset();
			}

			out_bind.memory_binds.clear();
			out_bind.image_memory_binds.clear();

			out_bind.memory_binds.reserve(in_command.num_resource_regions);
			out_bind.image_memory_binds.reserve(in_command.num_resource_regions);

			const VkSparseMemoryBind* memory_bind_ptr = out_bind.memory_binds.data();
			const VkSparseImageMemoryBind* image_memory_bind_ptr = out_bind.image_memory_binds.data();

			if (in_command.sparse_resource->IsBuffer())
			{
				auto internal_sparse = to_internal((const GPUBuffer*)in_command.sparse_resource);

				VkSparseBufferMemoryBindInfo& info = out_bind.buffer_bind_info;
				info = {};
				info.buffer = internal_sparse->resource;
				info.pBinds = memory_bind_ptr;
				info.bindCount = in_command.num_resource_regions;
				memory_bind_ptr += in_command.num_resource_regions;

				for (uint32_t j = 0; j < in_command.num_resource_regions; ++j)
				{
					const SparseResourceCoordinate& in_coordinate = in_command.coordinates[j];
					const SparseRegionSize& in_size = in_command.sizes[j];

					const TileRangeFlags& in_flags = in_command.range_flags[j];
					uint32_t in_offset = in_command.range_start_offsets[j];
					uint32_t in_tile_count = in_command.range_tile_counts[j];
					VkSparseMemoryBind& out_memory_bind = out_bind.memory_binds.emplace_back();
					out_memory_bind = {};
					out_memory_bind.resourceOffset = in_coordinate.x * in_command.sparse_resource->sparse_page_size;
					out_memory_bind.size = in_tile_count * in_command.sparse_resource->sparse_page_size;
					if (in_flags == TileRangeFlags::Null)
					{
						out_memory_bind.memory = VK_NULL_HANDLE;
					}
					else
					{
						out_memory_bind.memory = tile_pool_memory;
						out_memory_bind.memoryOffset = tile_pool_offset + in_offset * in_command.sparse_resource->sparse_page_size;
					}
				}

				if (info.bindCount > 0)
				{
					out_info.pBufferBinds = &out_bind.buffer_bind_info;
					out_info.bufferBindCount = 1;
				}
			}
			else if (in_command.sparse_resource->IsTexture())
			{
				const Texture* sparse_texture = (const Texture*)in_command.sparse_resource;
				const TextureDesc& texture_desc = sparse_texture->GetDesc();
				auto internal_sparse = to_internal(sparse_texture);

				VkImageAspectFlags aspectMask = {};
				if (IsFormatDepthSupport(texture_desc.format))
				{
					aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(texture_desc.format))
					{
						aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				if (has_flag(texture_desc.bind_flags, BindFlag::RENDER_TARGET) ||
					has_flag(texture_desc.bind_flags, BindFlag::SHADER_RESOURCE) ||
					has_flag(texture_desc.bind_flags, BindFlag::UNORDERED_ACCESS))
				{
					aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
				}

				VkSparseImageOpaqueMemoryBindInfo& opaque_info = out_bind.image_opaque_bind_info;
				opaque_info = {};
				opaque_info.image = internal_sparse->resource;
				opaque_info.pBinds = memory_bind_ptr;
				opaque_info.bindCount = 0;

				VkSparseImageMemoryBindInfo& info = out_bind.image_bind_info;
				info = {};
				info.image = internal_sparse->resource;
				info.pBinds = image_memory_bind_ptr;
				info.bindCount = 0;

				for (uint32_t j = 0; j < in_command.num_resource_regions; ++j)
				{
					const SparseResourceCoordinate& in_coordinate = in_command.coordinates[j];
					const SparseRegionSize& in_size = in_command.sizes[j];
					const bool is_miptail = in_coordinate.mip >= internal_sparse->sparse_texture_properties.packed_mip_start;

					if (is_miptail)
					{
						opaque_info.bindCount++;
						memory_bind_ptr++;

						const TileRangeFlags& in_flags = in_command.range_flags[j];
						uint32_t in_offset = in_command.range_start_offsets[j];
						uint32_t in_tile_count = in_command.range_tile_counts[j];
						VkSparseMemoryBind& out_memory_bind = out_bind.memory_binds.emplace_back();
						out_memory_bind = {};
						out_memory_bind.resourceOffset = internal_sparse->sparse_texture_properties.packed_mip_tile_offset * sparse_texture->sparse_page_size;
						out_memory_bind.size = in_tile_count * in_command.sparse_resource->sparse_page_size;
						if (in_flags == TileRangeFlags::Null)
						{
							out_memory_bind.memory = VK_NULL_HANDLE;
						}
						else
						{
							out_memory_bind.memory = tile_pool_memory;
							out_memory_bind.memoryOffset = tile_pool_offset + in_offset * in_command.sparse_resource->sparse_page_size;
						}
					}
					else
					{
						info.bindCount++;
						image_memory_bind_ptr++;

						const TileRangeFlags& in_flags = in_command.range_flags[j];
						uint32_t in_offset = in_command.range_start_offsets[j];
						uint32_t in_tile_count = in_command.range_tile_counts[j];
						VkSparseImageMemoryBind& out_image_memory_bind = out_bind.image_memory_binds.emplace_back();
						out_image_memory_bind = {};
						if (in_flags == TileRangeFlags::Null)
						{
							out_image_memory_bind.memory = VK_NULL_HANDLE;
						}
						else
						{
							out_image_memory_bind.memory = tile_pool_memory;
							out_image_memory_bind.memoryOffset = tile_pool_offset + in_offset * in_command.sparse_resource->sparse_page_size;
						}
						out_image_memory_bind.subresource.mipLevel = in_coordinate.mip;
						out_image_memory_bind.subresource.arrayLayer = in_coordinate.slice;
						out_image_memory_bind.subresource.aspectMask = aspectMask;
						out_image_memory_bind.offset.x = in_coordinate.x * internal_sparse->sparse_texture_properties.tile_width;
						out_image_memory_bind.offset.y = in_coordinate.y * internal_sparse->sparse_texture_properties.tile_height;
						out_image_memory_bind.offset.z = in_coordinate.z * internal_sparse->sparse_texture_properties.tile_depth;
						out_image_memory_bind.extent.width = std::min(texture_desc.width, in_size.width * internal_sparse->sparse_texture_properties.tile_width);
						out_image_memory_bind.extent.height = std::min(texture_desc.height, in_size.height * internal_sparse->sparse_texture_properties.tile_height);
						out_image_memory_bind.extent.depth = std::min(texture_desc.depth, in_size.depth * internal_sparse->sparse_texture_properties.tile_depth);
					}

				}

				if (opaque_info.bindCount > 0)
				{
					out_info.pImageOpaqueBinds = &out_bind.image_opaque_bind_info;
					out_info.imageOpaqueBindCount = 1;
				}
				if (info.bindCount > 0)
				{
					out_info.pImageBinds = &out_bind.image_bind_info;
					out_info.imageBindCount = 1;
				}

			}

		}

		// Queue command:
		{
			CommandQueue& q = queues[queue];
			std::scoped_lock lock(*q.locker);
			assert(q.sparse_binding_supported);

			VkResult res = vkQueueBindSparse(q.queue, (uint32_t)sparse_infos.size(), sparse_infos.data(), VK_NULL_HANDLE);
			assert(res == VK_SUCCESS);
		}
	}

	void GraphicsDevice_Vulkan::WaitCommandList(CommandList cmd, CommandList wait_for)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		CommandList_Vulkan& commandlist_wait_for = GetCommandList(wait_for);
		assert(commandlist_wait_for.id < commandlist.id); // can't wait for future command list!
		VkSemaphore semaphore = new_semaphore();
		commandlist.waits.push_back(semaphore);
		commandlist_wait_for.signals.push_back(semaphore);
	}
	void GraphicsDevice_Vulkan::WaitQueue(CommandList cmd, QUEUE_TYPE wait_for)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.wait_queues.push_back(std::make_pair(wait_for, new_semaphore()));
	}
	void GraphicsDevice_Vulkan::RenderPassBegin(const SwapChain* swapchain, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.renderpass_barriers_begin.clear();
		commandlist.renderpass_barriers_end.clear();
		auto internal_state = to_internal(swapchain);

		internal_state->swapChainAcquireSemaphoreIndex = (internal_state->swapChainAcquireSemaphoreIndex + 1) % internal_state->swapchainAcquireSemaphores.size();

		internal_state->locker.lock();
		VkResult res = vkAcquireNextImageKHR(
			device,
			internal_state->swapChain,
			UINT64_MAX,
			internal_state->swapchainAcquireSemaphores[internal_state->swapChainAcquireSemaphoreIndex],
			VK_NULL_HANDLE,
			&internal_state->swapChainImageIndex
		);
		internal_state->locker.unlock();

		if (res != VK_SUCCESS)
		{
			// Handle outdated error in acquire:
			if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
			{
				// we need to create a new semaphore or jump through a few hoops to
				// wait for the current one to be unsignalled before we can use it again
				// creating a new one is easiest. See also:
				// https://github.com/KhronosGroup/Vulkan-Docs/issues/152
				// https://www.khronos.org/blog/resolving-longstanding-issues-with-wsi
				{
					std::scoped_lock lock(allocationhandler->destroylocker);
					for (auto& x : internal_state->swapchainAcquireSemaphores)
					{
						allocationhandler->destroyer_semaphores.emplace_back(x, allocationhandler->framecount);
					}
				}
				internal_state->swapchainAcquireSemaphores.clear();
				if (CreateSwapChainInternal(internal_state, physicalDevice, device, allocationhandler))
				{
					RenderPassBegin(swapchain, cmd);
					return;
				}
			}
			assert(0);
		}
		commandlist.prev_swapchains.push_back(*swapchain);
		
		VkRenderingInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		info.renderArea.offset.x = 0;
		info.renderArea.offset.y = 0;
		info.renderArea.extent.width = std::min(swapchain->desc.width, internal_state->swapChainExtent.width);
		info.renderArea.extent.height = std::min(swapchain->desc.height, internal_state->swapChainExtent.height);
		info.layerCount = 1;

		VkRenderingAttachmentInfo color_attachment = {};
		color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		color_attachment.imageView = internal_state->swapChainImageViews[internal_state->swapChainImageIndex];
		color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.clearValue.color.float32[0] = swapchain->desc.clear_color[0];
		color_attachment.clearValue.color.float32[1] = swapchain->desc.clear_color[1];
		color_attachment.clearValue.color.float32[2] = swapchain->desc.clear_color[2];
		color_attachment.clearValue.color.float32[3] = swapchain->desc.clear_color[3];

		info.colorAttachmentCount = 1;
		info.pColorAttachments = &color_attachment;

		VkImageMemoryBarrier2 barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.image = internal_state->swapChainImages[internal_state->swapChainImageIndex];
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
		barrier.srcAccessMask = VK_ACCESS_2_NONE;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		VkDependencyInfo dependencyInfo = {};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &barrier;
		vkCmdPipelineBarrier2(commandlist.GetCommandBuffer(), &dependencyInfo);

		barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_NONE;
		commandlist.renderpass_barriers_end.push_back(barrier);

		vkCmdBeginRendering(commandlist.GetCommandBuffer(), &info);

		commandlist.renderpass_info = RenderPassInfo::from(swapchain->desc);
	}
	void GraphicsDevice_Vulkan::RenderPassBegin(const RenderPassImage* images, uint32_t image_count, CommandList cmd, RenderPassFlags flags)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.renderpass_barriers_begin.clear();
		commandlist.renderpass_barriers_end.clear();

		VkRenderingInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		if (has_flag(flags, RenderPassFlags::SUSPENDING))
		{
			info.flags |= VK_RENDERING_SUSPENDING_BIT;
		}
		if (has_flag(flags, RenderPassFlags::RESUMING))
		{
			info.flags |= VK_RENDERING_RESUMING_BIT;
		}
		info.layerCount = 1;
		info.renderArea.offset.x = 0;
		info.renderArea.offset.y = 0;
		if (image_count == 0)
		{
			// no attachments can still render (UAV only rendering)
			info.renderArea.extent.width = properties2.properties.limits.maxFramebufferWidth;
			info.renderArea.extent.height = properties2.properties.limits.maxFramebufferHeight;
		}
		VkRenderingAttachmentInfo color_attachments[8] = {};
		VkRenderingAttachmentInfo depth_attachment = {};
		VkRenderingAttachmentInfo stencil_attachment = {};
		VkRenderingFragmentShadingRateAttachmentInfoKHR shading_rate_attachment = {};
		bool color = false;
		bool depth = false;
		bool stencil = false;
		uint32_t color_resolve_count = 0;
		for (uint32_t i = 0; i < image_count; ++i)
		{
			const RenderPassImage& image = images[i];
			const Texture* texture = image.texture;
			const TextureDesc& desc = texture->GetDesc();
			int subresource = image.subresource;
			auto internal_state = to_internal(texture);

			info.renderArea.extent.width = std::max(info.renderArea.extent.width, desc.width);
			info.renderArea.extent.height = std::max(info.renderArea.extent.height, desc.height);

			VkAttachmentLoadOp loadOp;
			switch (image.loadop)
			{
			default:
			case RenderPassImage::LoadOp::LOAD:
				loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
				break;
			case RenderPassImage::LoadOp::CLEAR:
				loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				break;
			case RenderPassImage::LoadOp::DONTCARE:
				loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				break;
			}

			VkAttachmentStoreOp storeOp;
			switch (image.storeop)
			{
			default:
			case RenderPassImage::StoreOp::STORE:
				storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				break;
			case RenderPassImage::StoreOp::DONTCARE:
				storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				break;
			}

			Texture_Vulkan::TextureSubresource descriptor;

			switch (image.type)
			{
			case RenderPassImage::Type::RENDERTARGET:
			{
				descriptor = subresource < 0 ? internal_state->rtv : internal_state->subresources_rtv[subresource];
				VkRenderingAttachmentInfo& color_attachment = color_attachments[info.colorAttachmentCount++];
				color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				color_attachment.imageView = descriptor.image_view;
				color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				color_attachment.loadOp = loadOp;
				color_attachment.storeOp = storeOp;
				color_attachment.clearValue.color.float32[0] = desc.clear.color[0];
				color_attachment.clearValue.color.float32[1] = desc.clear.color[1];
				color_attachment.clearValue.color.float32[2] = desc.clear.color[2];
				color_attachment.clearValue.color.float32[3] = desc.clear.color[3];
				color = true;
			}
			break;

			case RenderPassImage::Type::RESOLVE:
			{
				descriptor = subresource < 0 ? internal_state->srv : internal_state->subresources_srv[subresource];
				VkRenderingAttachmentInfo& color_attachment = color_attachments[color_resolve_count++];
				color_attachment.resolveImageView = descriptor.image_view;
				color_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				color_attachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
			}
			break;

			case RenderPassImage::Type::DEPTH_STENCIL:
			{
				descriptor = subresource < 0 ? internal_state->dsv : internal_state->subresources_dsv[subresource];
				depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				depth_attachment.imageView = descriptor.image_view;
				if (image.layout == ResourceState::DEPTHSTENCIL_READONLY)
				{
					depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
				}
				else
				{
					depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
				}
				depth_attachment.loadOp = loadOp;
				depth_attachment.storeOp = storeOp;
				depth_attachment.clearValue.depthStencil.depth = desc.clear.depth_stencil.depth;
				depth = true;
				if (IsFormatStencilSupport(desc.format))
				{
					stencil_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
					stencil_attachment.imageView = subresource < 0 ? internal_state->dsv.image_view : internal_state->subresources_dsv[subresource].image_view;
					if (image.layout == ResourceState::DEPTHSTENCIL_READONLY)
					{
						stencil_attachment.imageLayout = VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
					}
					else
					{
						stencil_attachment.imageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
					}
					stencil_attachment.loadOp = loadOp;
					stencil_attachment.storeOp = storeOp;
					stencil_attachment.clearValue.depthStencil.stencil = desc.clear.depth_stencil.stencil;
					stencil = true;
				}
			}
			break;

			case RenderPassImage::Type::RESOLVE_DEPTH:
			{
				descriptor = subresource < 0 ? internal_state->dsv : internal_state->subresources_dsv[subresource];
				depth_attachment.resolveImageView = descriptor.image_view;
				stencil_attachment.resolveImageView = descriptor.image_view;
				depth_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
				stencil_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
				switch (image.depth_resolve_mode)
				{
				default:
				case RenderPassImage::DepthResolveMode::Min:
					depth_attachment.resolveMode = VK_RESOLVE_MODE_MIN_BIT;
					stencil_attachment.resolveMode = VK_RESOLVE_MODE_MIN_BIT;
					break;
				case RenderPassImage::DepthResolveMode::Max:
					depth_attachment.resolveMode = VK_RESOLVE_MODE_MAX_BIT;
					stencil_attachment.resolveMode = VK_RESOLVE_MODE_MAX_BIT;
					break;
				}
			}
			break;

			case RenderPassImage::Type::SHADING_RATE_SOURCE:
				descriptor = subresource < 0 ? internal_state->uav : internal_state->subresources_uav[subresource];
				shading_rate_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
				shading_rate_attachment.imageView = descriptor.image_view;
				shading_rate_attachment.imageLayout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
				shading_rate_attachment.shadingRateAttachmentTexelSize.width = VARIABLE_RATE_SHADING_TILE_SIZE;
				shading_rate_attachment.shadingRateAttachmentTexelSize.height = VARIABLE_RATE_SHADING_TILE_SIZE;
				info.pNext = &shading_rate_attachment;
				break;
			default:
				break;
			}

			if (image.layout_before != image.layout)
			{
				VkImageMemoryBarrier2& barrier = commandlist.renderpass_barriers_begin.emplace_back();
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				barrier.image = internal_state->resource;
				barrier.oldLayout = _ConvertImageLayout(image.layout_before);
				barrier.newLayout = _ConvertImageLayout(image.layout);

				assert(barrier.newLayout != VK_IMAGE_LAYOUT_UNDEFINED);
				
				barrier.srcStageMask = _ConvertPipelineStage(image.layout_before);
				barrier.dstStageMask = _ConvertPipelineStage(image.layout);
				barrier.srcAccessMask = _ParseResourceState(image.layout_before);
				barrier.dstAccessMask = _ParseResourceState(image.layout);

				if (IsFormatDepthSupport(desc.format))
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(desc.format))
					{
						barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				barrier.subresourceRange.baseMipLevel = descriptor.firstMip;
				barrier.subresourceRange.levelCount = descriptor.mipCount;
				barrier.subresourceRange.baseArrayLayer = descriptor.firstSlice;
				barrier.subresourceRange.layerCount = descriptor.sliceCount;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			}

			if (image.layout != image.layout_after)
			{
				VkImageMemoryBarrier2& barrier = commandlist.renderpass_barriers_end.emplace_back();
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				barrier.image = internal_state->resource;
				barrier.oldLayout = _ConvertImageLayout(image.layout);
				barrier.newLayout = _ConvertImageLayout(image.layout_after);

				assert(barrier.newLayout != VK_IMAGE_LAYOUT_UNDEFINED);

				barrier.srcStageMask = _ConvertPipelineStage(image.layout);
				barrier.dstStageMask = _ConvertPipelineStage(image.layout_after);
				barrier.srcAccessMask = _ParseResourceState(image.layout);
				barrier.dstAccessMask = _ParseResourceState(image.layout_after);

				if (IsFormatDepthSupport(desc.format))
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(desc.format))
					{
						barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				barrier.subresourceRange.baseMipLevel = descriptor.firstMip;
				barrier.subresourceRange.levelCount = descriptor.mipCount;
				barrier.subresourceRange.baseArrayLayer = descriptor.firstSlice;
				barrier.subresourceRange.layerCount = descriptor.sliceCount;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			}

			info.layerCount = std::min(desc.array_size, std::max(info.layerCount, descriptor.sliceCount));
		}
		info.pColorAttachments = color ? color_attachments : nullptr;
		info.pDepthAttachment = depth ? &depth_attachment : nullptr;
		info.pStencilAttachment = stencil ? &stencil_attachment : nullptr;

		if (!commandlist.renderpass_barriers_begin.empty())
		{
			VkDependencyInfo dependencyInfo = {};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(commandlist.renderpass_barriers_begin.size());
			dependencyInfo.pImageMemoryBarriers = commandlist.renderpass_barriers_begin.data();

			vkCmdPipelineBarrier2(commandlist.GetCommandBuffer(), &dependencyInfo);
		}

		vkCmdBeginRendering(commandlist.GetCommandBuffer(), &info);

		commandlist.renderpass_info = RenderPassInfo::from(images, image_count);
	}
	void GraphicsDevice_Vulkan::RenderPassEnd(CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdEndRendering(commandlist.GetCommandBuffer());

		if (!commandlist.renderpass_barriers_end.empty())
		{
			VkDependencyInfo dependencyInfo = {};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(commandlist.renderpass_barriers_end.size());
			dependencyInfo.pImageMemoryBarriers = commandlist.renderpass_barriers_end.data();

			vkCmdPipelineBarrier2(commandlist.GetCommandBuffer(), &dependencyInfo);
			commandlist.renderpass_barriers_end.clear();
		}

		commandlist.renderpass_info = {};
	}
	void GraphicsDevice_Vulkan::BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd)
	{
		assert(rects != nullptr);
		VkRect2D scissors[16];
		assert(numRects <= arraysize(scissors));
		assert(numRects <= properties2.properties.limits.maxViewports);
		for(uint32_t i = 0; i < numRects; ++i)
		{
			scissors[i].extent.width = abs(rects[i].right - rects[i].left);
			scissors[i].extent.height = abs(rects[i].top - rects[i].bottom);
			scissors[i].offset.x = std::max(0, rects[i].left);
			scissors[i].offset.y = std::max(0, rects[i].top);
		}
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdSetScissorWithCount(commandlist.GetCommandBuffer(), numRects, scissors);
	}
	void GraphicsDevice_Vulkan::BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd)
	{
		assert(pViewports != nullptr);
		VkViewport vp[16];
		assert(NumViewports < arraysize(vp));
		assert(NumViewports < properties2.properties.limits.maxViewports);
		for (uint32_t i = 0; i < NumViewports; ++i)
		{
			vp[i].x = pViewports[i].top_left_x;
			vp[i].y = pViewports[i].top_left_y + pViewports[i].height;
			vp[i].width = std::max(1.0f, pViewports[i].width); // must be > 0 according to validation layer
			vp[i].height = -pViewports[i].height;
			vp[i].minDepth = pViewports[i].min_depth;
			vp[i].maxDepth = pViewports[i].max_depth;
		}
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdSetViewportWithCount(commandlist.GetCommandBuffer(), NumViewports, vp);
	}
	void GraphicsDevice_Vulkan::BindResource(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_SRV_COUNT);
		auto& binder = commandlist.binder;
		if (binder.table.SRV[slot].internal_state != resource->internal_state || binder.table.SRV_index[slot] != subresource)
		{
			binder.table.SRV[slot] = *resource;
			binder.table.SRV_index[slot] = subresource;
			binder.dirty |= DescriptorBinder::DIRTY_DESCRIPTOR;
		}
	}
	void GraphicsDevice_Vulkan::BindResources(const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindResource(resources[i], slot + i, cmd, -1);
			}
		}
	}
	void GraphicsDevice_Vulkan::BindUAV(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_UAV_COUNT);
		auto& binder = commandlist.binder;
		if (binder.table.UAV[slot].internal_state != resource->internal_state || binder.table.UAV_index[slot] != subresource)
		{
			binder.table.UAV[slot] = *resource;
			binder.table.UAV_index[slot] = subresource;
			binder.dirty |= DescriptorBinder::DIRTY_DESCRIPTOR;
		}
	}
	void GraphicsDevice_Vulkan::BindUAVs(const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindUAV(resources[i], slot + i, cmd, -1);
			}
		}
	}
	void GraphicsDevice_Vulkan::BindSampler(const Sampler* sampler, uint32_t slot, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_SAMPLER_COUNT);
		auto& binder = commandlist.binder;
		if (binder.table.SAM[slot].internal_state != sampler->internal_state)
		{
			binder.table.SAM[slot] = *sampler;
			binder.dirty |= DescriptorBinder::DIRTY_DESCRIPTOR;
		}
	}
	void GraphicsDevice_Vulkan::BindConstantBuffer(const GPUBuffer* buffer, uint32_t slot, CommandList cmd, uint64_t offset)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_CBV_COUNT);
		auto& binder = commandlist.binder;

		if (binder.table.CBV[slot].internal_state != buffer->internal_state)
		{
			binder.table.CBV[slot] = *buffer;
			binder.dirty |= DescriptorBinder::DIRTY_DESCRIPTOR;
		}

		if (binder.table.CBV_offset[slot] != offset)
		{
			binder.table.CBV_offset[slot] = offset;
			binder.dirty |= DescriptorBinder::DIRTY_OFFSET;
		}
	}
	void GraphicsDevice_Vulkan::BindVertexBuffers(const GPUBuffer *const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint64_t* offsets, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		size_t hash = 0;

		VkDeviceSize voffsets[8] = {};
		VkDeviceSize vstrides[8] = {};
		VkBuffer vbuffers[8] = {};
		assert(count <= 8);
		for (uint32_t i = 0; i < count; ++i)
		{
			wi::helper::hash_combine(hash, strides[i]);

			if (vertexBuffers[i] == nullptr || !vertexBuffers[i]->IsValid())
			{
				vbuffers[i] = nullBuffer;
			}
			else
			{
				auto internal_state = to_internal(vertexBuffers[i]);
				vbuffers[i] = internal_state->resource;
				if (offsets != nullptr)
				{
					voffsets[i] = offsets[i];
				}
				if (strides != nullptr)
				{
					vstrides[i] = strides[i];
				}
			}
		}

		vkCmdBindVertexBuffers2(commandlist.GetCommandBuffer(), slot, count, vbuffers, voffsets, nullptr, vstrides);
	}
	void GraphicsDevice_Vulkan::BindIndexBuffer(const GPUBuffer* indexBuffer, const IndexBufferFormat format, uint64_t offset, CommandList cmd)
	{
		if (indexBuffer != nullptr)
		{
			auto internal_state = to_internal(indexBuffer);
			CommandList_Vulkan& commandlist = GetCommandList(cmd);
			vkCmdBindIndexBuffer(commandlist.GetCommandBuffer(), internal_state->resource, offset, format == IndexBufferFormat::UINT16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
		}
	}
	void GraphicsDevice_Vulkan::BindStencilRef(uint32_t value, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdSetStencilReference(commandlist.GetCommandBuffer(), VK_STENCIL_FRONT_AND_BACK, value);
	}
	void GraphicsDevice_Vulkan::BindBlendFactor(float r, float g, float b, float a, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		float blendConstants[] = { r, g, b, a };
		vkCmdSetBlendConstants(commandlist.GetCommandBuffer(), blendConstants);
	}
	void GraphicsDevice_Vulkan::BindShadingRate(ShadingRate rate, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		if (CheckCapability(GraphicsDeviceCapability::VARIABLE_RATE_SHADING) && commandlist.prev_shadingrate != rate)
		{
			commandlist.prev_shadingrate = rate;

			VkExtent2D fragmentSize;
			switch (rate)
			{
			case ShadingRate::RATE_1X1:
				fragmentSize.width = 1;
				fragmentSize.height = 1;
				break;
			case ShadingRate::RATE_1X2:
				fragmentSize.width = 1;
				fragmentSize.height = 2;
				break;
			case ShadingRate::RATE_2X1:
				fragmentSize.width = 2;
				fragmentSize.height = 1;
				break;
			case ShadingRate::RATE_2X2:
				fragmentSize.width = 2;
				fragmentSize.height = 2;
				break;
			case ShadingRate::RATE_2X4:
				fragmentSize.width = 2;
				fragmentSize.height = 4;
				break;
			case ShadingRate::RATE_4X2:
				fragmentSize.width = 4;
				fragmentSize.height = 2;
				break;
			case ShadingRate::RATE_4X4:
				fragmentSize.width = 4;
				fragmentSize.height = 4;
				break;
			default:
				break;
			}

			VkFragmentShadingRateCombinerOpKHR combiner[] = {
				VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR,
				VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR
			};

			if (fragment_shading_rate_properties.fragmentShadingRateNonTrivialCombinerOps == VK_TRUE)
			{
				if (fragment_shading_rate_features.primitiveFragmentShadingRate == VK_TRUE)
				{
					combiner[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR;
				}
				if (fragment_shading_rate_features.attachmentFragmentShadingRate == VK_TRUE)
				{
					combiner[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR;
				}
			}
			else
			{
				if (fragment_shading_rate_features.primitiveFragmentShadingRate == VK_TRUE)
				{
					combiner[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR;
				}
				if (fragment_shading_rate_features.attachmentFragmentShadingRate == VK_TRUE)
				{
					combiner[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR;
				}
			}

			vkCmdSetFragmentShadingRateKHR(
				commandlist.GetCommandBuffer(),
				&fragmentSize,
				combiner
			);
		}
	}
	void GraphicsDevice_Vulkan::BindPipelineState(const PipelineState* pso, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.active_cs = nullptr;
		commandlist.active_rt = nullptr;

		auto internal_state = to_internal(pso);

		if (internal_state->pipeline != VK_NULL_HANDLE)
		{
			if (commandlist.active_pso != pso)
			{
				vkCmdBindPipeline(commandlist.GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, internal_state->pipeline);
			}

			commandlist.prev_pipeline_hash = 0;
			commandlist.dirty_pso = false;
		}
		else
		{
			size_t pipeline_hash = 0;
			wi::helper::hash_combine(pipeline_hash, internal_state->hash);
			wi::helper::hash_combine(pipeline_hash, commandlist.renderpass_info.get_hash());
			if (commandlist.prev_pipeline_hash == pipeline_hash)
			{
				commandlist.active_pso = pso;
				return;
			}
			commandlist.prev_pipeline_hash = pipeline_hash;
			commandlist.dirty_pso = true;
		}

		if (commandlist.active_pso == nullptr)
		{
			commandlist.binder.dirty |= DescriptorBinder::DIRTY_ALL;
		}
		else
		{
			auto active_internal = to_internal(commandlist.active_pso);
			if (internal_state->binding_hash != active_internal->binding_hash)
			{
				commandlist.binder.dirty |= DescriptorBinder::DIRTY_ALL;
			}
		}

		if (!internal_state->bindlessSets.empty())
		{
			vkCmdBindDescriptorSets(
				commandlist.GetCommandBuffer(),
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				internal_state->pipelineLayout,
				internal_state->bindlessFirstSet,
				(uint32_t)internal_state->bindlessSets.size(),
				internal_state->bindlessSets.data(),
				0,
				nullptr
			);
		}

		commandlist.active_pso = pso;
	}
	void GraphicsDevice_Vulkan::BindComputeShader(const Shader* cs, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		if (commandlist.active_cs == cs)
		{
			return;
		}
		commandlist.active_pso = nullptr;
		commandlist.active_rt = nullptr;

		assert(cs->stage == ShaderStage::CS || cs->stage == ShaderStage::LIB);

		if (commandlist.active_cs == nullptr)
		{
			commandlist.binder.dirty |= DescriptorBinder::DIRTY_ALL;
		}
		else
		{
			auto internal_state = to_internal(cs);
			auto active_internal = to_internal(commandlist.active_cs);
			if (internal_state->binding_hash != active_internal->binding_hash)
			{
				commandlist.binder.dirty |= DescriptorBinder::DIRTY_ALL;
			}
		}

		commandlist.active_cs = cs;
		auto internal_state = to_internal(cs);

		if (cs->stage == ShaderStage::CS)
		{
			vkCmdBindPipeline(commandlist.GetCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, internal_state->pipeline_cs);

			if (!internal_state->bindlessSets.empty())
			{
				vkCmdBindDescriptorSets(
					commandlist.GetCommandBuffer(),
					VK_PIPELINE_BIND_POINT_COMPUTE,
					internal_state->pipelineLayout_cs,
					internal_state->bindlessFirstSet,
					(uint32_t)internal_state->bindlessSets.size(),
					internal_state->bindlessSets.data(),
					0,
					nullptr
				);
			}
		}
		else if (cs->stage == ShaderStage::LIB)
		{
			if (!internal_state->bindlessSets.empty())
			{
				vkCmdBindDescriptorSets(
					commandlist.GetCommandBuffer(),
					VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
					internal_state->pipelineLayout_cs,
					internal_state->bindlessFirstSet,
					(uint32_t)internal_state->bindlessSets.size(),
					internal_state->bindlessSets.data(),
					0,
					nullptr
				);
			}
		}
	}
	void GraphicsDevice_Vulkan::BindDepthBounds(float min_bounds, float max_bounds, CommandList cmd)
	{
		if (features2.features.depthBounds == VK_TRUE)
		{
			CommandList_Vulkan& commandlist = GetCommandList(cmd);
			vkCmdSetDepthBounds(commandlist.GetCommandBuffer(), min_bounds, max_bounds);
		}
	}
	void GraphicsDevice_Vulkan::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDraw(commandlist.GetCommandBuffer(), vertexCount, 1, startVertexLocation, 0);
	}
	void GraphicsDevice_Vulkan::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndexed(commandlist.GetCommandBuffer(), indexCount, 1, startIndexLocation, baseVertexLocation, 0);
	}
	void GraphicsDevice_Vulkan::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDraw(commandlist.GetCommandBuffer(), vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_Vulkan::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndexed(commandlist.GetCommandBuffer(), indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_Vulkan::DrawInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndirect(commandlist.GetCommandBuffer(), internal_state->resource, args_offset, 1, (uint32_t)sizeof(VkDrawIndirectCommand));
	}
	void GraphicsDevice_Vulkan::DrawIndexedInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndexedIndirect(commandlist.GetCommandBuffer(), internal_state->resource, args_offset, 1, sizeof(VkDrawIndexedIndirectCommand));
	}
	void GraphicsDevice_Vulkan::DrawInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndirectCount(commandlist.GetCommandBuffer(), args_internal->resource, args_offset, count_internal->resource, count_offset, max_count, sizeof(VkDrawIndirectCommand));
	}
	void GraphicsDevice_Vulkan::DrawIndexedInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndexedIndirectCount(commandlist.GetCommandBuffer(), args_internal->resource, args_offset, count_internal->resource, count_offset, max_count, sizeof(VkDrawIndexedIndirectCommand));
	}
	void GraphicsDevice_Vulkan::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predispatch(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDispatch(commandlist.GetCommandBuffer(), threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}
	void GraphicsDevice_Vulkan::DispatchIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predispatch(cmd);
		auto internal_state = to_internal(args);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDispatchIndirect(commandlist.GetCommandBuffer(), internal_state->resource, args_offset);
	}
	void GraphicsDevice_Vulkan::DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawMeshTasksEXT(commandlist.GetCommandBuffer(), threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}
	void GraphicsDevice_Vulkan::DispatchMeshIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawMeshTasksIndirectEXT(commandlist.GetCommandBuffer(), internal_state->resource, args_offset, 1, sizeof(VkDispatchIndirectCommand));
	}
	void GraphicsDevice_Vulkan::DispatchMeshIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawMeshTasksIndirectCountEXT(commandlist.GetCommandBuffer(), args_internal->resource, args_offset, count_internal->resource, count_offset, max_count, sizeof(VkDispatchIndirectCommand));
	}
	void GraphicsDevice_Vulkan::CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		if (pDst->type == GPUResource::Type::TEXTURE && pSrc->type == GPUResource::Type::TEXTURE)
		{
			auto internal_state_src = to_internal((const Texture*)pSrc);
			auto internal_state_dst = to_internal((const Texture*)pDst);

			const TextureDesc& src_desc = ((const Texture*)pSrc)->GetDesc();
			const TextureDesc& dst_desc = ((const Texture*)pDst)->GetDesc();

			if (src_desc.usage == Usage::UPLOAD)
			{
				VkBufferImageCopy copy = {};
				copy.imageSubresource.baseArrayLayer = 0;
				copy.imageSubresource.layerCount = dst_desc.array_size;
				copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				const uint32_t data_stride = GetFormatStride(dst_desc.format);
				uint32_t mip_width = dst_desc.width;
				uint32_t mip_height = dst_desc.height;
				uint32_t mip_depth = dst_desc.depth;
				for (uint32_t mip = 0; mip < dst_desc.mip_levels; ++mip)
				{
					copy.imageExtent.width = mip_width;
					copy.imageExtent.height = mip_height;
					copy.imageExtent.depth = mip_depth;
					copy.imageSubresource.mipLevel = mip;
					vkCmdCopyBufferToImage(
						commandlist.GetCommandBuffer(),
						internal_state_src->staging_resource,
						internal_state_dst->resource,
						_ConvertImageLayout(ResourceState::COPY_DST),
						1,
						&copy
					);

					copy.bufferOffset += mip_width * mip_height * mip_depth * data_stride;
					mip_width = std::max(1u, mip_width / 2);
					mip_height = std::max(1u, mip_height / 2);
					mip_depth = std::max(1u, mip_depth / 2);
				}

			}
			else if (dst_desc.usage == Usage::READBACK)
			{
				VkBufferImageCopy copy = {};
				copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				const uint32_t data_stride = GetFormatStride(dst_desc.format);
				const uint32_t block_size = GetFormatBlockSize(dst_desc.format);
				const uint32_t num_blocks_x = dst_desc.width / block_size;
				const uint32_t num_blocks_y = dst_desc.height / block_size;
				for (uint32_t slice = 0; slice < dst_desc.array_size; ++slice)
				{
					copy.imageSubresource.baseArrayLayer = slice;
					copy.imageSubresource.layerCount = 1;
					uint32_t mip_blocks_x = num_blocks_x;
					uint32_t mip_blocks_y = num_blocks_y;
					uint32_t mip_width = dst_desc.width;
					uint32_t mip_height = dst_desc.height;
					uint32_t mip_depth = dst_desc.depth;
					for (uint32_t mip = 0; mip < dst_desc.mip_levels; ++mip)
					{
						copy.imageExtent.width = mip_width;
						copy.imageExtent.height = mip_height;
						copy.imageExtent.depth = mip_depth;
						copy.imageSubresource.mipLevel = mip;
						vkCmdCopyImageToBuffer(
							commandlist.GetCommandBuffer(),
							internal_state_src->resource,
							_ConvertImageLayout(ResourceState::COPY_SRC),
							internal_state_dst->staging_resource,
							1,
							&copy
						);

						copy.bufferOffset += mip_blocks_x * mip_blocks_y * mip_depth * data_stride;
						mip_blocks_x = std::max(1u, mip_blocks_x / 2);
						mip_blocks_y = std::max(1u, mip_blocks_y / 2);
						mip_width = std::max(1u, mip_width / 2);
						mip_height = std::max(1u, mip_height / 2);
						mip_depth = std::max(1u, mip_depth / 2);
					}
				}
			}
			else
			{
				VkImageCopy copy = {};
				copy.extent.width = dst_desc.width;
				copy.extent.height = dst_desc.height;
				copy.extent.depth = std::max(1u, dst_desc.depth);

				copy.srcOffset.x = 0;
				copy.srcOffset.y = 0;
				copy.srcOffset.z = 0;

				copy.dstOffset.x = 0;
				copy.dstOffset.y = 0;
				copy.dstOffset.z = 0;

				if (IsFormatDepthSupport(src_desc.format))
				{
					copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(src_desc.format))
					{
						copy.srcSubresource.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				copy.srcSubresource.baseArrayLayer = 0;
				copy.srcSubresource.layerCount = src_desc.array_size;
				copy.srcSubresource.mipLevel = 0;

				if (has_flag(dst_desc.bind_flags, BindFlag::DEPTH_STENCIL))
				{
					copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(dst_desc.format))
					{
						copy.dstSubresource.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				copy.dstSubresource.baseArrayLayer = 0;
				copy.dstSubresource.layerCount = dst_desc.array_size;
				copy.dstSubresource.mipLevel = 0;

				vkCmdCopyImage(commandlist.GetCommandBuffer(),
					internal_state_src->resource, _ConvertImageLayout(ResourceState::COPY_SRC),
					internal_state_dst->resource, _ConvertImageLayout(ResourceState::COPY_DST),
					1, &copy
				);
			}
		}
		else if (pDst->type == GPUResource::Type::BUFFER && pSrc->type == GPUResource::Type::BUFFER)
		{
			auto internal_state_src = to_internal((const GPUBuffer*)pSrc);
			auto internal_state_dst = to_internal((const GPUBuffer*)pDst);

			const GPUBufferDesc& src_desc = ((const GPUBuffer*)pSrc)->GetDesc();
			const GPUBufferDesc& dst_desc = ((const GPUBuffer*)pDst)->GetDesc();

			VkBufferCopy copy = {};
			copy.srcOffset = 0;
			copy.dstOffset = 0;
			copy.size = std::min(src_desc.size, dst_desc.size);

			vkCmdCopyBuffer(commandlist.GetCommandBuffer(),
				internal_state_src->resource,
				internal_state_dst->resource,
				1, &copy
			);
		}
	}
	void GraphicsDevice_Vulkan::CopyBuffer(const GPUBuffer* pDst, uint64_t dst_offset, const GPUBuffer* pSrc, uint64_t src_offset, uint64_t size, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto internal_state_src = to_internal(pSrc);
		auto internal_state_dst = to_internal(pDst);

		VkBufferCopy copy = {};
		copy.srcOffset = src_offset;
		copy.dstOffset = dst_offset;
		copy.size = size;

		vkCmdCopyBuffer(commandlist.GetCommandBuffer(),
			internal_state_src->resource,
			internal_state_dst->resource,
			1, &copy
		);
	}
	void GraphicsDevice_Vulkan::CopyTexture(const Texture* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t dstMip, uint32_t dstSlice, const Texture* src, uint32_t srcMip, uint32_t srcSlice, CommandList cmd, const Box* srcbox, ImageAspect dst_aspect, ImageAspect src_aspect)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto src_internal = to_internal(src);
		auto dst_internal = to_internal(dst);

		VkImageCopy copy = {};
		copy.dstSubresource.aspectMask = _ConvertImageAspect(dst_aspect);
		copy.dstSubresource.baseArrayLayer = dstSlice;
		copy.dstSubresource.layerCount = 1;
		copy.dstSubresource.mipLevel = dstMip;
		copy.dstOffset.x = dstX;
		copy.dstOffset.y = dstY;
		copy.dstOffset.z = dstZ;

		copy.srcSubresource.aspectMask = _ConvertImageAspect(src_aspect);
		copy.srcSubresource.baseArrayLayer = srcSlice;
		copy.srcSubresource.layerCount = 1;
		copy.srcSubresource.mipLevel = srcMip;

		if (srcbox == nullptr)
		{
			copy.srcOffset.x = 0;
			copy.srcOffset.y = 0;
			copy.srcOffset.z = 0;
			if (src->desc.format == Format::NV12 && src_aspect == ImageAspect::CHROMINANCE)
			{
				copy.extent.width = std::min(dst->desc.width, src->desc.width / 2);
				copy.extent.height = std::min(dst->desc.height, src->desc.height / 2);
			}
			else
			{
				copy.extent.width = std::min(dst->desc.width, src->desc.width);
				copy.extent.height = std::min(dst->desc.height, src->desc.height);
			}
			copy.extent.depth = std::min(dst->desc.depth, src->desc.depth);

			copy.extent.width = std::max(1u, copy.extent.width >> srcMip);
			copy.extent.height = std::max(1u, copy.extent.height >> srcMip);
			copy.extent.depth = std::max(1u, copy.extent.depth >> srcMip);
		}
		else
		{
			copy.srcOffset.x = srcbox->left;
			copy.srcOffset.y = srcbox->top;
			copy.srcOffset.z = srcbox->front;
			copy.extent.width = srcbox->right - srcbox->left;
			copy.extent.height = srcbox->bottom - srcbox->top;
			copy.extent.depth = srcbox->back - srcbox->front;
		}

		vkCmdCopyImage(
			commandlist.GetCommandBuffer(),
			src_internal->resource,
			_ConvertImageLayout(ResourceState::COPY_SRC),
			dst_internal->resource,
			_ConvertImageLayout(ResourceState::COPY_DST),
			1,
			&copy
		);
	}
	void GraphicsDevice_Vulkan::QueryBegin(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(heap);

		switch (heap->desc.type)
		{
		case GpuQueryType::OCCLUSION_BINARY:
			vkCmdBeginQuery(commandlist.GetCommandBuffer(), internal_state->pool, index, 0);
			break;
		case GpuQueryType::OCCLUSION:
			vkCmdBeginQuery(commandlist.GetCommandBuffer(), internal_state->pool, index, VK_QUERY_CONTROL_PRECISE_BIT);
			break;
		case GpuQueryType::TIMESTAMP:
			break;
		}
	}
	void GraphicsDevice_Vulkan::QueryEnd(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(heap);

		switch (heap->desc.type)
		{
		case GpuQueryType::TIMESTAMP:
			vkCmdWriteTimestamp2(commandlist.GetCommandBuffer(), VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, internal_state->pool, index);
			break;
		case GpuQueryType::OCCLUSION_BINARY:
		case GpuQueryType::OCCLUSION:
			vkCmdEndQuery(commandlist.GetCommandBuffer(), internal_state->pool, index);
			break;
		}
	}
	void GraphicsDevice_Vulkan::QueryResolve(const GPUQueryHeap* heap, uint32_t index, uint32_t count, const GPUBuffer* dest, uint64_t dest_offset, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		auto internal_state = to_internal(heap);
		auto dst_internal = to_internal(dest);

		VkQueryResultFlags flags = VK_QUERY_RESULT_64_BIT;
		flags |= VK_QUERY_RESULT_WAIT_BIT;

		switch (heap->desc.type)
		{
		case GpuQueryType::OCCLUSION_BINARY:
			flags |= VK_QUERY_RESULT_PARTIAL_BIT;
			break;
		default:
			break;
		}

		vkCmdCopyQueryPoolResults(
			commandlist.GetCommandBuffer(),
			internal_state->pool,
			index,
			count,
			dst_internal->resource,
			dest_offset,
			sizeof(uint64_t),
			flags
		);

	}
	void GraphicsDevice_Vulkan::QueryReset(const GPUQueryHeap* heap, uint32_t index, uint32_t count, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		auto internal_state = to_internal(heap);

		vkCmdResetQueryPool(
			commandlist.GetCommandBuffer(),
			internal_state->pool,
			index,
			count
		);

	}
	void GraphicsDevice_Vulkan::Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		auto& memoryBarriers = commandlist.frame_memoryBarriers;
		auto& imageBarriers = commandlist.frame_imageBarriers;
		auto& bufferBarriers = commandlist.frame_bufferBarriers;

		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const GPUBarrier& barrier = barriers[i];

			if (barrier.type == GPUBarrier::Type::IMAGE && (barrier.image.texture == nullptr || !barrier.image.texture->IsValid()))
				continue;
			if (barrier.type == GPUBarrier::Type::BUFFER && (barrier.buffer.buffer == nullptr || !barrier.buffer.buffer->IsValid()))
				continue;

			switch (barrier.type)
			{
			default:
			case GPUBarrier::Type::MEMORY:
			case GPUBarrier::Type::ALIASING:
			{
				VkMemoryBarrier2 barrierdesc = {};
				barrierdesc.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
				barrierdesc.pNext = nullptr;
				barrierdesc.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
				barrierdesc.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
				barrierdesc.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
				barrierdesc.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;

				if (CheckCapability(GraphicsDeviceCapability::RAYTRACING))
				{
					barrierdesc.srcStageMask |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
					barrierdesc.dstStageMask |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
					barrierdesc.srcAccessMask |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
					barrierdesc.dstAccessMask |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
				}

				if (CheckCapability(GraphicsDeviceCapability::PREDICATION))
				{
					barrierdesc.srcStageMask |= VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT;
					barrierdesc.dstStageMask |= VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT;
					barrierdesc.srcAccessMask |= VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT;
					barrierdesc.dstAccessMask |= VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT;
				}

				memoryBarriers.push_back(barrierdesc);
			}
			break;
			case GPUBarrier::Type::IMAGE:
			{
				const TextureDesc& desc = barrier.image.texture->desc;
				auto internal_state = to_internal(barrier.image.texture);

				VkImageMemoryBarrier2 barrierdesc = {};
				barrierdesc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				barrierdesc.pNext = nullptr;
				barrierdesc.image = internal_state->resource;
				barrierdesc.oldLayout = _ConvertImageLayout(barrier.image.layout_before);
				barrierdesc.newLayout = _ConvertImageLayout(barrier.image.layout_after);
				barrierdesc.srcStageMask = _ConvertPipelineStage(barrier.image.layout_before);
				barrierdesc.dstStageMask = _ConvertPipelineStage(barrier.image.layout_after);
				barrierdesc.srcAccessMask = _ParseResourceState(barrier.image.layout_before);
				barrierdesc.dstAccessMask = _ParseResourceState(barrier.image.layout_after);
				if (IsFormatDepthSupport(desc.format))
				{
					barrierdesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(desc.format))
					{
						barrierdesc.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					barrierdesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				if (barrier.image.aspect != nullptr)
				{
					barrierdesc.subresourceRange.aspectMask = _ConvertImageAspect(*barrier.image.aspect);
				}
				if (barrier.image.mip >= 0 || barrier.image.slice >= 0)
				{
					barrierdesc.subresourceRange.baseMipLevel = (uint32_t)std::max(0, barrier.image.mip);
					barrierdesc.subresourceRange.levelCount = 1;
					barrierdesc.subresourceRange.baseArrayLayer = (uint32_t)std::max(0, barrier.image.slice);
					barrierdesc.subresourceRange.layerCount = 1;
				}
				else
				{
					barrierdesc.subresourceRange.baseMipLevel = 0;
					barrierdesc.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
					barrierdesc.subresourceRange.baseArrayLayer = 0;
					barrierdesc.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
				}
				barrierdesc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrierdesc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				imageBarriers.push_back(barrierdesc);
			}
			break;
			case GPUBarrier::Type::BUFFER:
			{
				const GPUBufferDesc& desc = barrier.buffer.buffer->desc;
				auto internal_state = to_internal(barrier.buffer.buffer);

				VkBufferMemoryBarrier2 barrierdesc = {};
				barrierdesc.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
				barrierdesc.pNext = nullptr;
				barrierdesc.buffer = internal_state->resource;
				barrierdesc.size = desc.size;
				barrierdesc.offset = 0;
				barrierdesc.srcStageMask = _ConvertPipelineStage(barrier.buffer.state_before);
				barrierdesc.dstStageMask = _ConvertPipelineStage(barrier.buffer.state_after);
				barrierdesc.srcAccessMask = _ParseResourceState(barrier.buffer.state_before);
				barrierdesc.dstAccessMask = _ParseResourceState(barrier.buffer.state_after);
				barrierdesc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrierdesc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				if (has_flag(desc.misc_flags, ResourceMiscFlag::RAY_TRACING))
				{
					assert(CheckCapability(GraphicsDeviceCapability::RAYTRACING));
					barrierdesc.srcStageMask |= _ConvertPipelineStage(ResourceState::RAYTRACING_ACCELERATION_STRUCTURE);
					barrierdesc.dstStageMask |= _ConvertPipelineStage(ResourceState::RAYTRACING_ACCELERATION_STRUCTURE);
				}

				if (has_flag(desc.misc_flags, ResourceMiscFlag::PREDICATION))
				{
					assert(CheckCapability(GraphicsDeviceCapability::PREDICATION));
					barrierdesc.srcStageMask |= _ConvertPipelineStage(ResourceState::PREDICATION);
					barrierdesc.dstStageMask |= _ConvertPipelineStage(ResourceState::PREDICATION);
				}

				bufferBarriers.push_back(barrierdesc);
			}
			break;
			}
		}

		if (!memoryBarriers.empty() ||
			!bufferBarriers.empty() ||
			!imageBarriers.empty()
			)
		{
			VkDependencyInfo dependencyInfo = {};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.memoryBarrierCount = static_cast<uint32_t>(memoryBarriers.size());
			dependencyInfo.pMemoryBarriers = memoryBarriers.data();
			dependencyInfo.bufferMemoryBarrierCount = static_cast<uint32_t>(bufferBarriers.size());
			dependencyInfo.pBufferMemoryBarriers = bufferBarriers.data();
			dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(imageBarriers.size());
			dependencyInfo.pImageMemoryBarriers = imageBarriers.data();

			vkCmdPipelineBarrier2(commandlist.GetCommandBuffer(), &dependencyInfo);

			memoryBarriers.clear();
			imageBarriers.clear();
			bufferBarriers.clear();
		}
	}
	void GraphicsDevice_Vulkan::BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto dst_internal = to_internal(dst);

		VkAccelerationStructureBuildGeometryInfoKHR info = dst_internal->buildInfo;
		info.dstAccelerationStructure = dst_internal->resource;
		info.srcAccelerationStructure = VK_NULL_HANDLE;
		info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

		info.scratchData.deviceAddress = dst_internal->scratch_address;

		if (src != nullptr)
		{
			info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;

			auto src_internal = to_internal(src);
			info.srcAccelerationStructure = src_internal->resource;
		}

		commandlist.accelerationstructure_build_geometries = dst_internal->geometries; // copy!
		commandlist.accelerationstructure_build_ranges.clear();

		info.type = dst_internal->createInfo.type;
		info.geometryCount = (uint32_t)commandlist.accelerationstructure_build_geometries.size();
		commandlist.accelerationstructure_build_ranges.reserve(info.geometryCount);

		switch (dst->desc.type)
		{
		case RaytracingAccelerationStructureDesc::Type::BOTTOMLEVEL:
		{
			size_t i = 0;
			for (auto& x : dst->desc.bottom_level.geometries)
			{
				auto& geometry = commandlist.accelerationstructure_build_geometries[i];

				auto& range = commandlist.accelerationstructure_build_ranges.emplace_back();
				range = {};

				if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE)
				{
					geometry.flags |= VK_GEOMETRY_OPAQUE_BIT_KHR;
				}
				if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_NO_DUPLICATE_ANYHIT_INVOCATION)
				{
					geometry.flags |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
				}

				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type::TRIANGLES)
				{
					geometry.geometry.triangles.vertexData.deviceAddress = to_internal(&x.triangles.vertex_buffer)->address +
						x.triangles.vertex_byte_offset;

					geometry.geometry.triangles.indexData.deviceAddress = to_internal(&x.triangles.index_buffer)->address +
						x.triangles.index_offset * (x.triangles.index_format == IndexBufferFormat::UINT16 ? sizeof(uint16_t) : sizeof(uint32_t));

					if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
					{
						geometry.geometry.triangles.transformData.deviceAddress = to_internal(&x.triangles.transform_3x4_buffer)->address;
						range.transformOffset = x.triangles.transform_3x4_buffer_offset;
					}

					range.primitiveCount = x.triangles.index_count / 3;
					range.primitiveOffset = 0;
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type::PROCEDURAL_AABBS)
				{
					geometry.geometry.aabbs.data.deviceAddress = to_internal(&x.aabbs.aabb_buffer)->address;

					range.primitiveCount = x.aabbs.count;
					range.primitiveOffset = x.aabbs.offset;
				}

				i++;
			}
		}
		break;
		case RaytracingAccelerationStructureDesc::Type::TOPLEVEL:
		{
			auto& geometry = commandlist.accelerationstructure_build_geometries.back();
			geometry.geometry.instances.data.deviceAddress = to_internal(&dst->desc.top_level.instance_buffer)->address;

			auto& range = commandlist.accelerationstructure_build_ranges.emplace_back();
			range = {};
			range.primitiveCount = dst->desc.top_level.count;
			range.primitiveOffset = dst->desc.top_level.offset;
		}
		break;
		}

		info.pGeometries = commandlist.accelerationstructure_build_geometries.data();

		VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = commandlist.accelerationstructure_build_ranges.data();

		vkCmdBuildAccelerationStructuresKHR(
			commandlist.GetCommandBuffer(),
			1,
			&info,
			&pRangeInfo
		);
	}
	void GraphicsDevice_Vulkan::BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.prev_pipeline_hash = 0;
		commandlist.active_rt = rtpso;

		BindComputeShader(rtpso->desc.shader_libraries.front().shader, cmd);

		vkCmdBindPipeline(commandlist.GetCommandBuffer(), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, to_internal(rtpso)->pipeline);
	}
	void GraphicsDevice_Vulkan::DispatchRays(const DispatchRaysDesc* desc, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		predispatch(cmd);

		VkStridedDeviceAddressRegionKHR raygen = {};
		raygen.deviceAddress = desc->ray_generation.buffer ? to_internal(desc->ray_generation.buffer)->address : 0;
		raygen.deviceAddress += desc->ray_generation.offset;
		raygen.size = desc->ray_generation.size;
		raygen.stride = raygen.size; // raygen specifically must be size == stride
		
		VkStridedDeviceAddressRegionKHR miss = {};
		miss.deviceAddress = desc->miss.buffer ? to_internal(desc->miss.buffer)->address : 0;
		miss.deviceAddress += desc->miss.offset;
		miss.size = desc->miss.size;
		miss.stride = desc->miss.stride;

		VkStridedDeviceAddressRegionKHR hitgroup = {};
		hitgroup.deviceAddress = desc->hit_group.buffer ? to_internal(desc->hit_group.buffer)->address : 0;
		hitgroup.deviceAddress += desc->hit_group.offset;
		hitgroup.size = desc->hit_group.size;
		hitgroup.stride = desc->hit_group.stride;

		VkStridedDeviceAddressRegionKHR callable = {};
		callable.deviceAddress = desc->callable.buffer ? to_internal(desc->callable.buffer)->address : 0;
		callable.deviceAddress += desc->callable.offset;
		callable.size = desc->callable.size;
		callable.stride = desc->callable.stride;

		vkCmdTraceRaysKHR(
			commandlist.GetCommandBuffer(),
			&raygen,
			&miss,
			&hitgroup,
			&callable,
			desc->width, 
			desc->height, 
			desc->depth
		);
	}
	void GraphicsDevice_Vulkan::PushConstants(const void* data, uint32_t size, CommandList cmd, uint32_t offset)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		if (commandlist.active_pso != nullptr)
		{
			auto pso_internal = to_internal(commandlist.active_pso);
			if (pso_internal->pushconstants.size > 0)
			{
				vkCmdPushConstants(
					commandlist.GetCommandBuffer(),
					pso_internal->pipelineLayout,
					pso_internal->pushconstants.stageFlags,
					offset,
					size,
					data
				);
				return;
			}
			assert(0); // there was no push constant block!
		}
		if(commandlist.active_cs != nullptr)
		{
			auto cs_internal = to_internal(commandlist.active_cs);
			if (cs_internal->pushconstants.size > 0)
			{
				vkCmdPushConstants(
					commandlist.GetCommandBuffer(),
					cs_internal->pipelineLayout_cs,
					cs_internal->pushconstants.stageFlags,
					offset,
					size,
					data
				);
				return;
			}
			assert(0); // there was no push constant block!
		}
		assert(0); // there was no active pipeline!
	}
	void GraphicsDevice_Vulkan::PredicationBegin(const GPUBuffer* buffer, uint64_t offset, PredicationOp op, CommandList cmd)
	{
		if (CheckCapability(GraphicsDeviceCapability::PREDICATION))
		{
			CommandList_Vulkan& commandlist = GetCommandList(cmd);
			auto internal_state = to_internal(buffer);

			VkConditionalRenderingBeginInfoEXT info = {};
			info.sType = VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
			if (op == PredicationOp::NOT_EQUAL_ZERO)
			{
				info.flags = VK_CONDITIONAL_RENDERING_INVERTED_BIT_EXT;
			}
			info.offset = offset;
			info.buffer = internal_state->resource;
			vkCmdBeginConditionalRenderingEXT(commandlist.GetCommandBuffer(), &info);
		}
	}
	void GraphicsDevice_Vulkan::PredicationEnd(CommandList cmd)
	{
		if (CheckCapability(GraphicsDeviceCapability::PREDICATION))
		{
			CommandList_Vulkan& commandlist = GetCommandList(cmd);
			vkCmdEndConditionalRenderingEXT(commandlist.GetCommandBuffer());
		}
	}
	void GraphicsDevice_Vulkan::ClearUAV(const GPUResource* resource, uint32_t value, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		if (resource->IsBuffer())
		{
			auto internal_state = to_internal((const GPUBuffer*)resource);
			vkCmdFillBuffer(
				commandlist.GetCommandBuffer(),
				internal_state->resource,
				0,
				VK_WHOLE_SIZE,
				value
			);
		}
		else if (resource->IsTexture())
		{
			VkClearColorValue color = {};
			color.uint32[0] = value;
			color.uint32[1] = value;
			color.uint32[2] = value;
			color.uint32[3] = value;

			VkImageSubresourceRange range = {};
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseArrayLayer = 0;
			range.baseMipLevel = 0;
			range.layerCount = VK_REMAINING_ARRAY_LAYERS;
			range.levelCount = VK_REMAINING_MIP_LEVELS;

			auto internal_state = to_internal((const Texture*)resource);
			vkCmdClearColorImage(
				commandlist.GetCommandBuffer(),
				internal_state->resource,
				VK_IMAGE_LAYOUT_GENERAL, // "ClearUAV" so must be in UNORDERED_ACCESS state, that's a given
				&color,
				1,
				&range
			);
		}

	}
	void GraphicsDevice_Vulkan::VideoDecode(const VideoDecoder* video_decoder, const VideoDecodeOperation* op, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto decoder_internal = to_internal(video_decoder);
		auto stream_internal = to_internal(op->stream);
		auto dpb_internal = to_internal(op->DPB);

		const h264::SliceHeader* slice_header = (const h264::SliceHeader*)op->slice_header;
		const h264::PPS* pps = (const h264::PPS*)op->pps;
		const h264::SPS* sps = (const h264::SPS*)op->sps;

		StdVideoDecodeH264PictureInfo std_picture_info_h264 = {};
		std_picture_info_h264.pic_parameter_set_id = slice_header->pic_parameter_set_id;
		std_picture_info_h264.seq_parameter_set_id = pps->seq_parameter_set_id;
		std_picture_info_h264.frame_num = slice_header->frame_num;
		std_picture_info_h264.PicOrderCnt[0] = op->poc[0];
		std_picture_info_h264.PicOrderCnt[1] = op->poc[1];
		std_picture_info_h264.idr_pic_id = slice_header->idr_pic_id;
		std_picture_info_h264.flags.is_intra = op->frame_type == VideoFrameType::Intra ? 1 : 0;
		std_picture_info_h264.flags.is_reference = op->reference_priority > 0 ? 1 : 0;
		std_picture_info_h264.flags.IdrPicFlag = (std_picture_info_h264.flags.is_intra && std_picture_info_h264.flags.is_reference) ? 1 : 0;
		std_picture_info_h264.flags.field_pic_flag = slice_header->field_pic_flag;
		std_picture_info_h264.flags.bottom_field_flag = slice_header->bottom_field_flag;
		std_picture_info_h264.flags.complementary_field_pair = 0;

		VkVideoReferenceSlotInfoKHR reference_slot_infos[17] = {};
		VkVideoPictureResourceInfoKHR reference_slot_pictures[17] = {};
		VkVideoDecodeH264DpbSlotInfoKHR dpb_slots_h264[17] = {};
		StdVideoDecodeH264ReferenceInfo reference_infos_h264[17] = {};
		for (uint32_t i = 0; i < op->DPB->desc.array_size; ++i)
		{
			VkVideoReferenceSlotInfoKHR& slot = reference_slot_infos[i];
			VkVideoPictureResourceInfoKHR& pic = reference_slot_pictures[i];
			VkVideoDecodeH264DpbSlotInfoKHR& dpb = dpb_slots_h264[i];
			StdVideoDecodeH264ReferenceInfo& ref = reference_infos_h264[i];

			slot.sType = VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR;
			slot.pPictureResource = &pic;
			slot.slotIndex = i;
			slot.pNext = &dpb;

			pic.sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
			pic.codedOffset.x = 0;
			pic.codedOffset.y = 0;
			pic.codedExtent.width = op->DPB->desc.width;
			pic.codedExtent.height = op->DPB->desc.height;
			pic.baseArrayLayer = i;
			pic.imageViewBinding = dpb_internal->video_decode_view;

			dpb.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_DPB_SLOT_INFO_KHR;
			dpb.pStdReferenceInfo = &ref;

			ref.flags.bottom_field_flag = 0;
			ref.flags.top_field_flag = 0;
			ref.flags.is_non_existing = 0;
			ref.flags.used_for_long_term_reference = 0;
			ref.FrameNum = op->dpb_framenum[i];
			ref.PicOrderCnt[0] = op->dpb_poc[i];
			ref.PicOrderCnt[1] = op->dpb_poc[i];
		}

		VkVideoReferenceSlotInfoKHR reference_slots[17] = {};
		for (size_t i = 0; i < op->dpb_reference_count; ++i)
		{
			uint32_t ref_slot = op->dpb_reference_slots[i];
			assert(ref_slot != op->current_dpb);
			reference_slots[i] = reference_slot_infos[ref_slot];
		}
		reference_slots[op->dpb_reference_count] = reference_slot_infos[op->current_dpb];
		reference_slots[op->dpb_reference_count].slotIndex = -1;

		VkVideoBeginCodingInfoKHR begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR;
		begin_info.videoSession = decoder_internal->video_session;
		begin_info.videoSessionParameters = decoder_internal->session_parameters;
		begin_info.referenceSlotCount = op->dpb_reference_count + 1; // add in the current reconstructed DPB image
		begin_info.pReferenceSlots = begin_info.referenceSlotCount == 0 ? nullptr : reference_slots;
		vkCmdBeginVideoCodingKHR(commandlist.GetCommandBuffer(), &begin_info);

		if (op->flags & VideoDecodeOperation::FLAG_SESSION_RESET)
		{
			VkVideoCodingControlInfoKHR control_info = {};
			control_info.sType = VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR;
			control_info.flags = VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR;
			vkCmdControlVideoCodingKHR(commandlist.GetCommandBuffer(), &control_info);
		}

		VkVideoDecodeInfoKHR decode_info = {};
		decode_info.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_INFO_KHR;
		decode_info.srcBuffer = stream_internal->resource;
		decode_info.srcBufferOffset = (VkDeviceSize)op->stream_offset;
		decode_info.srcBufferRange = (VkDeviceSize)AlignTo(op->stream_size, VIDEO_DECODE_BITSTREAM_ALIGNMENT);
		decode_info.dstPictureResource = *reference_slot_infos[op->current_dpb].pPictureResource;
		decode_info.referenceSlotCount = op->dpb_reference_count;
		decode_info.pReferenceSlots = decode_info.referenceSlotCount == 0 ? nullptr : reference_slots;
		decode_info.pSetupReferenceSlot = &reference_slot_infos[op->current_dpb];

		uint32_t slice_offset = 0;

		// https://vulkan.lunarg.com/doc/view/1.3.239.0/windows/1.3-extensions/vkspec.html#_h_264_decoding_parameters
		VkVideoDecodeH264PictureInfoKHR picture_info_h264 = {};
		picture_info_h264.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PICTURE_INFO_KHR;
		picture_info_h264.pStdPictureInfo = &std_picture_info_h264;
		picture_info_h264.sliceCount = 1;
		picture_info_h264.pSliceOffsets = &slice_offset;
		decode_info.pNext = &picture_info_h264;

		vkCmdDecodeVideoKHR(commandlist.GetCommandBuffer(), &decode_info);

		VkVideoEndCodingInfoKHR end_info = {};
		end_info.sType = VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR;
		vkCmdEndVideoCodingKHR(commandlist.GetCommandBuffer(), &end_info);
	}

	void GraphicsDevice_Vulkan::EventBegin(const char* name, CommandList cmd)
	{
		if (!debugUtils)
			return;
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
		label.pLabelName = name;
		label.color[0] = 0.0f;
		label.color[1] = 0.0f;
		label.color[2] = 0.0f;
		label.color[3] = 1.0f;
		vkCmdBeginDebugUtilsLabelEXT(commandlist.GetCommandBuffer(), &label);
	}
	void GraphicsDevice_Vulkan::EventEnd(CommandList cmd)
	{
		if (!debugUtils)
			return;
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		vkCmdEndDebugUtilsLabelEXT(commandlist.GetCommandBuffer());
	}
	void GraphicsDevice_Vulkan::SetMarker(const char* name, CommandList cmd)
	{
		if (!debugUtils)
			return;
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		VkDebugUtilsLabelEXT label { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
		label.pLabelName = name;
		label.color[0] = 0.0f;
		label.color[1] = 0.0f;
		label.color[2] = 0.0f;
		label.color[3] = 1.0f;
		vkCmdInsertDebugUtilsLabelEXT(commandlist.GetCommandBuffer(), &label);
	}

}

#endif // WICKEDENGINE_BUILD_VULKAN
