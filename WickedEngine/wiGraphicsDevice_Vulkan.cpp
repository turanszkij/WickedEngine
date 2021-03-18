#include "wiGraphicsDevice_Vulkan.h"

#ifdef WICKEDENGINE_BUILD_VULKAN

#define VOLK_IMPLEMENTATION
#include "Utility/volk.h"

#include "Utility/spirv_reflect.h"

#include "wiGraphicsDevice_SharedInternals.h"
#include "wiHelper.h"
#include "wiBackLog.h"
#include "wiVersion.h"

#define VMA_IMPLEMENTATION
#include "Utility/vk_mem_alloc.h"

#include <sstream>
#include <vector>
#include <cstring>
#include <iostream>
#include <set>
#include <algorithm>

#ifdef SDL2
#include <SDL2/SDL_vulkan.h>
#include "sdl2.h"
#endif

// These shifts are made so that Vulkan resource bindings slots don't interfere with each other across shader stages:
//	These are also defined in compile_shaders_spirv.py as shift numbers, and it needs to be synced!
#define VULKAN_BINDING_SHIFT_B 0
#define VULKAN_BINDING_SHIFT_T 1000
#define VULKAN_BINDING_SHIFT_U 2000
#define VULKAN_BINDING_SHIFT_S 3000

namespace wiGraphics
{

namespace Vulkan_Internal
{
	// Converters:
	constexpr VkFormat _ConvertFormat(FORMAT value)
	{
		switch (value)
		{
		case FORMAT_UNKNOWN:
			return VK_FORMAT_UNDEFINED;
			break;
		case FORMAT_R32G32B32A32_FLOAT:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
			break;
		case FORMAT_R32G32B32A32_UINT:
			return VK_FORMAT_R32G32B32A32_UINT;
			break;
		case FORMAT_R32G32B32A32_SINT:
			return VK_FORMAT_R32G32B32A32_SINT;
			break;
		case FORMAT_R32G32B32_FLOAT:
			return VK_FORMAT_R32G32B32_SFLOAT;
			break;
		case FORMAT_R32G32B32_UINT:
			return VK_FORMAT_R32G32B32_UINT;
			break;
		case FORMAT_R32G32B32_SINT:
			return VK_FORMAT_R32G32B32_SINT;
			break;
		case FORMAT_R16G16B16A16_FLOAT:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
			break;
		case FORMAT_R16G16B16A16_UNORM:
			return VK_FORMAT_R16G16B16A16_UNORM;
			break;
		case FORMAT_R16G16B16A16_UINT:
			return VK_FORMAT_R16G16B16A16_UINT;
			break;
		case FORMAT_R16G16B16A16_SNORM:
			return VK_FORMAT_R16G16B16A16_SNORM;
			break;
		case FORMAT_R16G16B16A16_SINT:
			return VK_FORMAT_R16G16B16A16_SINT;
			break;
		case FORMAT_R32G32_FLOAT:
			return VK_FORMAT_R32G32_SFLOAT;
			break;
		case FORMAT_R32G32_UINT:
			return VK_FORMAT_R32G32_UINT;
			break;
		case FORMAT_R32G32_SINT:
			return VK_FORMAT_R32G32_SINT;
			break;
		case FORMAT_R32G8X24_TYPELESS:
			return VK_FORMAT_D32_SFLOAT_S8_UINT;
			break;
		case FORMAT_D32_FLOAT_S8X24_UINT:
			return VK_FORMAT_D32_SFLOAT_S8_UINT;
			break;
		case FORMAT_R10G10B10A2_UNORM:
			return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
			break;
		case FORMAT_R10G10B10A2_UINT:
			return VK_FORMAT_A2B10G10R10_UINT_PACK32;
			break;
		case FORMAT_R11G11B10_FLOAT:
			return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
			break;
		case FORMAT_R8G8B8A8_UNORM:
			return VK_FORMAT_R8G8B8A8_UNORM;
			break;
		case FORMAT_R8G8B8A8_UNORM_SRGB:
			return VK_FORMAT_R8G8B8A8_SRGB;
			break;
		case FORMAT_R8G8B8A8_UINT:
			return VK_FORMAT_R8G8B8A8_UINT;
			break;
		case FORMAT_R8G8B8A8_SNORM:
			return VK_FORMAT_R8G8B8A8_SNORM;
			break;
		case FORMAT_R8G8B8A8_SINT:
			return VK_FORMAT_R8G8B8A8_SINT;
			break;
		case FORMAT_R16G16_FLOAT:
			return VK_FORMAT_R16G16_SFLOAT;
			break;
		case FORMAT_R16G16_UNORM:
			return VK_FORMAT_R16G16_UNORM;
			break;
		case FORMAT_R16G16_UINT:
			return VK_FORMAT_R16G16_UINT;
			break;
		case FORMAT_R16G16_SNORM:
			return VK_FORMAT_R16G16_SNORM;
			break;
		case FORMAT_R16G16_SINT:
			return VK_FORMAT_R16G16_SINT;
			break;
		case FORMAT_R32_TYPELESS:
			return VK_FORMAT_D32_SFLOAT;
			break;
		case FORMAT_D32_FLOAT:
			return VK_FORMAT_D32_SFLOAT;
			break;
		case FORMAT_R32_FLOAT:
			return VK_FORMAT_R32_SFLOAT;
			break;
		case FORMAT_R32_UINT:
			return VK_FORMAT_R32_UINT;
			break;
		case FORMAT_R32_SINT:
			return VK_FORMAT_R32_SINT;
			break;
		case FORMAT_R24G8_TYPELESS:
			return VK_FORMAT_D24_UNORM_S8_UINT;
			break;
		case FORMAT_D24_UNORM_S8_UINT:
			return VK_FORMAT_D24_UNORM_S8_UINT;
			break;
		case FORMAT_R8G8_UNORM:
			return VK_FORMAT_R8G8_UNORM;
			break;
		case FORMAT_R8G8_UINT:
			return VK_FORMAT_R8G8_UINT;
			break;
		case FORMAT_R8G8_SNORM:
			return VK_FORMAT_R8G8_SNORM;
			break;
		case FORMAT_R8G8_SINT:
			return VK_FORMAT_R8G8_SINT;
			break;
		case FORMAT_R16_TYPELESS:
			return VK_FORMAT_D16_UNORM;
			break;
		case FORMAT_R16_FLOAT:
			return VK_FORMAT_R16_SFLOAT;
			break;
		case FORMAT_D16_UNORM:
			return VK_FORMAT_D16_UNORM;
			break;
		case FORMAT_R16_UNORM:
			return VK_FORMAT_R16_UNORM;
			break;
		case FORMAT_R16_UINT:
			return VK_FORMAT_R16_UINT;
			break;
		case FORMAT_R16_SNORM:
			return VK_FORMAT_R16_SNORM;
			break;
		case FORMAT_R16_SINT:
			return VK_FORMAT_R16_SINT;
			break;
		case FORMAT_R8_UNORM:
			return VK_FORMAT_R8_UNORM;
			break;
		case FORMAT_R8_UINT:
			return VK_FORMAT_R8_UINT;
			break;
		case FORMAT_R8_SNORM:
			return VK_FORMAT_R8_SNORM;
			break;
		case FORMAT_R8_SINT:
			return VK_FORMAT_R8_SINT;
			break;
		case FORMAT_BC1_UNORM:
			return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			break;
		case FORMAT_BC1_UNORM_SRGB:
			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
			break;
		case FORMAT_BC2_UNORM:
			return VK_FORMAT_BC2_UNORM_BLOCK;
			break;
		case FORMAT_BC2_UNORM_SRGB:
			return VK_FORMAT_BC2_SRGB_BLOCK;
			break;
		case FORMAT_BC3_UNORM:
			return VK_FORMAT_BC3_UNORM_BLOCK;
			break;
		case FORMAT_BC3_UNORM_SRGB:
			return VK_FORMAT_BC3_SRGB_BLOCK;
			break;
		case FORMAT_BC4_UNORM:
			return VK_FORMAT_BC4_UNORM_BLOCK;
			break;
		case FORMAT_BC4_SNORM:
			return VK_FORMAT_BC4_SNORM_BLOCK;
			break;
		case FORMAT_BC5_UNORM:
			return VK_FORMAT_BC5_UNORM_BLOCK;
			break;
		case FORMAT_BC5_SNORM:
			return VK_FORMAT_BC5_SNORM_BLOCK;
			break;
		case FORMAT_B8G8R8A8_UNORM:
			return VK_FORMAT_B8G8R8A8_UNORM;
			break;
		case FORMAT_B8G8R8A8_UNORM_SRGB:
			return VK_FORMAT_B8G8R8A8_SRGB;
			break;
		case FORMAT_BC6H_UF16:
			return VK_FORMAT_BC6H_UFLOAT_BLOCK;
			break;
		case FORMAT_BC6H_SF16:
			return VK_FORMAT_BC6H_SFLOAT_BLOCK;
			break;
		case FORMAT_BC7_UNORM:
			return VK_FORMAT_BC7_UNORM_BLOCK;
			break;
		case FORMAT_BC7_UNORM_SRGB:
			return VK_FORMAT_BC7_SRGB_BLOCK;
			break;
		}
		return VK_FORMAT_UNDEFINED;
	}
	constexpr VkCompareOp _ConvertComparisonFunc(COMPARISON_FUNC value)
	{
		switch (value)
		{
		case COMPARISON_NEVER:
			return VK_COMPARE_OP_NEVER;
			break;
		case COMPARISON_LESS:
			return VK_COMPARE_OP_LESS;
			break;
		case COMPARISON_EQUAL:
			return VK_COMPARE_OP_EQUAL;
			break;
		case COMPARISON_LESS_EQUAL:
			return VK_COMPARE_OP_LESS_OR_EQUAL;
			break;
		case COMPARISON_GREATER:
			return VK_COMPARE_OP_GREATER;
			break;
		case COMPARISON_NOT_EQUAL:
			return VK_COMPARE_OP_NOT_EQUAL;
			break;
		case COMPARISON_GREATER_EQUAL:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;
			break;
		case COMPARISON_ALWAYS:
			return VK_COMPARE_OP_ALWAYS;
			break;
		default:
			break;
		}
		return VK_COMPARE_OP_NEVER;
	}
	constexpr VkBlendFactor _ConvertBlend(BLEND value)
	{
		switch (value)
		{
		case BLEND_ZERO:
			return VK_BLEND_FACTOR_ZERO;
			break;
		case BLEND_ONE:
			return VK_BLEND_FACTOR_ONE;
			break;
		case BLEND_SRC_COLOR:
			return VK_BLEND_FACTOR_SRC_COLOR;
			break;
		case BLEND_INV_SRC_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			break;
		case BLEND_SRC_ALPHA:
			return VK_BLEND_FACTOR_SRC_ALPHA;
			break;
		case BLEND_INV_SRC_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			break;
		case BLEND_DEST_ALPHA:
			return VK_BLEND_FACTOR_DST_ALPHA;
			break;
		case BLEND_INV_DEST_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			break;
		case BLEND_DEST_COLOR:
			return VK_BLEND_FACTOR_DST_COLOR;
			break;
		case BLEND_INV_DEST_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
			break;
		case BLEND_SRC_ALPHA_SAT:
			return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
			break;
		case BLEND_BLEND_FACTOR:
			return VK_BLEND_FACTOR_CONSTANT_COLOR;
			break;
		case BLEND_INV_BLEND_FACTOR:
			return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
			break;
		case BLEND_SRC1_COLOR:
			return VK_BLEND_FACTOR_SRC1_COLOR;
			break;
		case BLEND_INV_SRC1_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
			break;
		case BLEND_SRC1_ALPHA:
			return VK_BLEND_FACTOR_SRC1_ALPHA;
			break;
		case BLEND_INV_SRC1_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
			break;
		default:
			break;
		}
		return VK_BLEND_FACTOR_ZERO;
	}
	constexpr VkBlendOp _ConvertBlendOp(BLEND_OP value)
	{
		switch (value)
		{
		case BLEND_OP_ADD:
			return VK_BLEND_OP_ADD;
			break;
		case BLEND_OP_SUBTRACT:
			return VK_BLEND_OP_SUBTRACT;
			break;
		case BLEND_OP_REV_SUBTRACT:
			return VK_BLEND_OP_REVERSE_SUBTRACT;
			break;
		case BLEND_OP_MIN:
			return VK_BLEND_OP_MIN;
			break;
		case BLEND_OP_MAX:
			return VK_BLEND_OP_MAX;
			break;
		default:
			break;
		}
		return VK_BLEND_OP_ADD;
	}
	constexpr VkSamplerAddressMode _ConvertTextureAddressMode(TEXTURE_ADDRESS_MODE value)
	{
		switch (value)
		{
		case TEXTURE_ADDRESS_WRAP:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			break;
		case TEXTURE_ADDRESS_MIRROR:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			break;
		case TEXTURE_ADDRESS_CLAMP:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			break;
		case TEXTURE_ADDRESS_BORDER:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			break;
		default:
			break;
		}
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}
	constexpr VkStencilOp _ConvertStencilOp(STENCIL_OP value)
	{
		switch (value)
		{
		case wiGraphics::STENCIL_OP_KEEP:
			return VK_STENCIL_OP_KEEP;
			break;
		case wiGraphics::STENCIL_OP_ZERO:
			return VK_STENCIL_OP_ZERO;
			break;
		case wiGraphics::STENCIL_OP_REPLACE:
			return VK_STENCIL_OP_REPLACE;
			break;
		case wiGraphics::STENCIL_OP_INCR_SAT:
			return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
			break;
		case wiGraphics::STENCIL_OP_DECR_SAT:
			return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
			break;
		case wiGraphics::STENCIL_OP_INVERT:
			return VK_STENCIL_OP_INVERT;
			break;
		case wiGraphics::STENCIL_OP_INCR:
			return VK_STENCIL_OP_INCREMENT_AND_WRAP;
			break;
		case wiGraphics::STENCIL_OP_DECR:
			return VK_STENCIL_OP_DECREMENT_AND_WRAP;
			break;
		default:
			break;
		}
		return VK_STENCIL_OP_KEEP;
	}
	constexpr VkImageLayout _ConvertImageLayout(IMAGE_LAYOUT value)
	{
		switch (value)
		{
		case wiGraphics::IMAGE_LAYOUT_UNDEFINED:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		case wiGraphics::IMAGE_LAYOUT_RENDERTARGET:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case wiGraphics::IMAGE_LAYOUT_DEPTHSTENCIL:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case wiGraphics::IMAGE_LAYOUT_DEPTHSTENCIL_READONLY:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		case wiGraphics::IMAGE_LAYOUT_SHADER_RESOURCE:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case wiGraphics::IMAGE_LAYOUT_UNORDERED_ACCESS:
			return VK_IMAGE_LAYOUT_GENERAL;
		case wiGraphics::IMAGE_LAYOUT_COPY_SRC:
			return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case wiGraphics::IMAGE_LAYOUT_COPY_DST:
			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case wiGraphics::IMAGE_LAYOUT_SHADING_RATE_SOURCE:
			return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
		}
		return VK_IMAGE_LAYOUT_UNDEFINED;
	}
	constexpr VkShaderStageFlags _ConvertStageFlags(SHADERSTAGE value)
	{
		switch (value)
		{
		case wiGraphics::MS:
			return VK_SHADER_STAGE_MESH_BIT_NV;
		case wiGraphics::AS:
			return VK_SHADER_STAGE_TASK_BIT_NV;
		case wiGraphics::VS:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case wiGraphics::HS:
			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case wiGraphics::DS:
			return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		case wiGraphics::GS:
			return VK_SHADER_STAGE_GEOMETRY_BIT;
		case wiGraphics::PS:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case wiGraphics::CS:
			return VK_SHADER_STAGE_COMPUTE_BIT;
		default:
			return VK_SHADER_STAGE_ALL;
		}
	}


	inline VkAccessFlags _ParseImageLayout(IMAGE_LAYOUT value)
	{
		VkAccessFlags flags = 0;

		switch (value)
		{
		case wiGraphics::IMAGE_LAYOUT_UNDEFINED:
			break;
		case wiGraphics::IMAGE_LAYOUT_RENDERTARGET:
			flags |= VK_ACCESS_SHADER_WRITE_BIT;
			break;
		case wiGraphics::IMAGE_LAYOUT_DEPTHSTENCIL:
			flags |= VK_ACCESS_SHADER_WRITE_BIT;
			break;
		case wiGraphics::IMAGE_LAYOUT_DEPTHSTENCIL_READONLY:
			flags |= VK_ACCESS_SHADER_READ_BIT;
			break;
		case wiGraphics::IMAGE_LAYOUT_SHADER_RESOURCE:
			flags |= VK_ACCESS_SHADER_READ_BIT;
			break;
		case wiGraphics::IMAGE_LAYOUT_UNORDERED_ACCESS:
			flags |= VK_ACCESS_SHADER_READ_BIT;
			flags |= VK_ACCESS_SHADER_WRITE_BIT;
			break;
		case wiGraphics::IMAGE_LAYOUT_COPY_SRC:
			flags |= VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case wiGraphics::IMAGE_LAYOUT_COPY_DST:
			flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		}

		return flags;
	}
	inline VkAccessFlags _ParseBufferState(BUFFER_STATE value)
	{
		VkAccessFlags flags = 0;

		switch (value)
		{
		case wiGraphics::BUFFER_STATE_UNDEFINED:
			break;
		case wiGraphics::BUFFER_STATE_VERTEX_BUFFER:
			flags |= VK_ACCESS_SHADER_READ_BIT;
			flags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			break;
		case wiGraphics::BUFFER_STATE_INDEX_BUFFER:
			flags |= VK_ACCESS_SHADER_READ_BIT;
			flags |= VK_ACCESS_INDEX_READ_BIT;
			break;
		case wiGraphics::BUFFER_STATE_CONSTANT_BUFFER:
			flags |= VK_ACCESS_SHADER_READ_BIT;
			flags |= VK_ACCESS_UNIFORM_READ_BIT;
			break;
		case wiGraphics::BUFFER_STATE_INDIRECT_ARGUMENT:
			flags |= VK_ACCESS_SHADER_READ_BIT;
			flags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
			break;
		case wiGraphics::BUFFER_STATE_SHADER_RESOURCE:
			flags |= VK_ACCESS_SHADER_READ_BIT;
			flags |= VK_ACCESS_UNIFORM_READ_BIT;
			break;
		case wiGraphics::BUFFER_STATE_UNORDERED_ACCESS:
			flags |= VK_ACCESS_SHADER_READ_BIT;
			flags |= VK_ACCESS_SHADER_WRITE_BIT;
			break;
		case wiGraphics::BUFFER_STATE_COPY_SRC:
			flags |= VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case wiGraphics::BUFFER_STATE_COPY_DST:
			flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		default:
			break;
		}

		return flags;
	}
	
	bool checkExtensionSupport(const char* checkExtension, const std::vector<VkExtensionProperties>& available_extensions)
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

	// Validation layer helpers:
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
	bool checkValidationLayerSupport()
	{
		uint32_t layerCount;
		VkResult res = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		assert(res == VK_SUCCESS);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		res = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
		assert(res == VK_SUCCESS);

		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
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
		// Log debug messge
		std::stringstream ss("");

		if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			ss << "[Vulkan Warning]: " << callback_data->pMessage << std::endl;
		}
		else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			ss << "[Vulkan Error]: " << callback_data->pMessage << std::endl;
		}

		std::clog << ss.str();
#ifdef _WIN32
		OutputDebugStringA(ss.str().c_str());
#endif

		return VK_FALSE;
	}

	// Memory tools:

	inline size_t Align(size_t uLocation, size_t uAlign)
	{
		if ((0 == uAlign) || (uAlign & (uAlign - 1)))
		{
			assert(0);
		}

		return ((uLocation + (uAlign - 1)) & ~(uAlign - 1));
	}


	struct Buffer_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VmaAllocation allocation = nullptr;
		VkBuffer resource = VK_NULL_HANDLE;
		int cbv_index = -1;
		VkBufferView srv = VK_NULL_HANDLE;
		int srv_index = -1;
		VkBufferView uav = VK_NULL_HANDLE;
		int uav_index = -1;
		std::vector<VkBufferView> subresources_srv;
		std::vector<int> subresources_srv_index;
		std::vector<VkBufferView> subresources_uav;
		std::vector<int> subresources_uav_index;
		VkDeviceAddress address = 0;
		bool is_typedbuffer = false;

		GraphicsDevice::GPUAllocation dynamic[COMMANDLIST_COUNT];

		~Buffer_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (resource) allocationhandler->destroyer_buffers.push_back(std::make_pair(std::make_pair(resource, allocation), framecount));
			if (srv) allocationhandler->destroyer_bufferviews.push_back(std::make_pair(srv, framecount));
			if (uav) allocationhandler->destroyer_bufferviews.push_back(std::make_pair(uav, framecount));
			for (auto x : subresources_srv)
			{
				allocationhandler->destroyer_bufferviews.push_back(std::make_pair(x, framecount));
			}
			for (auto x : subresources_uav)
			{
				allocationhandler->destroyer_bufferviews.push_back(std::make_pair(x, framecount));
			}
			if (cbv_index >= 0) allocationhandler->destroyer_bindlessUniformBuffers.push_back(std::make_pair(cbv_index, framecount));
			if (is_typedbuffer)
			{
				if (srv_index >= 0) allocationhandler->destroyer_bindlessUniformTexelBuffers.push_back(std::make_pair(srv_index, framecount));
				if (uav_index >= 0) allocationhandler->destroyer_bindlessStorageTexelBuffers.push_back(std::make_pair(uav_index, framecount));
				for (auto x : subresources_srv_index)
				{
					if (x >= 0) allocationhandler->destroyer_bindlessUniformTexelBuffers.push_back(std::make_pair(x, framecount));
				}
				for (auto x : subresources_uav_index)
				{
					if (x >= 0) allocationhandler->destroyer_bindlessStorageTexelBuffers.push_back(std::make_pair(x, framecount));
				}
			}
			else
			{
				if (srv_index >= 0) allocationhandler->destroyer_bindlessStorageBuffers.push_back(std::make_pair(srv_index, framecount));
				if (uav_index >= 0) allocationhandler->destroyer_bindlessStorageBuffers.push_back(std::make_pair(uav_index, framecount));
				for (auto x : subresources_srv_index)
				{
					if (x >= 0) allocationhandler->destroyer_bindlessStorageBuffers.push_back(std::make_pair(x, framecount));
				}
				for (auto x : subresources_uav_index)
				{
					if (x >= 0) allocationhandler->destroyer_bindlessStorageBuffers.push_back(std::make_pair(x, framecount));
				}
			}
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Texture_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VmaAllocation allocation = nullptr;
		VkImage resource = VK_NULL_HANDLE;
		VkBuffer staging_resource = VK_NULL_HANDLE;
		VkImageView srv = VK_NULL_HANDLE;
		int srv_index = -1;
		VkImageView uav = VK_NULL_HANDLE;
		int uav_index = -1;
		VkImageView rtv = VK_NULL_HANDLE;
		VkImageView dsv = VK_NULL_HANDLE;
		uint32_t framebuffer_layercount = 0;
		std::vector<VkImageView> subresources_srv;
		std::vector<int> subresources_srv_index;
		std::vector<VkImageView> subresources_uav;
		std::vector<int> subresources_uav_index;
		std::vector<VkImageView> subresources_rtv;
		std::vector<VkImageView> subresources_dsv;
		std::vector<uint32_t> subresources_framebuffer_layercount;

		VkSubresourceLayout subresourcelayout = {};

		~Texture_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (resource) allocationhandler->destroyer_images.push_back(std::make_pair(std::make_pair(resource, allocation), framecount));
			if (staging_resource) allocationhandler->destroyer_buffers.push_back(std::make_pair(std::make_pair(staging_resource, allocation), framecount));
			if (srv) allocationhandler->destroyer_imageviews.push_back(std::make_pair(srv, framecount));
			if (uav) allocationhandler->destroyer_imageviews.push_back(std::make_pair(uav, framecount));
			if (srv) allocationhandler->destroyer_imageviews.push_back(std::make_pair(rtv, framecount));
			if (uav) allocationhandler->destroyer_imageviews.push_back(std::make_pair(dsv, framecount));
			for (auto x : subresources_srv)
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(x, framecount));
			}
			for (auto x : subresources_uav)
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(x, framecount));
			}
			for (auto x : subresources_rtv)
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(x, framecount));
			}
			for (auto x : subresources_dsv)
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(x, framecount));
			}
			if (srv_index >= 0) allocationhandler->destroyer_bindlessSampledImages.push_back(std::make_pair(srv_index, framecount));
			if (uav_index >= 0) allocationhandler->destroyer_bindlessStorageImages.push_back(std::make_pair(uav_index, framecount));
			for (auto x : subresources_srv_index)
			{
				if (x >= 0) allocationhandler->destroyer_bindlessSampledImages.push_back(std::make_pair(x, framecount));
			}
			for (auto x : subresources_uav_index)
			{
				if (x >= 0) allocationhandler->destroyer_bindlessStorageImages.push_back(std::make_pair(x, framecount));
			}
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
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		std::vector<VkImageViewType> imageViewTypes;

		std::vector<VkDescriptorSetLayoutBinding> bindlessBindings;
		std::vector<VkDescriptorSet> bindlessSets;
		uint32_t bindlessFirstSet = 0;

		VkPushConstantRange pushconstants = {};

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
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE; // no lifetime management here
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE; // no lifetime management here
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		std::vector<VkImageViewType> imageViewTypes;

		std::vector<VkDescriptorSetLayoutBinding> bindlessBindings;
		std::vector<VkDescriptorSet> bindlessSets;
		uint32_t bindlessFirstSet = 0;

		VkPushConstantRange pushconstants = {};

		size_t binding_hash = 0;

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		VkPipelineShaderStageCreateInfo shaderStages[SHADERSTAGE_COUNT] = {};
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		VkPipelineRasterizationDepthClipStateCreateInfoEXT depthclip = {};
		VkViewport viewport = {};
		VkRect2D scissor = {};
		VkPipelineViewportStateCreateInfo viewportState = {};
		VkPipelineDepthStencilStateCreateInfo depthstencil = {};
		VkSampleMask samplemask = {};
		VkPipelineTessellationStateCreateInfo tessellationInfo = {};
	};
	struct RenderPass_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VkRenderPass renderpass = VK_NULL_HANDLE;
		VkFramebuffer framebuffer = VK_NULL_HANDLE;
		VkRenderPassBeginInfo beginInfo = {};
		VkClearValue clearColors[9] = {};

		~RenderPass_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (renderpass) allocationhandler->destroyer_renderpasses.push_back(std::make_pair(renderpass, framecount));
			if (framebuffer) allocationhandler->destroyer_framebuffers.push_back(std::make_pair(framebuffer, framecount));
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
		std::vector<VkAccelerationStructureGeometryKHR> geometries;
		std::vector<uint32_t> primitiveCounts;
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
	RenderPass_Vulkan* to_internal(const RenderPass* param)
	{
		return static_cast<RenderPass_Vulkan*>(param->internal_state.get());
	}
	BVH_Vulkan* to_internal(const RaytracingAccelerationStructure* param)
	{
		return static_cast<BVH_Vulkan*>(param->internal_state.get());
	}
	RTPipelineState_Vulkan* to_internal(const RaytracingPipelineState* param)
	{
		return static_cast<RTPipelineState_Vulkan*>(param->internal_state.get());
	}
}
using namespace Vulkan_Internal;

	// Allocators:

	void GraphicsDevice_Vulkan::FrameResources::ResourceFrameAllocator::init(GraphicsDevice_Vulkan* device, size_t size)
	{
		this->device = device;
		auto internal_state = std::make_shared<Buffer_Vulkan>();
		internal_state->allocationhandler = device->allocationhandler;
		buffer.internal_state = internal_state;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		if (device->features_1_2.bufferDeviceAddress == VK_TRUE)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		}
		bufferInfo.flags = 0;

		VkResult res;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		res = vmaCreateBuffer(device->allocationhandler->allocator, &bufferInfo, &allocInfo, &internal_state->resource, &internal_state->allocation, nullptr);
		assert(res == VK_SUCCESS);

		if (bufferInfo.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			VkBufferDeviceAddressInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			info.buffer = internal_state->resource;
			internal_state->address = vkGetBufferDeviceAddress(device->device, &info);
		}

		void* pData = internal_state->allocation->GetMappedData();
		dataCur = dataBegin = reinterpret_cast<uint8_t*>(pData);
		dataEnd = dataBegin + size;

		// Because the "buffer" is created by hand in this, fill the desc to indicate how it can be used:
		this->buffer.type = GPUResource::GPU_RESOURCE_TYPE::BUFFER;
		this->buffer.desc.ByteWidth = (uint32_t)((size_t)dataEnd - (size_t)dataBegin);
		this->buffer.desc.Usage = USAGE_DYNAMIC;
		this->buffer.desc.BindFlags = BIND_VERTEX_BUFFER | BIND_INDEX_BUFFER | BIND_SHADER_RESOURCE;
		this->buffer.desc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

		int index = device->allocationhandler->bindlessStorageBuffers.allocate();
		if (index >= 0)
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = internal_state->resource;
			bufferInfo.offset = 0;
			bufferInfo.range = (VkDeviceSize)size;
			VkWriteDescriptorSet write = {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write.dstBinding = 0;
			write.dstArrayElement = index;
			write.descriptorCount = 1;
			write.dstSet = device->allocationhandler->bindlessStorageBuffers.descriptorSet;
			write.pBufferInfo = &bufferInfo;
			vkUpdateDescriptorSets(device->device, 1, &write, 0, nullptr);
		}
		internal_state->srv_index = index;
	}
	uint8_t* GraphicsDevice_Vulkan::FrameResources::ResourceFrameAllocator::allocate(size_t dataSize, size_t alignment)
	{
		dataCur = reinterpret_cast<uint8_t*>(Align(reinterpret_cast<size_t>(dataCur), alignment));

		if (dataCur + dataSize > dataEnd)
		{
			init(device, ((size_t)dataEnd + dataSize - (size_t)dataBegin) * 2);
		}

		uint8_t* retVal = dataCur;

		dataCur += dataSize;

		return retVal;
	}
	void GraphicsDevice_Vulkan::FrameResources::ResourceFrameAllocator::clear()
	{
		dataCur = dataBegin;
	}
	uint64_t GraphicsDevice_Vulkan::FrameResources::ResourceFrameAllocator::calculateOffset(uint8_t* address)
	{
		assert(address >= dataBegin && address < dataEnd);
		return static_cast<uint64_t>(address - dataBegin);
	}

	void GraphicsDevice_Vulkan::FrameResources::DescriptorBinder::init(GraphicsDevice_Vulkan* device)
	{
		this->device = device;

		// Important that these don't reallocate themselves during writing descriptors!
		descriptorWrites.reserve(128);
		bufferInfos.reserve(128);
		imageInfos.reserve(128);
		texelBufferViews.reserve(128);
		accelerationStructureViews.reserve(128);

		VkResult res;

		// Create descriptor pool:
		VkDescriptorPoolSize poolSizes[9] = {};
		uint32_t count = 0;

		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = GPU_RESOURCE_HEAP_CBV_COUNT * poolSize;
		count++;

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		poolSizes[1].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT * poolSize;
		count++;

		poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		poolSizes[2].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT * poolSize;
		count++;

		poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[3].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT * poolSize;
		count++;

		poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[4].descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT * poolSize;
		count++;

		poolSizes[5].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		poolSizes[5].descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT * poolSize;
		count++;

		poolSizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[6].descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT * poolSize;
		count++;

		poolSizes[7].type = VK_DESCRIPTOR_TYPE_SAMPLER;
		poolSizes[7].descriptorCount = GPU_SAMPLER_HEAP_COUNT * poolSize;
		count++;

		if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING))
		{
			poolSizes[8].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			poolSizes[8].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT * poolSize;
			count++;
		}

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = count;
		poolInfo.pPoolSizes = poolSizes;
		poolInfo.maxSets = poolSize;
		//poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		res = vkCreateDescriptorPool(device->device, &poolInfo, nullptr, &descriptorPool);
		assert(res == VK_SUCCESS);

		reset();

	}
	void GraphicsDevice_Vulkan::FrameResources::DescriptorBinder::destroy()
	{
		if (descriptorPool != VK_NULL_HANDLE)
		{
			device->allocationhandler->destroylocker.lock();
			device->allocationhandler->destroyer_descriptorPools.push_back(std::make_pair(descriptorPool, device->FRAMECOUNT));
			descriptorPool = VK_NULL_HANDLE;
			device->allocationhandler->destroylocker.unlock();
		}
	}
	void GraphicsDevice_Vulkan::FrameResources::DescriptorBinder::reset()
	{
		dirty = true;

		if (descriptorPool != VK_NULL_HANDLE)
		{
			VkResult res = vkResetDescriptorPool(device->device, descriptorPool, 0);
			assert(res == VK_SUCCESS);
		}

		memset(CBV, 0, sizeof(CBV));
		memset(SRV, 0, sizeof(SRV));
		memset(SRV_index, -1, sizeof(SRV_index));
		memset(UAV, 0, sizeof(UAV));
		memset(UAV_index, -1, sizeof(UAV_index));
		memset(SAM, 0, sizeof(SAM));
	}
	void GraphicsDevice_Vulkan::FrameResources::DescriptorBinder::validate(bool graphics, CommandList cmd)
	{
		if (!dirty)
			return;
		dirty = false;

		auto pso_internal = graphics ? to_internal(device->active_pso[cmd]) : nullptr;
		auto cs_internal = graphics ? nullptr : to_internal(device->active_cs[cmd]);

		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		if (graphics)
		{
			pipelineLayout = pso_internal->pipelineLayout;
			descriptorSetLayout = pso_internal->descriptorSetLayout;
		}
		else
		{
			pipelineLayout = cs_internal->pipelineLayout_cs;
			descriptorSetLayout = cs_internal->descriptorSetLayout;
		}

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		VkResult res = vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet);
		while (res == VK_ERROR_OUT_OF_POOL_MEMORY)
		{
			poolSize *= 2;
			destroy();
			init(device);
			allocInfo.descriptorPool = descriptorPool;
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

			descriptorWrites.emplace_back();
			auto& write = descriptorWrites.back();
			write = {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = descriptorSet;
			write.dstArrayElement = 0;
			write.descriptorType = x.descriptorType;
			write.dstBinding = x.binding;
			write.descriptorCount = 1;

			VkImageViewType viewtype = imageViewTypes[i++];

			switch (x.descriptorType)
			{
			case VK_DESCRIPTOR_TYPE_SAMPLER:
			{
				imageInfos.emplace_back();
				write.pImageInfo = &imageInfos.back();
				imageInfos.back() = {};

				const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_S;
				const Sampler* sampler = SAM[original_binding];
				if (sampler == nullptr || !sampler->IsValid())
				{
					imageInfos.back().sampler = device->nullSampler;
				}
				else
				{
					imageInfos.back().sampler = to_internal(sampler)->resource;
				}
			}
			break;

			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			{
				imageInfos.emplace_back();
				write.pImageInfo = &imageInfos.back();
				imageInfos.back() = {};

				const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_T;
				const GPUResource* resource = SRV[original_binding];
				if (resource == nullptr || !resource->IsValid() || !resource->IsTexture())
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
					int subresource = SRV_index[original_binding];
					const Texture* texture = (const Texture*)resource;
					if (subresource >= 0)
					{
						imageInfos.back().imageView = to_internal(texture)->subresources_srv[subresource];
					}
					else
					{
						imageInfos.back().imageView = to_internal(texture)->srv;
					}

					imageInfos.back().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				}
			}
			break;

			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			{
				imageInfos.emplace_back();
				write.pImageInfo = &imageInfos.back();
				imageInfos.back() = {};
				imageInfos.back().imageLayout = VK_IMAGE_LAYOUT_GENERAL;

				const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_U;
				const GPUResource* resource = UAV[original_binding];
				if (resource == nullptr || !resource->IsValid() || !resource->IsTexture())
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
					int subresource = UAV_index[original_binding];
					const Texture* texture = (const Texture*)resource;
					if (subresource >= 0)
					{
						imageInfos.back().imageView = to_internal(texture)->subresources_uav[subresource];
					}
					else
					{
						imageInfos.back().imageView = to_internal(texture)->uav;
					}
				}
			}
			break;

			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			{
				bufferInfos.emplace_back();
				write.pBufferInfo = &bufferInfos.back();
				bufferInfos.back() = {};

				const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_B;
				const GPUBuffer* buffer = CBV[original_binding];
				if (buffer == nullptr || !buffer->IsValid())
				{
					bufferInfos.back().buffer = device->nullBuffer;
					bufferInfos.back().range = VK_WHOLE_SIZE;
				}
				else
				{
					auto internal_state = to_internal(buffer);
					if (buffer->desc.Usage == USAGE_DYNAMIC)
					{
						const GPUAllocation& allocation = internal_state->dynamic[cmd];
						bufferInfos.back().buffer = to_internal(allocation.buffer)->resource;
						bufferInfos.back().offset = allocation.offset;
						bufferInfos.back().range = buffer->desc.ByteWidth;
					}
					else
					{
						bufferInfos.back().buffer = internal_state->resource;
						bufferInfos.back().offset = 0;
						bufferInfos.back().range = buffer->desc.ByteWidth;
					}
				}
			}
			break;

			case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
			{
				texelBufferViews.emplace_back();
				write.pTexelBufferView = &texelBufferViews.back();
				texelBufferViews.back() = {};

				const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_T;
				const GPUResource* resource = SRV[original_binding];
				if (resource == nullptr || !resource->IsValid() || !resource->IsBuffer())
				{
					texelBufferViews.back() = device->nullBufferView;
				}
				else
				{
					int subresource = SRV_index[original_binding];
					const GPUBuffer* buffer = (const GPUBuffer*)resource;
					if (subresource >= 0)
					{
						texelBufferViews.back() = to_internal(buffer)->subresources_srv[subresource];
					}
					else
					{
						texelBufferViews.back() = to_internal(buffer)->srv;
					}
				}
			}
			break;

			case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			{
				texelBufferViews.emplace_back();
				write.pTexelBufferView = &texelBufferViews.back();
				texelBufferViews.back() = {};

				const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_U;
				const GPUResource* resource = UAV[original_binding];
				if (resource == nullptr || !resource->IsValid() || !resource->IsBuffer())
				{
					texelBufferViews.back() = device->nullBufferView;
				}
				else
				{
					int subresource = UAV_index[original_binding];
					const GPUBuffer* buffer = (const GPUBuffer*)resource;
					if (subresource >= 0)
					{
						texelBufferViews.back() = to_internal(buffer)->subresources_uav[subresource];
					}
					else
					{
						texelBufferViews.back() = to_internal(buffer)->uav;
					}
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
					const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_T;
					const GPUResource* resource = SRV[original_binding];
					if (resource == nullptr || !resource->IsValid() || !resource->IsBuffer())
					{
						bufferInfos.back().buffer = device->nullBuffer;
						bufferInfos.back().range = VK_WHOLE_SIZE;
					}
					else
					{
						int subresource = SRV_index[original_binding];
						const GPUBuffer* buffer = (const GPUBuffer*)resource;
						bufferInfos.back().buffer = to_internal(buffer)->resource;
						bufferInfos.back().range = buffer->desc.ByteWidth;
					}
				}
				else
				{
					// UAV
					const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_U;
					const GPUResource* resource = UAV[original_binding];
					if (resource == nullptr || !resource->IsValid() || !resource->IsBuffer())
					{
						bufferInfos.back().buffer = device->nullBuffer;
						bufferInfos.back().range = VK_WHOLE_SIZE;
					}
					else
					{
						int subresource = UAV_index[original_binding];
						const GPUBuffer* buffer = (const GPUBuffer*)resource;
						bufferInfos.back().buffer = to_internal(buffer)->resource;
						bufferInfos.back().range = buffer->desc.ByteWidth;
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

				const uint32_t original_binding = x.binding - VULKAN_BINDING_SHIFT_T;
				const GPUResource* resource = SRV[original_binding];
				if (resource == nullptr || !resource->IsValid() || !resource->IsAccelerationStructure())
				{
					assert(0); // invalid acceleration structure!
				}
				else
				{
					const RaytracingAccelerationStructure* as = (const RaytracingAccelerationStructure*)resource;
					accelerationStructureViews.back().pAccelerationStructures = &to_internal(as)->resource;
				}
			}
			break;

			}
		}

		vkUpdateDescriptorSets(
			device->device,
			(uint32_t)descriptorWrites.size(),
			descriptorWrites.data(),
			0,
			nullptr
		);

		VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		if (!graphics)
		{
			bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

			if (device->active_cs[cmd]->stage == LIB)
			{
				bindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
			}
		}

		vkCmdBindDescriptorSets(
			device->GetDirectCommandList(cmd),
			bindPoint,
			pipelineLayout,
			0,
			1,
			&descriptorSet,
			0,
			nullptr
		);
	}

	void GraphicsDevice_Vulkan::pso_validate(CommandList cmd)
	{
		if (!dirty_pso[cmd])
			return;

		const PipelineState* pso = active_pso[cmd];
		size_t pipeline_hash = prev_pipeline_hash[cmd];
		wiHelper::hash_combine(pipeline_hash, vb_hash[cmd]);
		auto internal_state = to_internal(pso);

		VkPipeline pipeline = VK_NULL_HANDLE;
		auto it = pipelines_global.find(pipeline_hash);
		if (it == pipelines_global.end())
		{
			for (auto& x : pipelines_worker[cmd])
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
				pipelineInfo.renderPass = active_renderpass[cmd] == nullptr ? defaultRenderPass : to_internal(active_renderpass[cmd])->renderpass;
				pipelineInfo.subpass = 0;

				// MSAA:
				VkPipelineMultisampleStateCreateInfo multisampling = {};
				multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisampling.sampleShadingEnable = VK_FALSE;
				multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
				if (active_renderpass[cmd] != nullptr && active_renderpass[cmd]->desc.attachments.size() > 0)
				{
					multisampling.rasterizationSamples = (VkSampleCountFlagBits)active_renderpass[cmd]->desc.attachments[0].texture->desc.SampleCount;
				}
				if (pso->desc.rs != nullptr)
				{
					const RasterizerState& desc = *pso->desc.rs;
					if (desc.ForcedSampleCount > 1)
					{
						multisampling.rasterizationSamples = (VkSampleCountFlagBits)desc.ForcedSampleCount;
					}
				}
				multisampling.minSampleShading = 1.0f;
				VkSampleMask& samplemask = internal_state->samplemask;
				samplemask = pso->desc.sampleMask;
				multisampling.pSampleMask = &samplemask;
				multisampling.alphaToCoverageEnable = VK_FALSE;
				multisampling.alphaToOneEnable = VK_FALSE;

				pipelineInfo.pMultisampleState = &multisampling;


				// Blending:
				uint32_t numBlendAttachments = 0;
				VkPipelineColorBlendAttachmentState colorBlendAttachments[8] = {};
				const size_t blend_loopCount = active_renderpass[cmd] == nullptr ? 1 : active_renderpass[cmd]->desc.attachments.size();
				for (size_t i = 0; i < blend_loopCount; ++i)
				{
					if (active_renderpass[cmd] != nullptr && active_renderpass[cmd]->desc.attachments[i].type != RenderPassAttachment::RENDERTARGET)
					{
						continue;
					}

					const auto& desc = pso->desc.bs->RenderTarget[numBlendAttachments];
					VkPipelineColorBlendAttachmentState& attachment = colorBlendAttachments[numBlendAttachments];
					numBlendAttachments++;

					attachment.blendEnable = desc.BlendEnable ? VK_TRUE : VK_FALSE;

					attachment.colorWriteMask = 0;
					if (desc.RenderTargetWriteMask & COLOR_WRITE_ENABLE_RED)
					{
						attachment.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
					}
					if (desc.RenderTargetWriteMask & COLOR_WRITE_ENABLE_GREEN)
					{
						attachment.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
					}
					if (desc.RenderTargetWriteMask & COLOR_WRITE_ENABLE_BLUE)
					{
						attachment.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
					}
					if (desc.RenderTargetWriteMask & COLOR_WRITE_ENABLE_ALPHA)
					{
						attachment.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
					}

					attachment.srcColorBlendFactor = _ConvertBlend(desc.SrcBlend);
					attachment.dstColorBlendFactor = _ConvertBlend(desc.DestBlend);
					attachment.colorBlendOp = _ConvertBlendOp(desc.BlendOp);
					attachment.srcAlphaBlendFactor = _ConvertBlend(desc.SrcBlendAlpha);
					attachment.dstAlphaBlendFactor = _ConvertBlend(desc.DestBlendAlpha);
					attachment.alphaBlendOp = _ConvertBlendOp(desc.BlendOpAlpha);
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
				std::vector<VkVertexInputBindingDescription> bindings;
				std::vector<VkVertexInputAttributeDescription> attributes;
				if (pso->desc.il != nullptr)
				{
					uint32_t lastBinding = 0xFFFFFFFF;
					uint32_t i = 0;
					for (auto& x : pso->desc.il->elements)
					{
						VkVertexInputBindingDescription bind = {};
						bind.binding = x.InputSlot;
						bind.inputRate = x.InputSlotClass == INPUT_PER_VERTEX_DATA ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
						bind.stride = vb_strides[cmd][i];

						if (lastBinding != bind.binding)
						{
							bindings.push_back(bind);
							lastBinding = bind.binding;
						}
						else
						{
							bindings.back().stride += bind.stride;
						}

						i++;
					}

					uint32_t offset = 0;
					i = 0;
					lastBinding = 0xFFFFFFFF;
					for (auto& x : pso->desc.il->elements)
					{
						VkVertexInputAttributeDescription attr = {};
						attr.binding = x.InputSlot;
						if (attr.binding != lastBinding)
						{
							lastBinding = attr.binding;
							offset = 0;
						}
						attr.format = _ConvertFormat(x.Format);
						attr.location = i;
						attr.offset = x.AlignedByteOffset;
						if (attr.offset == InputLayout::APPEND_ALIGNED_ELEMENT)
						{
							// need to manually resolve this from the format spec.
							attr.offset = offset;
							offset += GetFormatStride(x.Format);
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

				VkResult res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
				assert(res == VK_SUCCESS);

				pipelines_worker[cmd].push_back(std::make_pair(pipeline_hash, pipeline));
			}
		}
		else
		{
			pipeline = it->second;
		}
		assert(pipeline != VK_NULL_HANDLE);

		vkCmdBindPipeline(GetDirectCommandList(cmd), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	}

	void GraphicsDevice_Vulkan::barrier_flush(CommandList cmd)
	{
		auto& memoryBarriers = frame_memoryBarriers[cmd];
		auto& imageBarriers = frame_imageBarriers[cmd];
		auto& bufferBarriers = frame_bufferBarriers[cmd];

		if (!memoryBarriers.empty() ||
			!bufferBarriers.empty() ||
			!imageBarriers.empty()
			)
		{
			// Remove NOP barriers:
			for (size_t i = 0; i < memoryBarriers.size(); ++i)
			{
				auto& barrier = memoryBarriers[i];
				if (barrier.srcAccessMask == barrier.dstAccessMask)
				{
					barrier = memoryBarriers.back();
					memoryBarriers.pop_back();
					i--;
				}
			}
			for (size_t i = 0; i < bufferBarriers.size(); ++i)
			{
				auto& barrier = bufferBarriers[i];
				if (barrier.srcAccessMask == barrier.dstAccessMask)
				{
					barrier = bufferBarriers.back();
					bufferBarriers.pop_back();
					i--;
				}
			}
			for (size_t i = 0; i < imageBarriers.size(); ++i)
			{
				auto& barrier = imageBarriers[i];
				if (barrier.oldLayout == barrier.newLayout)
				{
					barrier = imageBarriers.back();
					imageBarriers.pop_back();
					i--;
				}
			}

			if (!memoryBarriers.empty() ||
				!bufferBarriers.empty() ||
				!imageBarriers.empty()
				)
			{
				VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
				VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

				if (CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING))
				{
					srcStage |= VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
					dstStage |= VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
				}

				vkCmdPipelineBarrier(
					GetDirectCommandList(cmd),
					srcStage,
					dstStage,
					0,
					(uint32_t)memoryBarriers.size(), memoryBarriers.data(),
					(uint32_t)bufferBarriers.size(), bufferBarriers.data(),
					(uint32_t)imageBarriers.size(), imageBarriers.data()
				);

				memoryBarriers.clear();
				imageBarriers.clear();
				bufferBarriers.clear();
			}
		}
	}
	void GraphicsDevice_Vulkan::predraw(CommandList cmd)
	{
		pso_validate(cmd);

		GetFrameResources().descriptors[cmd].validate(true, cmd);

		if (pushconstants[cmd].size > 0)
		{
			auto pso_internal = to_internal(active_pso[cmd]);
			vkCmdPushConstants(
				GetDirectCommandList(cmd),
				pso_internal->pipelineLayout,
				pso_internal->pushconstants.stageFlags,
				pso_internal->pushconstants.offset,
				pso_internal->pushconstants.size,
				pushconstants[cmd].data
			);
			pushconstants[cmd].size = 0;
		}
	}
	void GraphicsDevice_Vulkan::predispatch(CommandList cmd)
	{
		barrier_flush(cmd);

		GetFrameResources().descriptors[cmd].validate(false, cmd);

		if (pushconstants[cmd].size > 0)
		{
			auto cs_internal = to_internal(active_cs[cmd]);
			vkCmdPushConstants(
				GetDirectCommandList(cmd),
				cs_internal->pipelineLayout_cs,
				cs_internal->pushconstants.stageFlags,
				cs_internal->pushconstants.offset,
				cs_internal->pushconstants.size,
				pushconstants[cmd].data
			);
			pushconstants[cmd].size = 0;
		}
	}

	// Engine functions
	GraphicsDevice_Vulkan::GraphicsDevice_Vulkan(wiPlatform::window_type window, bool fullscreen, bool debuglayer)
	{
		TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = sizeof(VkAccelerationStructureInstanceKHR);

		DEBUGDEVICE = debuglayer;

		FULLSCREEN = fullscreen;

#ifdef _WIN32
		dpi = GetDpiForWindow(window);
		RECT rect;
		GetClientRect(window, &rect);
		RESOLUTIONWIDTH = rect.right - rect.left;
		RESOLUTIONHEIGHT = rect.bottom - rect.top;
#elif SDL2
		int width, height;
		SDL_GetWindowSize(window, &width, &height);
		RESOLUTIONWIDTH = width;
		RESOLUTIONHEIGHT = height;
#endif // _WIN32

		VkResult res;

		res = volkInitialize();
		assert(res == VK_SUCCESS);

		// Fill out application info:
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Wicked Engine Application";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Wicked Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(wiVersion::GetMajor(), wiVersion::GetMinor(), wiVersion::GetRevision());
		appInfo.apiVersion = VK_API_VERSION_1_2;

		// Enumerate available extensions:
		uint32_t extensionCount = 0;
		res = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		assert(res == VK_SUCCESS);
		std::vector<VkExtensionProperties> availableInstanceExtensions(extensionCount);
		res = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableInstanceExtensions.data());
		assert(res == VK_SUCCESS);

		std::vector<const char*> extensionNames;

		if (checkExtensionSupport(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, availableInstanceExtensions))
		{
			// This is needed for not only debug layer, but also debug markers, object naming, etc:
			extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		
		extensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef _WIN32
		extensionNames.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif SDL2
		{
			uint32_t extensionCount;
			SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
			std::vector<const char *> extensionNames_sdl(extensionCount);
			SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensionNames_sdl.data());
			extensionNames.reserve(extensionNames.size() + extensionNames_sdl.size());
			extensionNames.insert(extensionNames.begin(),
					extensionNames_sdl.cbegin(), extensionNames_sdl.cend());
		}
#endif // _WIN32
		
		if (debuglayer && !checkValidationLayerSupport())
		{
			wiHelper::messageBox("Vulkan validation layer requested but not available!");
			debuglayer = false;
		}

		// Create instance:
		{
			VkInstanceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pApplicationInfo = &appInfo;
			createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
			createInfo.ppEnabledExtensionNames = extensionNames.data();
			createInfo.enabledLayerCount = 0;
			if (debuglayer)
			{
				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();
			}
			res = vkCreateInstance(&createInfo, nullptr, &instance);
			assert(res == VK_SUCCESS);

			volkLoadInstanceOnly(instance);
		}

		// Register validation layer callback:
		if (debuglayer)
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
			createInfo.pfnUserCallback = debugUtilsMessengerCallback;
			res = vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugUtilsMessenger);
			assert(res == VK_SUCCESS);
		}


		// Surface creation:
		{
#ifdef _WIN32
			VkWin32SurfaceCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			createInfo.hwnd = window;
			createInfo.hinstance = GetModuleHandle(nullptr);

			auto CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");

			if (!CreateWin32SurfaceKHR || CreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
				assert(0);
			}
#elif SDL2
			if (!SDL_Vulkan_CreateSurface(window, instance, &surface))
			{
				throw sdl2::SDLError("Error creating a vulkan surface");
			}
#else
#error WICKEDENGINE VULKAN DEVICE ERROR: PLATFORM NOT SUPPORTED
#endif // _WIN32
		}


		// Enumerating and creating devices:
		{
			uint32_t deviceCount = 0;
			VkResult res = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
			assert(res == VK_SUCCESS);

			if (deviceCount == 0) {
				wiHelper::messageBox("failed to find GPUs with Vulkan support!");
				assert(0);
			}

			std::vector<VkPhysicalDevice> devices(deviceCount);
			res = vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
			assert(res == VK_SUCCESS);

			const std::vector<const char*> required_deviceExtensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME,
			};
			std::vector<const char*> enabled_deviceExtensions;

			for (const auto& dev : devices) 
			{
				bool suitable = true;

				uint32_t extensionCount;
				VkResult res = vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);
				assert(res == VK_SUCCESS);
				std::vector<VkExtensionProperties> available_deviceExtensions(extensionCount);
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
					continue;

				// Swapchain query:
				res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &swapchain_capabilities);
				assert(res == VK_SUCCESS);

				uint32_t formatCount;
				res = vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, nullptr);
				assert(res == VK_SUCCESS);

				if (formatCount != 0)
				{
					swapchain_formats.resize(formatCount);
					res = vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, swapchain_formats.data());
					assert(res == VK_SUCCESS);
				}

				uint32_t presentModeCount;
				res = vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentModeCount, nullptr);
				assert(res == VK_SUCCESS);

				if (presentModeCount != 0) {
					swapchain_presentModes.resize(presentModeCount);
					res = vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentModeCount, swapchain_presentModes.data());
					assert(res == VK_SUCCESS);
				}

				suitable = !swapchain_formats.empty() && !swapchain_presentModes.empty();

				if (suitable) 
				{
					features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
					features_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
					features_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
					features2.pNext = &features_1_1;
					features_1_1.pNext = &features_1_2;
					void** features_chain = &features_1_2.pNext;
					acceleration_structure_features = {};
					raytracing_features = {};
					raytracing_query_features = {};
					fragment_shading_rate_features = {};
					mesh_shader_features = {};

					properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
					properties_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
					properties_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
					properties2.pNext = &properties_1_1;
					properties_1_1.pNext = &properties_1_2;
					void** properties_chain = &properties_1_2.pNext;
					acceleration_structure_properties = {};
					raytracing_properties = {};
					fragment_shading_rate_properties = {};
					mesh_shader_properties = {};

					enabled_deviceExtensions = required_deviceExtensions;

					if (checkExtensionSupport(VK_KHR_SPIRV_1_4_EXTENSION_NAME, available_deviceExtensions))
					{
						enabled_deviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
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

					if (!DEBUGDEVICE && checkExtensionSupport(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, available_deviceExtensions))
					{
						// Note: VRS will crash vulkan validation layers: https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/2473
						enabled_deviceExtensions.push_back(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
						fragment_shading_rate_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
						*features_chain = &fragment_shading_rate_features;
						features_chain = &fragment_shading_rate_features.pNext;
						fragment_shading_rate_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
						*properties_chain = &fragment_shading_rate_properties;
						properties_chain = &fragment_shading_rate_properties.pNext;
					}

					if (checkExtensionSupport(VK_NV_MESH_SHADER_EXTENSION_NAME, available_deviceExtensions))
					{
						enabled_deviceExtensions.push_back(VK_NV_MESH_SHADER_EXTENSION_NAME);
						mesh_shader_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
						*features_chain = &mesh_shader_features;
						features_chain = &mesh_shader_features.pNext;
						mesh_shader_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
						*properties_chain = &mesh_shader_properties;
						properties_chain = &mesh_shader_properties.pNext;
					}

					vkGetPhysicalDeviceProperties2(dev, &properties2);

					bool discrete = properties2.properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
					if (discrete || physicalDevice == VK_NULL_HANDLE)
					{
						physicalDevice = dev;
						if (discrete)
						{
							break; // if this is discrete GPU, look no further (prioritize discrete GPU)
						}
					}
				}
			}

			if (physicalDevice == VK_NULL_HANDLE)
			{
				wiHelper::messageBox("failed to find a suitable GPU!");
				assert(0);
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
			if (features2.features.tessellationShader == VK_TRUE)
			{
				capabilities |= GRAPHICSDEVICE_CAPABILITY_TESSELLATION;
			}
			if (features2.features.shaderStorageImageExtendedFormats == VK_TRUE)
			{
				capabilities |= GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_COMMON;
			}
			capabilities |= GRAPHICSDEVICE_CAPABILITY_RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS; // let's hope for the best...

			if (raytracing_features.rayTracingPipeline == VK_TRUE)
			{
				assert(acceleration_structure_features.accelerationStructure == VK_TRUE);
				assert(features_1_2.bufferDeviceAddress == VK_TRUE);
				capabilities |= GRAPHICSDEVICE_CAPABILITY_RAYTRACING;
				SHADER_IDENTIFIER_SIZE = raytracing_properties.shaderGroupHandleSize;
			}
			if (raytracing_query_features.rayQuery == VK_TRUE)
			{
				assert(acceleration_structure_features.accelerationStructure == VK_TRUE);
				assert(features_1_2.bufferDeviceAddress == VK_TRUE);
				capabilities |= GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE;
			}
			if (mesh_shader_features.meshShader == VK_TRUE && mesh_shader_features.taskShader == VK_TRUE)
			{
				capabilities |= GRAPHICSDEVICE_CAPABILITY_MESH_SHADER;
			}
			if (fragment_shading_rate_features.pipelineFragmentShadingRate == VK_TRUE)
			{
				capabilities |= GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING;
			}
			if (fragment_shading_rate_features.attachmentFragmentShadingRate == VK_TRUE)
			{
				capabilities |= GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING_TIER2;
				VARIABLE_RATE_SHADING_TILE_SIZE = std::min(fragment_shading_rate_properties.maxFragmentShadingRateAttachmentTexelSize.width, fragment_shading_rate_properties.maxFragmentShadingRateAttachmentTexelSize.height);
			}

			assert(features_1_2.hostQueryReset == VK_TRUE);

			if (features_1_2.descriptorIndexing)
			{
				capabilities |= GRAPHICSDEVICE_CAPABILITY_BINDLESS_DESCRIPTORS;
			}
			
			VkFormatProperties formatProperties = {};
			vkGetPhysicalDeviceFormatProperties(physicalDevice, _ConvertFormat(FORMAT_R11G11B10_FLOAT), &formatProperties);
			if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)
			{
				capabilities |= GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_R11G11B10_FLOAT;
			}

			// Find queue families:
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

			// Query base queue families:
			int familyIndex = 0;
			for (const auto& queueFamily : queueFamilies)
			{
				VkBool32 presentSupport = false;
				VkResult res = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, (uint32_t)familyIndex, surface, &presentSupport);
				assert(res == VK_SUCCESS);

				if (presentFamily < 0 && queueFamily.queueCount > 0 && presentSupport) {
					presentFamily = familyIndex;
				}

				if (graphicsFamily < 0 && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					graphicsFamily = familyIndex;
				}

				if (copyFamily < 0 && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
					copyFamily = familyIndex;
				}

				if (computeFamily < 0 && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
					computeFamily = familyIndex;
				}

				familyIndex++;
			}

			// Now try to find dedicated compute and transfer queues:
			familyIndex = 0;
			for (const auto& queueFamily : queueFamilies)
			{
				if (queueFamily.queueCount > 0 &&
					queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT &&
					!(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
					!(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
					) {
					copyFamily = familyIndex;
				}

				if (queueFamily.queueCount > 0 &&
					queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT &&
					!(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					) {
					computeFamily = familyIndex;
				}

				familyIndex++;
			}

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			std::set<int> uniqueQueueFamilies = { graphicsFamily, presentFamily, copyFamily, computeFamily };

			float queuePriority = 1.0f;
			for (int queueFamily : uniqueQueueFamilies)
			{
				VkDeviceQueueCreateInfo queueCreateInfo = {};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueFamily;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueCreateInfo);
			}

			VkDeviceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

			createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			createInfo.pQueueCreateInfos = queueCreateInfos.data();

			createInfo.pEnabledFeatures = nullptr;
			createInfo.pNext = &features2;

			createInfo.enabledExtensionCount = static_cast<uint32_t>(enabled_deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = enabled_deviceExtensions.data();

			if (debuglayer)
			{
				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();
			}
			else
			{
				createInfo.enabledLayerCount = 0;
			}

			res = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
			assert(res == VK_SUCCESS);

			volkLoadDevice(device);

			vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
			vkGetDeviceQueue(device, presentFamily, 0, &presentQueue);
			vkGetDeviceQueue(device, copyFamily, 0, &copyQueue);
			vkGetDeviceQueue(device, computeFamily, 0, &computeQueue);
		}

		allocationhandler = std::make_shared<AllocationHandler>();
		allocationhandler->device = device;
		allocationhandler->instance = instance;

		// Initialize Vulkan Memory Allocator helper:
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.device = device;
		allocatorInfo.instance = instance;
		if (features_1_2.bufferDeviceAddress)
		{
			allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		}
		res = vmaCreateAllocator(&allocatorInfo, &allocationhandler->allocator);
		assert(res == VK_SUCCESS);

		CreateBackBufferResources();

		// Create frame resources:
		for (uint32_t fr = 0; fr < BACKBUFFER_COUNT; ++fr)
		{
			// Fence:
			{
				VkFenceCreateInfo fenceInfo = {};
				fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				//fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
				VkResult res = vkCreateFence(device, &fenceInfo, nullptr, &frames[fr].frameFence);
				assert(res == VK_SUCCESS);
			}

			// Create resources for transition command buffer:
			{
				VkCommandPoolCreateInfo poolInfo = {};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.queueFamilyIndex = graphicsFamily;
				poolInfo.flags = 0; // Optional

				res = vkCreateCommandPool(device, &poolInfo, nullptr, &frames[fr].transitionCommandPool);
				assert(res == VK_SUCCESS);

				VkCommandBufferAllocateInfo commandBufferInfo = {};
				commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				commandBufferInfo.commandBufferCount = 1;
				commandBufferInfo.commandPool = frames[fr].transitionCommandPool;
				commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

				res = vkAllocateCommandBuffers(device, &commandBufferInfo, &frames[fr].transitionCommandBuffer);
				assert(res == VK_SUCCESS);

				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				beginInfo.pInheritanceInfo = nullptr; // Optional

				res = vkBeginCommandBuffer(frames[fr].transitionCommandBuffer, &beginInfo);
				assert(res == VK_SUCCESS);
			}

			// Create resources for copy (transfer) queue:
			{
				VkCommandPoolCreateInfo poolInfo = {};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.queueFamilyIndex = copyFamily;
				poolInfo.flags = 0; // Optional

				res = vkCreateCommandPool(device, &poolInfo, nullptr, &frames[fr].copyCommandPool);
				assert(res == VK_SUCCESS);

				VkCommandBufferAllocateInfo commandBufferInfo = {};
				commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				commandBufferInfo.commandBufferCount = 1;
				commandBufferInfo.commandPool = frames[fr].copyCommandPool;
				commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

				res = vkAllocateCommandBuffers(device, &commandBufferInfo, &frames[fr].copyCommandBuffer);
				assert(res == VK_SUCCESS);

				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				res = vkBeginCommandBuffer(frames[fr].copyCommandBuffer, &beginInfo);
				assert(res == VK_SUCCESS);
			}
		}

		// Create semaphores:
		{
			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			res = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &copySemaphore);
			assert(res == VK_SUCCESS);

			for (uint32_t i = 0; i < BACKBUFFER_COUNT; i++)
			{
				res = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frames[i].swapchainAcquireSemaphore);
				assert(res == VK_SUCCESS);
				res = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frames[i].swapchainReleaseSemaphore);
				assert(res == VK_SUCCESS);
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
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

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
			copyQueueLock.lock();
			{
				auto& frame = GetFrameResources();
				if (!copyQueueUse)
				{
					copyQueueUse = true;

					res = vkResetCommandPool(device, frame.copyCommandPool, 0);
					assert(res == VK_SUCCESS);

					VkCommandBufferBeginInfo beginInfo = {};
					beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
					beginInfo.pInheritanceInfo = nullptr; // Optional

					res = vkBeginCommandBuffer(frame.copyCommandBuffer, &beginInfo);
					assert(res == VK_SUCCESS);
				}

				VkImageMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.oldLayout = imageInfo.initialLayout;
				barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = nullImage1D;
				barrier.subresourceRange.layerCount = 1;
				frame.loadedimagetransitions.push_back(barrier);
				barrier.image = nullImage2D;
				barrier.subresourceRange.layerCount = 6;
				frame.loadedimagetransitions.push_back(barrier);
				barrier.image = nullImage3D;
				barrier.subresourceRange.layerCount = 1;
				frame.loadedimagetransitions.push_back(barrier);
			}
			copyQueueLock.unlock();

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
		pso_dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		pso_dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
		pso_dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
		pso_dynamicStates.push_back(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
		if (CheckCapability(GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING))
		{
			pso_dynamicStates.push_back(VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR);
		}

		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = (uint32_t)pso_dynamicStates.size();
		dynamicStateInfo.pDynamicStates = pso_dynamicStates.data();


		allocationhandler->bindlessUniformBuffers.init(device, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, properties_1_2.maxDescriptorSetUpdateAfterBindUniformBuffers / 4);
		allocationhandler->bindlessSampledImages.init(device, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, properties_1_2.maxDescriptorSetUpdateAfterBindSampledImages / 4);
		allocationhandler->bindlessUniformTexelBuffers.init(device, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, properties_1_2.maxDescriptorSetUpdateAfterBindSampledImages / 4);
		allocationhandler->bindlessStorageBuffers.init(device, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, properties_1_2.maxDescriptorSetUpdateAfterBindStorageBuffers / 4);
		allocationhandler->bindlessStorageImages.init(device, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, properties_1_2.maxDescriptorSetUpdateAfterBindStorageImages / 4);
		allocationhandler->bindlessStorageTexelBuffers.init(device, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, properties_1_2.maxDescriptorSetUpdateAfterBindStorageImages / 4);
		allocationhandler->bindlessSamplers.init(device, VK_DESCRIPTOR_TYPE_SAMPLER, 256);
		if (CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING))
		{
			allocationhandler->bindlessAccelerationStructures.init(device, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 32);
		}


		wiBackLog::post("Created GraphicsDevice_Vulkan");
	}
	GraphicsDevice_Vulkan::~GraphicsDevice_Vulkan()
	{
		VkResult res = vkQueueWaitIdle(graphicsQueue);
		assert(res == VK_SUCCESS);
		res = vkQueueWaitIdle(presentQueue);
		assert(res == VK_SUCCESS);

		for (auto& frame : frames)
		{
			vkDestroyFence(device, frame.frameFence, nullptr);
			for (auto& commandPool : frame.commandPools)
			{
				vkDestroyCommandPool(device, commandPool, nullptr);
			}
			vkDestroyCommandPool(device, frame.transitionCommandPool, nullptr);
			vkDestroyCommandPool(device, frame.copyCommandPool, nullptr);

			for (auto& descriptormanager : frame.descriptors)
			{
				descriptormanager.destroy();
			}

			vkDestroySemaphore(device, frame.swapchainAcquireSemaphore, nullptr);
			vkDestroySemaphore(device, frame.swapchainReleaseSemaphore, nullptr);
		}

		vkDestroySemaphore(device, copySemaphore, nullptr);

		for (auto& x : pso_layout_cache)
		{
			vkDestroyPipelineLayout(device, x.second.pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, x.second.descriptorSetLayout, nullptr);
		}

		for (auto& x : pipelines_worker)
		{
			for (auto& y : x)
			{
				vkDestroyPipeline(device, y.second, nullptr);
			}
		}
		for (auto& x : pipelines_global)
		{
			vkDestroyPipeline(device, x.second, nullptr);
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

		vkDestroyRenderPass(device, defaultRenderPass, nullptr);
		for (size_t i = 0; i < swapChainImages.size(); ++i)
		{
			vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
			vkDestroyImageView(device, swapChainImageViews[i], nullptr);
		}
		vkDestroySwapchainKHR(device, swapChain, nullptr);

		if (debugUtilsMessenger != VK_NULL_HANDLE)
		{
			vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
		}

		vkDestroySurfaceKHR(instance, surface, nullptr);
	}

	void GraphicsDevice_Vulkan::CreateBackBufferResources()
	{
		VkSurfaceFormatKHR surfaceFormat = {};
		surfaceFormat.format = _ConvertFormat(BACKBUFFER_FORMAT);
		bool valid = false;

		for (const auto& format : swapchain_formats)
		{
			if (format.format == surfaceFormat.format)
			{
				surfaceFormat = format;
				valid = true;
				break;
			}
		}
		if (!valid)
		{
			surfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
			BACKBUFFER_FORMAT = FORMAT_B8G8R8A8_UNORM;
		}

		VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapchain_capabilities);
		assert(res == VK_SUCCESS);

		swapChainExtent = { static_cast<uint32_t>(RESOLUTIONWIDTH), static_cast<uint32_t>(RESOLUTIONHEIGHT) };
		swapChainExtent.width = std::max(swapchain_capabilities.minImageExtent.width, std::min(swapchain_capabilities.maxImageExtent.width, swapChainExtent.width));
		swapChainExtent.height = std::max(swapchain_capabilities.minImageExtent.height, std::min(swapchain_capabilities.maxImageExtent.height, swapChainExtent.height));


		uint32_t imageCount = BACKBUFFER_COUNT;

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = swapChainExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		uint32_t queueFamilyIndices[] = { (uint32_t)graphicsFamily, (uint32_t)presentFamily };

		if (graphicsFamily != presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = swapchain_capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // The only one that is always supported
		if (!VSYNC)
		{
			// The immediate present mode is not necessarily supported:
			for (auto& presentmode : swapchain_presentModes)
			{
				if (presentmode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					createInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
					break;
				}
			}
		}
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = swapChain;

		res = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain);
		assert(res == VK_SUCCESS);

		if (createInfo.oldSwapchain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(device, createInfo.oldSwapchain, nullptr);
		}

		res = vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		assert(res == VK_SUCCESS);
		assert(BACKBUFFER_COUNT <= imageCount);
		swapChainImages.resize(imageCount);
		res = vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
		assert(res == VK_SUCCESS);
		swapChainImageFormat = surfaceFormat.format;

		if (vkSetDebugUtilsObjectNameEXT != nullptr)
		{
			VkDebugUtilsObjectNameInfoEXT info = {};
			info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			info.pObjectName = "SWAPCHAIN";
			info.objectType = VK_OBJECT_TYPE_IMAGE;
			for (auto& x : swapChainImages)
			{
				info.objectHandle = (uint64_t)x;

				res = vkSetDebugUtilsObjectNameEXT(device, &info);
				assert(res == VK_SUCCESS);
			}
		}

		// Create default render pass:
		{
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = swapChainImageFormat;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;

			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;

			if (defaultRenderPass != VK_NULL_HANDLE)
			{
				allocationhandler->destroyer_renderpasses.push_back(std::make_pair(defaultRenderPass, allocationhandler->framecount));
			}
			res = vkCreateRenderPass(device, &renderPassInfo, nullptr, &defaultRenderPass);
			assert(res == VK_SUCCESS);

		}

		// Create swap chain render targets:
		swapChainImageViews.resize(swapChainImages.size());
		swapChainFramebuffers.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); ++i)
		{
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (swapChainImageViews[i] != VK_NULL_HANDLE)
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(swapChainImageViews[i], allocationhandler->framecount));
			}
			res = vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]);
			assert(res == VK_SUCCESS);

			VkImageView attachments[] = {
				swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = defaultRenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (swapChainFramebuffers[i] != VK_NULL_HANDLE)
			{
				allocationhandler->destroyer_framebuffers.push_back(std::make_pair(swapChainFramebuffers[i], allocationhandler->framecount));
			}
			res = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]);
			assert(res == VK_SUCCESS);
		}
	}

	void GraphicsDevice_Vulkan::SetResolution(int width, int height)
	{
		if (width != RESOLUTIONWIDTH || height != RESOLUTIONHEIGHT)
		{
			RESOLUTIONWIDTH = width;
			RESOLUTIONHEIGHT = height;

			CreateBackBufferResources();
		}
	}

	Texture GraphicsDevice_Vulkan::GetBackBuffer()
	{
		auto internal_state = std::make_shared<Texture_Vulkan>();
		internal_state->resource = swapChainImages[swapChainImageIndex];

		Texture result;
		result.type = GPUResource::GPU_RESOURCE_TYPE::TEXTURE;
		result.internal_state = internal_state;
		result.desc.type = TextureDesc::TEXTURE_2D;
		result.desc.Width = swapChainExtent.width;
		result.desc.Height = swapChainExtent.height;
		result.desc.Format = BACKBUFFER_FORMAT;
		return result;
	}

	bool GraphicsDevice_Vulkan::CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *pBuffer) const
	{
		auto internal_state = std::make_shared<Buffer_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		pBuffer->internal_state = internal_state;
		pBuffer->type = GPUResource::GPU_RESOURCE_TYPE::BUFFER;

		pBuffer->desc = *pDesc;

		if (pDesc->Usage == USAGE_DYNAMIC && pDesc->BindFlags & BIND_CONSTANT_BUFFER)
		{
			// this special case will use frame allocator
			return true;
		}

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = pBuffer->desc.ByteWidth;
		bufferInfo.usage = 0;
		if (pBuffer->desc.BindFlags & BIND_VERTEX_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}
		if (pBuffer->desc.BindFlags & BIND_INDEX_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}
		if (pBuffer->desc.BindFlags & BIND_CONSTANT_BUFFER)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		}
		if (pBuffer->desc.BindFlags & BIND_SHADER_RESOURCE)
		{
			if (pBuffer->desc.Format == FORMAT_UNKNOWN)
			{
				bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			}
			else
			{
				bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
			}
		}
		if (pBuffer->desc.BindFlags & BIND_UNORDERED_ACCESS)
		{
			if (pBuffer->desc.Format == FORMAT_UNKNOWN)
			{
				bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			}
			else
			{
				bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
			}
		}
		if (pBuffer->desc.MiscFlags & RESOURCE_MISC_INDIRECT_ARGS)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		}
		if (pBuffer->desc.MiscFlags & RESOURCE_MISC_RAY_TRACING)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		}
		if (features_1_2.bufferDeviceAddress == VK_TRUE)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		}
		bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		bufferInfo.flags = 0;

		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;



		VkResult res;

		VmaAllocationCreateInfo allocInfo = {};
		//allocInfo.flags = VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
		allocInfo.flags = VMA_ALLOCATION_CREATE_STRATEGY_MIN_FRAGMENTATION_BIT;
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		if (pDesc->Usage == USAGE_STAGING)
		{
			if (pDesc->CPUAccessFlags & CPU_ACCESS_READ)
			{
				allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			}
			else
			{
				allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
				allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			}
		}
		if (pDesc->Usage == USAGE_DYNAMIC)
		{
			allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		}

		res = vmaCreateBuffer(allocationhandler->allocator, &bufferInfo, &allocInfo, &internal_state->resource, &internal_state->allocation, nullptr);
		assert(res == VK_SUCCESS);

		if (bufferInfo.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			VkBufferDeviceAddressInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			info.buffer = internal_state->resource;
			internal_state->address = vkGetBufferDeviceAddress(device, &info);
		}

		// Issue data copy on request:
		if (pInitialData != nullptr)
		{
			GPUBufferDesc uploaddesc;
			uploaddesc.ByteWidth = pDesc->ByteWidth;
			uploaddesc.Usage = USAGE_STAGING;
			GPUBuffer uploadbuffer;
			bool upload_success = CreateBuffer(&uploaddesc, nullptr, &uploadbuffer);
			assert(upload_success);
			VkBuffer upload_resource = to_internal(&uploadbuffer)->resource;
			VmaAllocation upload_allocation = to_internal(&uploadbuffer)->allocation;

			void* pData = upload_allocation->GetMappedData();
			assert(pData != nullptr);

			memcpy(pData, pInitialData->pSysMem, pBuffer->desc.ByteWidth);

			copyQueueLock.lock();
			{
				auto& frame = GetFrameResources();
				if (!copyQueueUse)
				{
					copyQueueUse = true;

					res = vkResetCommandPool(device, frame.copyCommandPool, 0);
					assert(res == VK_SUCCESS);

					VkCommandBufferBeginInfo beginInfo = {};
					beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
					beginInfo.pInheritanceInfo = nullptr; // Optional

					res = vkBeginCommandBuffer(frame.copyCommandBuffer, &beginInfo);
					assert(res == VK_SUCCESS);
				}

				VkBufferMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				barrier.buffer = internal_state->resource;
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.size = VK_WHOLE_SIZE;

				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				vkCmdPipelineBarrier(
					frame.copyCommandBuffer,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0, nullptr,
					1, &barrier,
					0, nullptr
				);

				VkBufferCopy copyRegion = {};
				copyRegion.size = pBuffer->desc.ByteWidth;
				copyRegion.srcOffset = 0;
				copyRegion.dstOffset = 0;

				vkCmdCopyBuffer(
					frame.copyCommandBuffer,
					upload_resource,
					internal_state->resource,
					1,
					&copyRegion
				);

				VkAccessFlags tmp = barrier.srcAccessMask;
				barrier.srcAccessMask = barrier.dstAccessMask;
				barrier.dstAccessMask = 0;

				if (pBuffer->desc.BindFlags & BIND_CONSTANT_BUFFER)
				{
					barrier.dstAccessMask |= VK_ACCESS_UNIFORM_READ_BIT;
				}
				if (pBuffer->desc.BindFlags & BIND_VERTEX_BUFFER)
				{
					barrier.dstAccessMask |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
				}
				if (pBuffer->desc.BindFlags & BIND_INDEX_BUFFER)
				{
					barrier.dstAccessMask |= VK_ACCESS_INDEX_READ_BIT;
				}
				if(pBuffer->desc.BindFlags & BIND_SHADER_RESOURCE)
				{
					barrier.dstAccessMask |= VK_ACCESS_SHADER_READ_BIT;
				}
				if (pBuffer->desc.BindFlags & BIND_UNORDERED_ACCESS)
				{
					barrier.dstAccessMask |= VK_ACCESS_SHADER_WRITE_BIT;
				}

				// transfer queue-ownership from copy to graphics:
				barrier.srcQueueFamilyIndex = copyFamily;
				barrier.dstQueueFamilyIndex = graphicsFamily;

				vkCmdPipelineBarrier(
					frame.copyCommandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					0,
					0, nullptr,
					1, &barrier,
					0, nullptr
				);
			}
			copyQueueLock.unlock();
		}

		if (pDesc->Format == FORMAT_UNKNOWN)
		{
			internal_state->is_typedbuffer = false;
		}
		else
		{
			internal_state->is_typedbuffer = true;
		}


		// Create resource views if needed
		if (pDesc->BindFlags & BIND_CONSTANT_BUFFER)
		{
			CreateSubresource(pBuffer, CBV, 0);
		}
		if (pDesc->BindFlags & BIND_SHADER_RESOURCE)
		{
			CreateSubresource(pBuffer, SRV, 0);
		}
		if (pDesc->BindFlags & BIND_UNORDERED_ACCESS)
		{
			CreateSubresource(pBuffer, UAV, 0);
		}



		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateTexture(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture *pTexture) const
	{
		auto internal_state = std::make_shared<Texture_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		pTexture->internal_state = internal_state;
		pTexture->type = GPUResource::GPU_RESOURCE_TYPE::TEXTURE;

		pTexture->desc = *pDesc;

		if (pTexture->desc.MipLevels == 0)
		{
			pTexture->desc.MipLevels = (uint32_t)log2(std::max(pTexture->desc.Width, pTexture->desc.Height)) + 1;
		}

		VmaAllocationCreateInfo allocInfo = {};
		//allocInfo.flags = VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
		allocInfo.flags = VMA_ALLOCATION_CREATE_STRATEGY_MIN_FRAGMENTATION_BIT;
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.extent.width = pTexture->desc.Width;
		imageInfo.extent.height = pTexture->desc.Height;
		imageInfo.extent.depth = 1;
		imageInfo.format = _ConvertFormat(pTexture->desc.Format);
		imageInfo.arrayLayers = pTexture->desc.ArraySize;
		imageInfo.mipLevels = pTexture->desc.MipLevels;
		imageInfo.samples = (VkSampleCountFlagBits)pTexture->desc.SampleCount;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = 0;
		if (pTexture->desc.BindFlags & BIND_SHADER_RESOURCE)
		{
			imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		if (pTexture->desc.BindFlags & BIND_UNORDERED_ACCESS)
		{
			imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
		}
		if (pTexture->desc.BindFlags & BIND_RENDER_TARGET)
		{
			imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			allocInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		}
		if (pTexture->desc.BindFlags & BIND_DEPTH_STENCIL)
		{
			imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			allocInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		}
		imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		imageInfo.flags = 0;
		if (pTexture->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
		{
			imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		}

		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		switch (pTexture->desc.type)
		{
		case TextureDesc::TEXTURE_1D:
			imageInfo.imageType = VK_IMAGE_TYPE_1D;
			break;
		case TextureDesc::TEXTURE_2D:
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			break;
		case TextureDesc::TEXTURE_3D:
			imageInfo.imageType = VK_IMAGE_TYPE_3D;
			imageInfo.extent.depth = pTexture->desc.Depth;
			break;
		default:
			assert(0);
			break;
		}

		VkResult res;

		if (pTexture->desc.Usage == USAGE_STAGING)
		{
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = imageInfo.extent.width * imageInfo.extent.height * imageInfo.extent.depth * imageInfo.arrayLayers *
				GetFormatStride(pTexture->desc.Format);

			if (pDesc->CPUAccessFlags & CPU_ACCESS_READ)
			{
				allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			}
			else
			{
				allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
				allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			}

			res = vmaCreateBuffer(allocationhandler->allocator, &bufferInfo, &allocInfo, &internal_state->staging_resource, &internal_state->allocation, nullptr);
			assert(res == VK_SUCCESS);

			imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
			VkImage image;
			res = vkCreateImage(device, &imageInfo, nullptr, &image);
			assert(res == VK_SUCCESS);

			VkImageSubresource subresource = {};
			subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			vkGetImageSubresourceLayout(device, image, &subresource, &internal_state->subresourcelayout);

			vkDestroyImage(device, image, nullptr);
			return res == VK_SUCCESS;
		}
		else
		{
			res = vmaCreateImage(allocationhandler->allocator, &imageInfo, &allocInfo, &internal_state->resource, &internal_state->allocation, nullptr);
			assert(res == VK_SUCCESS);
		}

		// Issue data copy on request:
		if (pInitialData != nullptr)
		{
			GPUBufferDesc uploaddesc;
			uploaddesc.ByteWidth = (uint32_t)internal_state->allocation->GetSize();
			uploaddesc.Usage = USAGE_STAGING;
			GPUBuffer uploadbuffer;
			bool upload_success = CreateBuffer(&uploaddesc, nullptr, &uploadbuffer);
			assert(upload_success);
			VkBuffer upload_resource = to_internal(&uploadbuffer)->resource;
			VmaAllocation upload_allocation = to_internal(&uploadbuffer)->allocation;

			void* pData = upload_allocation->GetMappedData();
			assert(pData != nullptr);

			std::vector<VkBufferImageCopy> copyRegions;

			size_t cpyoffset = 0;
			uint32_t initDataIdx = 0;
			uint32_t width = imageInfo.extent.width;
			uint32_t height = imageInfo.extent.height;
			uint32_t depth = imageInfo.extent.depth;
			for (uint32_t mip = 0; mip < pDesc->MipLevels; ++mip)
			{
				const SubresourceData& subresourceData = pInitialData[initDataIdx++];
				size_t cpysize = subresourceData.SysMemPitch * height * depth;
				if (IsFormatBlockCompressed(pDesc->Format))
				{
					cpysize /= 4;
				}
				uint8_t* cpyaddr = (uint8_t*)pData + cpyoffset;
				memcpy(cpyaddr, subresourceData.pSysMem, cpysize);

				VkBufferImageCopy copyRegion = {};
				copyRegion.bufferOffset = cpyoffset;
				copyRegion.bufferRowLength = 0;
				copyRegion.bufferImageHeight = 0;

				copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.imageSubresource.mipLevel = mip;
				copyRegion.imageSubresource.baseArrayLayer = 0;
				copyRegion.imageSubresource.layerCount = 1;

				copyRegion.imageOffset = { 0, 0, 0 };
				copyRegion.imageExtent = {
					width,
					height,
					depth
				};

				width = std::max(1u, width / 2);
				height = std::max(1u, height / 2);
				depth = std::max(1u, depth / 2);

				copyRegions.push_back(copyRegion);

				cpyoffset += Align(cpysize, GetFormatStride(pDesc->Format));
			}

			copyQueueLock.lock();
			{
				auto& frame = GetFrameResources();
				if (!copyQueueUse)
				{
					copyQueueUse = true;

					res = vkResetCommandPool(device, frame.copyCommandPool, 0);
					assert(res == VK_SUCCESS);

					VkCommandBufferBeginInfo beginInfo = {};
					beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
					beginInfo.pInheritanceInfo = nullptr; // Optional

					res = vkBeginCommandBuffer(frame.copyCommandBuffer, &beginInfo);
					assert(res == VK_SUCCESS);
				}

				VkImageMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = internal_state->resource;
				barrier.oldLayout = imageInfo.initialLayout;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = imageInfo.arrayLayers;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = imageInfo.mipLevels;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				vkCmdPipelineBarrier(
					frame.copyCommandBuffer,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);

				vkCmdCopyBufferToImage(frame.copyCommandBuffer, upload_resource, internal_state->resource, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)copyRegions.size(), copyRegions.data());

				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = _ConvertImageLayout(pTexture->desc.layout);
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = _ParseImageLayout(pTexture->desc.layout);

				frame.loadedimagetransitions.push_back(barrier);

			}
			copyQueueLock.unlock();
		}
		else
		{
			copyQueueLock.lock();


			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = internal_state->resource;
			barrier.oldLayout = imageInfo.initialLayout;
			barrier.newLayout = _ConvertImageLayout(pTexture->desc.layout);
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = _ParseImageLayout(pTexture->desc.layout);
			if (pTexture->desc.BindFlags & BIND_DEPTH_STENCIL)
			{
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				if (IsFormatStencilSupport(pTexture->desc.Format))
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

			auto& frame = GetFrameResources();
			frame.loadedimagetransitions.push_back(barrier);

			copyQueueLock.unlock();
		}

		if (pTexture->desc.BindFlags & BIND_RENDER_TARGET)
		{
			CreateSubresource(pTexture, RTV, 0, -1, 0, -1);
		}
		if (pTexture->desc.BindFlags & BIND_DEPTH_STENCIL)
		{
			CreateSubresource(pTexture, DSV, 0, -1, 0, -1);
		}
		if (pTexture->desc.BindFlags & BIND_SHADER_RESOURCE)
		{
			CreateSubresource(pTexture, SRV, 0, -1, 0, -1);
		}
		if (pTexture->desc.BindFlags & BIND_UNORDERED_ACCESS)
		{
			CreateSubresource(pTexture, UAV, 0, -1, 0, -1);
		}

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateShader(SHADERSTAGE stage, const void *pShaderBytecode, size_t BytecodeLength, Shader *pShader) const
	{
		auto internal_state = std::make_shared<Shader_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		pShader->internal_state = internal_state;

		pShader->stage = stage;

		VkResult res = VK_SUCCESS;

		VkShaderModuleCreateInfo moduleInfo = {};
		moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleInfo.codeSize = BytecodeLength;
		moduleInfo.pCode = (const uint32_t*)pShaderBytecode;
		res = vkCreateShaderModule(device, &moduleInfo, nullptr, &internal_state->shaderModule);
		assert(res == VK_SUCCESS);

		internal_state->stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		internal_state->stageInfo.module = internal_state->shaderModule;
		internal_state->stageInfo.pName = "main";
		switch (stage)
		{
		case wiGraphics::MS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_MESH_BIT_NV;
			break;
		case wiGraphics::AS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_TASK_BIT_NV;
			break;
		case wiGraphics::VS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			break;
		case wiGraphics::HS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			break;
		case wiGraphics::DS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			break;
		case wiGraphics::GS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			break;
		case wiGraphics::PS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			break;
		case wiGraphics::CS:
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

			std::vector<SpvReflectDescriptorBinding*> bindings(binding_count);
			result = spvReflectEnumerateDescriptorBindings(
				&module, &binding_count, bindings.data()
			);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			uint32_t push_count = 0;
			result = spvReflectEnumeratePushConstantBlocks(&module, &push_count, nullptr);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			std::vector<SpvReflectBlockVariable*> pushconstants(push_count);
			result = spvReflectEnumeratePushConstantBlocks(&module, &push_count, pushconstants.data());
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			std::vector<VkSampler> staticsamplers;

			for (auto& x : pushconstants)
			{
				auto& push = internal_state->pushconstants;
				push.stageFlags = internal_state->stageInfo.stage;
				push.offset = x->offset;
				push.size = x->size;
			}

			for (auto& x : bindings)
			{
				const bool bindless = x->count == 0 || x->count > 1 || x->set > 0; // strange: unbounded array returns count == 1

				if (bindless)
				{
					// There can be padding between bindless spaces because sets need to be bound contiguously
					internal_state->bindlessBindings.resize(std::max(internal_state->bindlessBindings.size(), (size_t)x->set));
				}

				auto& descriptor = bindless ? internal_state->bindlessBindings[x->set - 1] : internal_state->layoutBindings.emplace_back();
				descriptor.stageFlags = internal_state->stageInfo.stage;
				descriptor.binding = x->binding;
				descriptor.descriptorCount = x->count;
				descriptor.descriptorType = (VkDescriptorType)x->descriptor_type;

				if (bindless)
				{
					continue;
				}

				auto& imageViewType = internal_state->imageViewTypes.emplace_back();
				imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;

				if (x->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER)
				{
					bool staticsampler = false;
					for (auto& sam : pShader->auto_samplers)
					{
						if (x->binding == sam.slot + VULKAN_BINDING_SHIFT_S)
						{
							descriptor.pImmutableSamplers = &to_internal(&sam.sampler)->resource;
							staticsampler = true;
							break; // static sampler will be used instead
						}
					}
					if (!staticsampler)
					{
						for (auto& sam : common_samplers)
						{
							if (x->binding == sam.slot + VULKAN_BINDING_SHIFT_S)
							{
								descriptor.pImmutableSamplers = &to_internal(&sam.sampler)->resource;
								staticsampler = true;
								break; // static sampler will be used instead
							}
						}
					}
					if (staticsampler)
					{
						continue;
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

			if (stage == CS || stage == LIB)
			{
				internal_state->binding_hash = 0;
				size_t i = 0;
				for (auto& x : internal_state->layoutBindings)
				{
					wiHelper::hash_combine(internal_state->binding_hash, x.binding);
					wiHelper::hash_combine(internal_state->binding_hash, x.descriptorCount);
					wiHelper::hash_combine(internal_state->binding_hash, x.descriptorType);
					wiHelper::hash_combine(internal_state->binding_hash, x.stageFlags);
					wiHelper::hash_combine(internal_state->binding_hash, internal_state->imageViewTypes[i++]);
				}
				for (auto& x : internal_state->bindlessBindings)
				{
					wiHelper::hash_combine(internal_state->binding_hash, x.binding);
					wiHelper::hash_combine(internal_state->binding_hash, x.descriptorCount);
					wiHelper::hash_combine(internal_state->binding_hash, x.descriptorType);
					wiHelper::hash_combine(internal_state->binding_hash, x.stageFlags);
				}

				pso_layout_cache_mutex.lock();
				if (pso_layout_cache[internal_state->binding_hash].pipelineLayout == VK_NULL_HANDLE)
				{
					std::vector<VkDescriptorSetLayout> layouts;

					{
						VkDescriptorSetLayoutCreateInfo descriptorSetlayoutInfo = {};
						descriptorSetlayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
						descriptorSetlayoutInfo.pBindings = internal_state->layoutBindings.data();
						descriptorSetlayoutInfo.bindingCount = uint32_t(internal_state->layoutBindings.size());
						res = vkCreateDescriptorSetLayout(device, &descriptorSetlayoutInfo, nullptr, &internal_state->descriptorSetLayout);
						assert(res == VK_SUCCESS);
						pso_layout_cache[internal_state->binding_hash].descriptorSetLayout = internal_state->descriptorSetLayout;
						layouts.push_back(internal_state->descriptorSetLayout);
					}

					internal_state->bindlessFirstSet = (uint32_t)layouts.size();
					for (auto& x : internal_state->bindlessBindings)
					{
						switch (x.descriptorType)
						{
						case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
							layouts.push_back(allocationhandler->bindlessUniformBuffers.descriptorSetLayout);
							internal_state->bindlessSets.push_back(allocationhandler->bindlessUniformBuffers.descriptorSet);
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
					pso_layout_cache[internal_state->binding_hash].pipelineLayout = internal_state->pipelineLayout_cs;
				}
				else
				{
					internal_state->descriptorSetLayout = pso_layout_cache[internal_state->binding_hash].descriptorSetLayout;
					internal_state->pipelineLayout_cs = pso_layout_cache[internal_state->binding_hash].pipelineLayout;
				}
				pso_layout_cache_mutex.unlock();
			}
		}

		if (stage == CS)
		{
			VkComputePipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipelineInfo.layout = internal_state->pipelineLayout_cs;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

			// Create compute pipeline state in place:
			pipelineInfo.stage = internal_state->stageInfo;


			res = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &internal_state->pipeline_cs);
			assert(res == VK_SUCCESS);
		}

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateSampler(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState) const
	{
		auto internal_state = std::make_shared<Sampler_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		pSamplerState->internal_state = internal_state;

		pSamplerState->desc = *pSamplerDesc;

		VkSamplerCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.flags = 0;
		createInfo.pNext = nullptr;


		switch (pSamplerDesc->Filter)
		{
		case FILTER_MIN_MAG_MIP_POINT:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case FILTER_MIN_MAG_POINT_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case FILTER_MIN_POINT_MAG_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case FILTER_MIN_LINEAR_MAG_MIP_POINT:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case FILTER_MIN_MAG_LINEAR_MIP_POINT:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case FILTER_MIN_MAG_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case FILTER_ANISOTROPIC:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = true;
			createInfo.compareEnable = false;
			break;
		case FILTER_COMPARISON_MIN_MAG_MIP_POINT:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case FILTER_COMPARISON_ANISOTROPIC:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = true;
			createInfo.compareEnable = true;
			break;
		case FILTER_MINIMUM_MIN_MAG_MIP_POINT:
		case FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
		case FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
		case FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
		case FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
		case FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		case FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
		case FILTER_MINIMUM_MIN_MAG_MIP_LINEAR:
		case FILTER_MINIMUM_ANISOTROPIC:
		case FILTER_MAXIMUM_MIN_MAG_MIP_POINT:
		case FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
		case FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
		case FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
		case FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
		case FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		case FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
		case FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR:
		case FILTER_MAXIMUM_ANISOTROPIC:
		default:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		}

		createInfo.addressModeU = _ConvertTextureAddressMode(pSamplerDesc->AddressU);
		createInfo.addressModeV = _ConvertTextureAddressMode(pSamplerDesc->AddressV);
		createInfo.addressModeW = _ConvertTextureAddressMode(pSamplerDesc->AddressW);
		createInfo.maxAnisotropy = static_cast<float>(pSamplerDesc->MaxAnisotropy);
		createInfo.compareOp = _ConvertComparisonFunc(pSamplerDesc->ComparisonFunc);
		createInfo.minLod = pSamplerDesc->MinLOD;
		createInfo.maxLod = pSamplerDesc->MaxLOD;
		createInfo.mipLodBias = pSamplerDesc->MipLODBias;
		createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
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
	bool GraphicsDevice_Vulkan::CreateQueryHeap(const GPUQueryHeapDesc* pDesc, GPUQueryHeap* pQueryHeap) const
	{
		auto internal_state = std::make_shared<QueryHeap_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		pQueryHeap->internal_state = internal_state;

		pQueryHeap->desc = *pDesc;

		VkQueryPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		poolInfo.queryCount = pDesc->queryCount;

		switch (pDesc->type)
		{
		case GPU_QUERY_TYPE_TIMESTAMP:
			poolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
			break;
		case GPU_QUERY_TYPE_OCCLUSION:
		case GPU_QUERY_TYPE_OCCLUSION_BINARY:
			poolInfo.queryType = VK_QUERY_TYPE_OCCLUSION;
			break;
		}

		VkResult res = vkCreateQueryPool(device, &poolInfo, nullptr, &internal_state->pool);
		assert(res == VK_SUCCESS);
		vkResetQueryPool(device, internal_state->pool, 0, poolInfo.queryCount);

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreatePipelineState(const PipelineStateDesc* pDesc, PipelineState* pso) const
	{
		auto internal_state = std::make_shared<PipelineState_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		pso->internal_state = internal_state;

		pso->desc = *pDesc;

		pso->hash = 0;
		wiHelper::hash_combine(pso->hash, pDesc->ms);
		wiHelper::hash_combine(pso->hash, pDesc->as);
		wiHelper::hash_combine(pso->hash, pDesc->vs);
		wiHelper::hash_combine(pso->hash, pDesc->ps);
		wiHelper::hash_combine(pso->hash, pDesc->hs);
		wiHelper::hash_combine(pso->hash, pDesc->ds);
		wiHelper::hash_combine(pso->hash, pDesc->gs);
		wiHelper::hash_combine(pso->hash, pDesc->il);
		wiHelper::hash_combine(pso->hash, pDesc->rs);
		wiHelper::hash_combine(pso->hash, pDesc->bs);
		wiHelper::hash_combine(pso->hash, pDesc->dss);
		wiHelper::hash_combine(pso->hash, pDesc->pt);
		wiHelper::hash_combine(pso->hash, pDesc->sampleMask);

		VkResult res = VK_SUCCESS;

		{
			// Descriptor set layout comes from reflection data when there is no root signature specified:

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
					}
					i++;
				}

				if (shader_internal->pushconstants.size > 0)
				{
					internal_state->pushconstants.offset = shader_internal->pushconstants.offset;
					internal_state->pushconstants.size = shader_internal->pushconstants.size;
					internal_state->pushconstants.stageFlags |= shader_internal->pushconstants.stageFlags;
				}
			};

			insert_shader(pDesc->ms);
			insert_shader(pDesc->as);
			insert_shader(pDesc->vs);
			insert_shader(pDesc->hs);
			insert_shader(pDesc->ds);
			insert_shader(pDesc->gs);
			insert_shader(pDesc->ps);

			auto insert_shader_bindless = [&](const Shader* shader) {
				if (shader == nullptr)
					return;
				auto shader_internal = to_internal(shader);

				internal_state->bindlessBindings.resize(std::max(internal_state->bindlessBindings.size(), shader_internal->bindlessBindings.size()));

				int i = 0;
				for (auto& x : shader_internal->bindlessBindings)
				{
					if (internal_state->bindlessBindings[i].descriptorType != x.descriptorType)
					{
						internal_state->bindlessBindings[i] = x;
					}
					else
					{
						internal_state->bindlessBindings[i].stageFlags |= x.stageFlags;
					}
					i++;
				}
			};

			insert_shader_bindless(pDesc->ms);
			insert_shader_bindless(pDesc->as);
			insert_shader_bindless(pDesc->vs);
			insert_shader_bindless(pDesc->hs);
			insert_shader_bindless(pDesc->ds);
			insert_shader_bindless(pDesc->gs);
			insert_shader_bindless(pDesc->ps);

			internal_state->binding_hash = 0;
			size_t i = 0;
			for (auto& x : internal_state->layoutBindings)
			{
				wiHelper::hash_combine(internal_state->binding_hash, x.binding);
				wiHelper::hash_combine(internal_state->binding_hash, x.descriptorCount);
				wiHelper::hash_combine(internal_state->binding_hash, x.descriptorType);
				wiHelper::hash_combine(internal_state->binding_hash, x.stageFlags);
				wiHelper::hash_combine(internal_state->binding_hash, internal_state->imageViewTypes[i++]);
			}
			for (auto& x : internal_state->bindlessBindings)
			{
				wiHelper::hash_combine(internal_state->binding_hash, x.binding);
				wiHelper::hash_combine(internal_state->binding_hash, x.descriptorCount);
				wiHelper::hash_combine(internal_state->binding_hash, x.descriptorType);
				wiHelper::hash_combine(internal_state->binding_hash, x.stageFlags);
			}

			pso_layout_cache_mutex.lock();
			if (pso_layout_cache[internal_state->binding_hash].pipelineLayout == VK_NULL_HANDLE)
			{
				std::vector<VkDescriptorSetLayout> layouts;
				{
					VkDescriptorSetLayoutCreateInfo descriptorSetlayoutInfo = {};
					descriptorSetlayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					descriptorSetlayoutInfo.pBindings = internal_state->layoutBindings.data();
					descriptorSetlayoutInfo.bindingCount = static_cast<uint32_t>(internal_state->layoutBindings.size());
					res = vkCreateDescriptorSetLayout(device, &descriptorSetlayoutInfo, nullptr, &internal_state->descriptorSetLayout);
					assert(res == VK_SUCCESS);
					pso_layout_cache[internal_state->binding_hash].descriptorSetLayout = internal_state->descriptorSetLayout;
					layouts.push_back(internal_state->descriptorSetLayout);
				}

				pso_layout_cache[internal_state->binding_hash].bindlessFirstSet = (uint32_t)layouts.size();
				for (auto& x : internal_state->bindlessBindings)
				{
					switch (x.descriptorType)
					{
					case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
						layouts.push_back(allocationhandler->bindlessUniformBuffers.descriptorSetLayout);
						pso_layout_cache[internal_state->binding_hash].bindlessSets.push_back(allocationhandler->bindlessUniformBuffers.descriptorSet);
						break;
					case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
						layouts.push_back(allocationhandler->bindlessSampledImages.descriptorSetLayout);
						pso_layout_cache[internal_state->binding_hash].bindlessSets.push_back(allocationhandler->bindlessSampledImages.descriptorSet);
						break;
					case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
						layouts.push_back(allocationhandler->bindlessUniformTexelBuffers.descriptorSetLayout);
						pso_layout_cache[internal_state->binding_hash].bindlessSets.push_back(allocationhandler->bindlessUniformTexelBuffers.descriptorSet);
						break;
					case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
						layouts.push_back(allocationhandler->bindlessStorageBuffers.descriptorSetLayout);
						pso_layout_cache[internal_state->binding_hash].bindlessSets.push_back(allocationhandler->bindlessStorageBuffers.descriptorSet);
						break;
					case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
						layouts.push_back(allocationhandler->bindlessStorageImages.descriptorSetLayout);
						pso_layout_cache[internal_state->binding_hash].bindlessSets.push_back(allocationhandler->bindlessStorageImages.descriptorSet);
						break;
					case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
						layouts.push_back(allocationhandler->bindlessStorageTexelBuffers.descriptorSetLayout);
						pso_layout_cache[internal_state->binding_hash].bindlessSets.push_back(allocationhandler->bindlessStorageTexelBuffers.descriptorSet);
						break;
					case VK_DESCRIPTOR_TYPE_SAMPLER:
						layouts.push_back(allocationhandler->bindlessSamplers.descriptorSetLayout);
						pso_layout_cache[internal_state->binding_hash].bindlessSets.push_back(allocationhandler->bindlessSamplers.descriptorSet);
						break;
					case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
						layouts.push_back(allocationhandler->bindlessAccelerationStructures.descriptorSetLayout);
						pso_layout_cache[internal_state->binding_hash].bindlessSets.push_back(allocationhandler->bindlessAccelerationStructures.descriptorSet);
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
				pso_layout_cache[internal_state->binding_hash].pipelineLayout = internal_state->pipelineLayout;
			}
			internal_state->descriptorSetLayout = pso_layout_cache[internal_state->binding_hash].descriptorSetLayout;
			internal_state->pipelineLayout = pso_layout_cache[internal_state->binding_hash].pipelineLayout;
			internal_state->bindlessSets = pso_layout_cache[internal_state->binding_hash].bindlessSets;
			internal_state->bindlessFirstSet = pso_layout_cache[internal_state->binding_hash].bindlessFirstSet;
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
		case POINTLIST:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			break;
		case LINELIST:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			break;
		case LINESTRIP:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
			break;
		case TRIANGLESTRIP:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			break;
		case TRIANGLELIST:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			break;
		case PATCHLIST:
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
		rasterizer.pNext = &depthclip;

		if (pso->desc.rs != nullptr)
		{
			const RasterizerState& desc = *pso->desc.rs;

			switch (desc.FillMode)
			{
			case FILL_WIREFRAME:
				rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
				break;
			case FILL_SOLID:
			default:
				rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
				break;
			}

			switch (desc.CullMode)
			{
			case CULL_BACK:
				rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
				break;
			case CULL_FRONT:
				rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
				break;
			case CULL_NONE:
			default:
				rasterizer.cullMode = VK_CULL_MODE_NONE;
				break;
			}

			rasterizer.frontFace = desc.FrontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
			rasterizer.depthBiasEnable = desc.DepthBias != 0 || desc.SlopeScaledDepthBias != 0;
			rasterizer.depthBiasConstantFactor = static_cast<float>(desc.DepthBias);
			rasterizer.depthBiasClamp = desc.DepthBiasClamp;
			rasterizer.depthBiasSlopeFactor = desc.SlopeScaledDepthBias;

			// depth clip is extension in Vulkan 1.1:
			depthclip.depthClipEnable = desc.DepthClipEnable ? VK_TRUE : VK_FALSE;
		}

		pipelineInfo.pRasterizationState = &rasterizer;


		// Viewport, Scissor:
		VkViewport& viewport = internal_state->viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = 65535;
		viewport.height = 65535;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;

		VkRect2D& scissor = internal_state->scissor;
		scissor.extent.width = 65535;
		scissor.extent.height = 65535;

		VkPipelineViewportStateCreateInfo& viewportState = internal_state->viewportState;
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		pipelineInfo.pViewportState = &viewportState;


		// Depth-Stencil:
		VkPipelineDepthStencilStateCreateInfo& depthstencil = internal_state->depthstencil;
		depthstencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		if (pso->desc.dss != nullptr)
		{
			depthstencil.depthTestEnable = pso->desc.dss->DepthEnable ? VK_TRUE : VK_FALSE;
			depthstencil.depthWriteEnable = pso->desc.dss->DepthWriteMask == DEPTH_WRITE_MASK_ZERO ? VK_FALSE : VK_TRUE;
			depthstencil.depthCompareOp = _ConvertComparisonFunc(pso->desc.dss->DepthFunc);

			depthstencil.stencilTestEnable = pso->desc.dss->StencilEnable ? VK_TRUE : VK_FALSE;

			depthstencil.front.compareMask = pso->desc.dss->StencilReadMask;
			depthstencil.front.writeMask = pso->desc.dss->StencilWriteMask;
			depthstencil.front.reference = 0; // runtime supplied
			depthstencil.front.compareOp = _ConvertComparisonFunc(pso->desc.dss->FrontFace.StencilFunc);
			depthstencil.front.passOp = _ConvertStencilOp(pso->desc.dss->FrontFace.StencilPassOp);
			depthstencil.front.failOp = _ConvertStencilOp(pso->desc.dss->FrontFace.StencilFailOp);
			depthstencil.front.depthFailOp = _ConvertStencilOp(pso->desc.dss->FrontFace.StencilDepthFailOp);

			depthstencil.back.compareMask = pso->desc.dss->StencilReadMask;
			depthstencil.back.writeMask = pso->desc.dss->StencilWriteMask;
			depthstencil.back.reference = 0; // runtime supplied
			depthstencil.back.compareOp = _ConvertComparisonFunc(pso->desc.dss->BackFace.StencilFunc);
			depthstencil.back.passOp = _ConvertStencilOp(pso->desc.dss->BackFace.StencilPassOp);
			depthstencil.back.failOp = _ConvertStencilOp(pso->desc.dss->BackFace.StencilFailOp);
			depthstencil.back.depthFailOp = _ConvertStencilOp(pso->desc.dss->BackFace.StencilDepthFailOp);

			depthstencil.depthBoundsTestEnable = VK_FALSE;
		}

		pipelineInfo.pDepthStencilState = &depthstencil;


		// Tessellation:
		VkPipelineTessellationStateCreateInfo& tessellationInfo = internal_state->tessellationInfo;
		tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		tessellationInfo.patchControlPoints = 3;

		pipelineInfo.pTessellationState = &tessellationInfo;


		pipelineInfo.pDynamicState = &dynamicStateInfo;

		return res == VK_TRUE;
	}
	bool GraphicsDevice_Vulkan::CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass) const
	{
		auto internal_state = std::make_shared<RenderPass_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		renderpass->internal_state = internal_state;

		renderpass->desc = *pDesc;

		renderpass->hash = 0;
		wiHelper::hash_combine(renderpass->hash, pDesc->attachments.size());
		for (auto& attachment : pDesc->attachments)
		{
			if (attachment.type == RenderPassAttachment::RENDERTARGET || attachment.type == RenderPassAttachment::DEPTH_STENCIL)
			{
				wiHelper::hash_combine(renderpass->hash, attachment.texture->desc.Format);
				wiHelper::hash_combine(renderpass->hash, attachment.texture->desc.SampleCount);
			}
		}

		VkResult res;

		VkImageView attachments[18] = {};
		VkAttachmentDescription2 attachmentDescriptions[18] = {};
		VkAttachmentReference2 colorAttachmentRefs[8] = {};
		VkAttachmentReference2 resolveAttachmentRefs[8] = {};
		VkAttachmentReference2 shadingRateAttachmentRef = {};
		VkAttachmentReference2 depthAttachmentRef = {};

		VkFragmentShadingRateAttachmentInfoKHR shading_rate_attachment = {};
		shading_rate_attachment.sType = VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
		shading_rate_attachment.pFragmentShadingRateAttachment = &shadingRateAttachmentRef;
		shading_rate_attachment.shadingRateAttachmentTexelSize.width = VARIABLE_RATE_SHADING_TILE_SIZE;
		shading_rate_attachment.shadingRateAttachmentTexelSize.height = VARIABLE_RATE_SHADING_TILE_SIZE;

		int resolvecount = 0;

		VkSubpassDescription2 subpass = {};
		subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		const RenderPassDesc& desc = renderpass->desc;

		uint32_t validAttachmentCount = 0;
		for (auto& attachment : renderpass->desc.attachments)
		{
			const Texture* texture = attachment.texture;
			const TextureDesc& texdesc = texture->desc;
			int subresource = attachment.subresource;
			auto texture_internal_state = to_internal(texture);

			attachmentDescriptions[validAttachmentCount].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
			attachmentDescriptions[validAttachmentCount].format = _ConvertFormat(texdesc.Format);
			attachmentDescriptions[validAttachmentCount].samples = (VkSampleCountFlagBits)texdesc.SampleCount;

			switch (attachment.loadop)
			{
			default:
			case RenderPassAttachment::LOADOP_LOAD:
				attachmentDescriptions[validAttachmentCount].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
				break;
			case RenderPassAttachment::LOADOP_CLEAR:
				attachmentDescriptions[validAttachmentCount].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				break;
			case RenderPassAttachment::LOADOP_DONTCARE:
				attachmentDescriptions[validAttachmentCount].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				break;
			}

			switch (attachment.storeop)
			{
			default:
			case RenderPassAttachment::STOREOP_STORE:
				attachmentDescriptions[validAttachmentCount].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				break;
			case RenderPassAttachment::STOREOP_DONTCARE:
				attachmentDescriptions[validAttachmentCount].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				break;
			}

			attachmentDescriptions[validAttachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDescriptions[validAttachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			attachmentDescriptions[validAttachmentCount].initialLayout = _ConvertImageLayout(attachment.initial_layout);
			attachmentDescriptions[validAttachmentCount].finalLayout = _ConvertImageLayout(attachment.final_layout);

			if (attachment.type == RenderPassAttachment::RENDERTARGET)
			{
				if (subresource < 0 || texture_internal_state->subresources_rtv.empty())
				{
					attachments[validAttachmentCount] = texture_internal_state->rtv;
				}
				else
				{
					assert(texture_internal_state->subresources_rtv.size() > size_t(subresource) && "Invalid RTV subresource!");
					attachments[validAttachmentCount] = texture_internal_state->subresources_rtv[subresource];
				}
				if (attachments[validAttachmentCount] == VK_NULL_HANDLE)
				{
					continue;
				}

				colorAttachmentRefs[subpass.colorAttachmentCount].sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
				colorAttachmentRefs[subpass.colorAttachmentCount].attachment = validAttachmentCount;
				colorAttachmentRefs[subpass.colorAttachmentCount].layout = _ConvertImageLayout(attachment.subpass_layout);
				colorAttachmentRefs[subpass.colorAttachmentCount].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				subpass.colorAttachmentCount++;
				subpass.pColorAttachments = colorAttachmentRefs;
			}
			else if (attachment.type == RenderPassAttachment::DEPTH_STENCIL)
			{
				if (subresource < 0 || texture_internal_state->subresources_dsv.empty())
				{
					attachments[validAttachmentCount] = texture_internal_state->dsv;
				}
				else
				{
					assert(texture_internal_state->subresources_dsv.size() > size_t(subresource) && "Invalid DSV subresource!");
					attachments[validAttachmentCount] = texture_internal_state->subresources_dsv[subresource];
				}
				if (attachments[validAttachmentCount] == VK_NULL_HANDLE)
				{
					continue;
				}

				depthAttachmentRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
				depthAttachmentRef.attachment = validAttachmentCount;
				depthAttachmentRef.layout = _ConvertImageLayout(attachment.subpass_layout);
				depthAttachmentRef.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				subpass.pDepthStencilAttachment = &depthAttachmentRef;

				if (IsFormatStencilSupport(texdesc.Format))
				{
					depthAttachmentRef.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					switch (attachment.loadop)
					{
					default:
					case RenderPassAttachment::LOADOP_LOAD:
						attachmentDescriptions[validAttachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
						break;
					case RenderPassAttachment::LOADOP_CLEAR:
						attachmentDescriptions[validAttachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
						break;
					case RenderPassAttachment::LOADOP_DONTCARE:
						attachmentDescriptions[validAttachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
						break;
					}

					switch (attachment.storeop)
					{
					default:
					case RenderPassAttachment::STOREOP_STORE:
						attachmentDescriptions[validAttachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
						break;
					case RenderPassAttachment::STOREOP_DONTCARE:
						attachmentDescriptions[validAttachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
						break;
					}
				}
			}
			else if (attachment.type == RenderPassAttachment::RESOLVE)
			{
				resolveAttachmentRefs[resolvecount].sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;

				if (attachment.texture == nullptr)
				{
					resolveAttachmentRefs[resolvecount].attachment = VK_ATTACHMENT_UNUSED;
				}
				else
				{
					if (subresource < 0 || texture_internal_state->subresources_srv.empty())
					{
						attachments[validAttachmentCount] = texture_internal_state->srv;
					}
					else
					{
						assert(texture_internal_state->subresources_srv.size() > size_t(subresource) && "Invalid SRV subresource!");
						attachments[validAttachmentCount] = texture_internal_state->subresources_srv[subresource];
					}
					if (attachments[validAttachmentCount] == VK_NULL_HANDLE)
					{
						continue;
					}
					resolveAttachmentRefs[resolvecount].attachment = validAttachmentCount;
					resolveAttachmentRefs[resolvecount].layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					resolveAttachmentRefs[resolvecount].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}

				resolvecount++;
				subpass.pResolveAttachments = resolveAttachmentRefs;
			}
			else if (attachment.type == RenderPassAttachment::SHADING_RATE_SOURCE && CheckCapability(GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING_TIER2))
			{
				shadingRateAttachmentRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;

				if (attachment.texture == nullptr)
				{
					shadingRateAttachmentRef.attachment = VK_ATTACHMENT_UNUSED;
				}
				else
				{
					if (subresource < 0 || texture_internal_state->subresources_uav.empty())
					{
						attachments[validAttachmentCount] = texture_internal_state->uav;
					}
					else
					{
						assert(texture_internal_state->subresources_uav.size() > size_t(subresource) && "Invalid UAV subresource!");
						attachments[validAttachmentCount] = texture_internal_state->subresources_uav[subresource];
					}
					if (attachments[validAttachmentCount] == VK_NULL_HANDLE)
					{
						continue;
					}
					shadingRateAttachmentRef.attachment = validAttachmentCount;
					shadingRateAttachmentRef.layout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
					shadingRateAttachmentRef.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}

				subpass.pNext = &shading_rate_attachment;
			}

			validAttachmentCount++;
		}
		assert(renderpass->desc.attachments.size() == validAttachmentCount);

		VkRenderPassCreateInfo2 renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
		renderPassInfo.attachmentCount = validAttachmentCount;
		renderPassInfo.pAttachments = attachmentDescriptions;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		res = vkCreateRenderPass2(device, &renderPassInfo, nullptr, &internal_state->renderpass);
		assert(res == VK_SUCCESS);

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = internal_state->renderpass;
		framebufferInfo.attachmentCount = validAttachmentCount;

		if (validAttachmentCount > 0)
		{
			const TextureDesc& texdesc = desc.attachments[0].texture->desc;
			auto texture_internal = to_internal(desc.attachments[0].texture);
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = texdesc.Width;
			framebufferInfo.height = texdesc.Height;
			if (desc.attachments[0].subresource >= 0)
			{
				framebufferInfo.layers = texture_internal->subresources_framebuffer_layercount[0];
			}
			else
			{
				framebufferInfo.layers = texture_internal->framebuffer_layercount;
			}
			framebufferInfo.layers = std::min(framebufferInfo.layers, texdesc.ArraySize);
		}
		else
		{
			framebufferInfo.pAttachments = nullptr;
			framebufferInfo.width = properties2.properties.limits.maxFramebufferWidth;
			framebufferInfo.height = properties2.properties.limits.maxFramebufferHeight;
			framebufferInfo.layers = properties2.properties.limits.maxFramebufferLayers;
		}

		res = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &internal_state->framebuffer);
		assert(res == VK_SUCCESS);


		internal_state->beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		internal_state->beginInfo.renderPass = internal_state->renderpass;
		internal_state->beginInfo.framebuffer = internal_state->framebuffer;
		internal_state->beginInfo.renderArea.offset = { 0, 0 };
		internal_state->beginInfo.renderArea.extent.width = framebufferInfo.width;
		internal_state->beginInfo.renderArea.extent.height = framebufferInfo.height;

		if (validAttachmentCount > 0)
		{
			const TextureDesc& texdesc = desc.attachments[0].texture->desc;

			internal_state->beginInfo.clearValueCount = validAttachmentCount;
			internal_state->beginInfo.pClearValues = internal_state->clearColors;

			int i = 0;
			for (auto& attachment : desc.attachments)
			{
				if (desc.attachments[i].type == RenderPassAttachment::RESOLVE ||
					desc.attachments[i].type == RenderPassAttachment::SHADING_RATE_SOURCE ||
					attachment.texture == nullptr)
					continue;

				const ClearValue& clear = desc.attachments[i].texture->desc.clear;
				if (desc.attachments[i].type == RenderPassAttachment::RENDERTARGET)
				{
					internal_state->clearColors[i].color.float32[0] = clear.color[0];
					internal_state->clearColors[i].color.float32[1] = clear.color[1];
					internal_state->clearColors[i].color.float32[2] = clear.color[2];
					internal_state->clearColors[i].color.float32[3] = clear.color[3];
				}
				else if (desc.attachments[i].type == RenderPassAttachment::DEPTH_STENCIL)
				{
					internal_state->clearColors[i].depthStencil.depth = clear.depthstencil.depth;
					internal_state->clearColors[i].depthStencil.stencil = clear.depthstencil.stencil;
				}
				else
				{
					assert(0);
				}
				i++;
			}
		}

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* pDesc, RaytracingAccelerationStructure* bvh) const
	{
		auto internal_state = std::make_shared<BVH_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		bvh->internal_state = internal_state;
		bvh->type = GPUResource::GPU_RESOURCE_TYPE::RAYTRACING_ACCELERATION_STRUCTURE;

		bvh->desc = *pDesc;

		internal_state->buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		internal_state->buildInfo.flags = 0;
		if (bvh->desc._flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE)
		{
			internal_state->buildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		}
		if (bvh->desc._flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_COMPACTION)
		{
			internal_state->buildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
		}
		if (bvh->desc._flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_TRACE)
		{
			internal_state->buildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		}
		if (bvh->desc._flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD)
		{
			internal_state->buildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
		}
		if (bvh->desc._flags & RaytracingAccelerationStructureDesc::FLAG_MINIMIZE_MEMORY)
		{
			internal_state->buildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;
		}

		switch (pDesc->type)
		{
		case RaytracingAccelerationStructureDesc::BOTTOMLEVEL:
		{
			internal_state->buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

			for (auto& x : pDesc->bottomlevel.geometries)
			{
				internal_state->geometries.emplace_back();
				auto& geometry = internal_state->geometries.back();
				geometry = {};
				geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;

				internal_state->primitiveCounts.emplace_back();
				uint32_t& primitiveCount = internal_state->primitiveCounts.back();

				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::TRIANGLES)
				{
					geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
					geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
					geometry.geometry.triangles.indexType = x.triangles.indexFormat == INDEXFORMAT_16BIT ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
					geometry.geometry.triangles.maxVertex = x.triangles.vertexCount;
					geometry.geometry.triangles.vertexStride = x.triangles.vertexStride;
					geometry.geometry.triangles.vertexFormat = _ConvertFormat(x.triangles.vertexFormat);

					primitiveCount = x.triangles.indexCount / 3;
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::PROCEDURAL_AABBS)
				{
					geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
					geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
					geometry.geometry.aabbs.stride = sizeof(float) * 6; // min - max corners

					primitiveCount = x.aabbs.count;
				}
			}


		}
		break;
		case RaytracingAccelerationStructureDesc::TOPLEVEL:
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
			primitiveCount = pDesc->toplevel.count;
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
		allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

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


		if (pDesc->type == RaytracingAccelerationStructureDesc::TOPLEVEL)
		{
			int index = allocationhandler->bindlessAccelerationStructures.allocate();
			if (index >= 0)
			{
				VkWriteDescriptorSetAccelerationStructureKHR acc = {};
				acc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
				acc.accelerationStructureCount = 1;
				acc.pAccelerationStructures = &internal_state->resource;

				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
				write.dstBinding = 0;
				write.dstArrayElement = index;
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

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* pDesc, RaytracingPipelineState* rtpso) const
	{
		auto internal_state = std::make_shared<RTPipelineState_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		rtpso->internal_state = internal_state;

		rtpso->desc = *pDesc;

		VkRayTracingPipelineCreateInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		info.flags = 0;

		std::vector<VkPipelineShaderStageCreateInfo> stages;
		for (auto& x : pDesc->shaderlibraries)
		{
			stages.emplace_back();
			auto& stage = stages.back();
			stage = {};
			stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage.module = to_internal(x.shader)->shaderModule;
			switch (x.type)
			{
			default:
			case ShaderLibrary::RAYGENERATION:
				stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
				break;
			case ShaderLibrary::MISS:
				stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
				break;
			case ShaderLibrary::CLOSESTHIT:
				stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
				break;
			case ShaderLibrary::ANYHIT:
				stage.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
				break;
			case ShaderLibrary::INTERSECTION:
				stage.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
				break;
			}
			stage.pName = x.function_name.c_str();
		}
		info.stageCount = (uint32_t)stages.size();
		info.pStages = stages.data();

		std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;
		groups.reserve(pDesc->hitgroups.size());
		for (auto& x : pDesc->hitgroups)
		{
			groups.emplace_back();
			auto& group = groups.back();
			group = {};
			group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			switch (x.type)
			{
			default:
			case ShaderHitGroup::GENERAL:
				group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
				break;
			case ShaderHitGroup::TRIANGLES:
				group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
				break;
			case ShaderHitGroup::PROCEDURAL:
				group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
				break;
			}
			group.generalShader = x.general_shader;
			group.closestHitShader = x.closesthit_shader;
			group.anyHitShader = x.anyhit_shader;
			group.intersectionShader = x.intersection_shader;
		}
		info.groupCount = (uint32_t)groups.size();
		info.pGroups = groups.data();

		info.maxPipelineRayRecursionDepth = pDesc->max_trace_recursion_depth;

		info.layout = to_internal(pDesc->shaderlibraries.front().shader)->pipelineLayout_cs; // think better way

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
			VK_NULL_HANDLE,
			1,
			&info,
			nullptr,
			&internal_state->pipeline
		);
		assert(res == VK_SUCCESS);

		return res == VK_SUCCESS;
	}
	
	int GraphicsDevice_Vulkan::CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount) const
	{
		auto internal_state = to_internal(texture);

		VkImageViewCreateInfo view_desc = {};
		view_desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_desc.flags = 0;
		view_desc.image = internal_state->resource;
		view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_desc.subresourceRange.baseArrayLayer = firstSlice;
		view_desc.subresourceRange.layerCount = sliceCount;
		view_desc.subresourceRange.baseMipLevel = firstMip;
		view_desc.subresourceRange.levelCount = mipCount;
		view_desc.format = _ConvertFormat(texture->desc.Format);

		if (texture->desc.type == TextureDesc::TEXTURE_1D)
		{
			if (texture->desc.ArraySize > 1)
			{
				view_desc.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
			}
			else
			{
				view_desc.viewType = VK_IMAGE_VIEW_TYPE_1D;
			}
		}
		else if (texture->desc.type == TextureDesc::TEXTURE_2D)
		{
			if (texture->desc.ArraySize > 1)
			{
				if (texture->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
				{
					if (texture->desc.ArraySize > 6 && sliceCount > 6)
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
		else if (texture->desc.type == TextureDesc::TEXTURE_3D)
		{
			view_desc.viewType = VK_IMAGE_VIEW_TYPE_3D;
		}

		switch (type)
		{
		case wiGraphics::SRV:
		{
			switch (texture->desc.Format)
			{
			case FORMAT_R16_TYPELESS:
				view_desc.format = VK_FORMAT_D16_UNORM;
				view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				break;
			case FORMAT_R32_TYPELESS:
				view_desc.format = VK_FORMAT_D32_SFLOAT;
				view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				break;
			case FORMAT_R24G8_TYPELESS:
				view_desc.format = VK_FORMAT_D24_UNORM_S8_UINT;
				view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				break;
			case FORMAT_R32G8X24_TYPELESS:
				view_desc.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
				view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				break;
			}

			VkImageView srv;
			VkResult res = vkCreateImageView(device, &view_desc, nullptr, &srv);

			int index = allocationhandler->bindlessSampledImages.allocate();
			if (index >= 0)
			{
				VkDescriptorImageInfo imageInfo = {};
				imageInfo.imageView = srv;
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				write.dstBinding = 0;
				write.dstArrayElement = index;
				write.descriptorCount = 1;
				write.dstSet = allocationhandler->bindlessSampledImages.descriptorSet;
				write.pImageInfo = &imageInfo;
				vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
			}

			if (res == VK_SUCCESS)
			{
				if (internal_state->srv == VK_NULL_HANDLE)
				{
					internal_state->srv = srv;
					internal_state->srv_index = index;
					return -1;
				}
				internal_state->subresources_srv.push_back(srv);
				internal_state->subresources_srv_index.push_back(index);
				return int(internal_state->subresources_srv.size() - 1);
			}
			else
			{
				assert(0);
			}
		}
		break;
		case wiGraphics::UAV:
		{
			if (view_desc.viewType == VK_IMAGE_VIEW_TYPE_CUBE || view_desc.viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
			{
				view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			}

			VkImageView uav;
			VkResult res = vkCreateImageView(device, &view_desc, nullptr, &uav);

			int index = allocationhandler->bindlessStorageImages.allocate();
			if (index >= 0)
			{
				VkDescriptorImageInfo imageInfo = {};
				imageInfo.imageView = uav;
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				write.dstBinding = 0;
				write.dstArrayElement = index;
				write.descriptorCount = 1;
				write.dstSet = allocationhandler->bindlessStorageImages.descriptorSet;
				write.pImageInfo = &imageInfo;
				vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
			}

			if (res == VK_SUCCESS)
			{
				if (internal_state->uav == VK_NULL_HANDLE)
				{
					internal_state->uav = uav;
					internal_state->uav_index = index;
					return -1;
				}
				internal_state->subresources_uav.push_back(uav);
				internal_state->subresources_uav_index.push_back(index);
				return int(internal_state->subresources_uav.size() - 1);
			}
			else
			{
				assert(0);
			}
		}
		break;
		case wiGraphics::RTV:
		{
			VkImageView rtv;
			view_desc.subresourceRange.levelCount = 1;
			VkResult res = vkCreateImageView(device, &view_desc, nullptr, &rtv);

			if (res == VK_SUCCESS)
			{
				if (internal_state->rtv == VK_NULL_HANDLE)
				{
					internal_state->rtv = rtv;
					internal_state->framebuffer_layercount = view_desc.subresourceRange.layerCount;
					return -1;
				}
				internal_state->subresources_rtv.push_back(rtv);
				internal_state->subresources_framebuffer_layercount.push_back(view_desc.subresourceRange.layerCount);
				return int(internal_state->subresources_rtv.size() - 1);
			}
			else
			{
				assert(0);
			}
		}
		break;
		case wiGraphics::DSV:
		{
			view_desc.subresourceRange.levelCount = 1;
			view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			switch (texture->desc.Format)
			{
			case FORMAT_R16_TYPELESS:
				view_desc.format = VK_FORMAT_D16_UNORM;
				break;
			case FORMAT_R32_TYPELESS:
				view_desc.format = VK_FORMAT_D32_SFLOAT;
				break;
			case FORMAT_R24G8_TYPELESS:
				view_desc.format = VK_FORMAT_D24_UNORM_S8_UINT;
				view_desc.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
				break;
			case FORMAT_R32G8X24_TYPELESS:
				view_desc.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
				view_desc.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
				break;
			}

			VkImageView dsv;
			VkResult res = vkCreateImageView(device, &view_desc, nullptr, &dsv);

			if (res == VK_SUCCESS)
			{
				if (internal_state->dsv == VK_NULL_HANDLE)
				{
					internal_state->dsv = dsv;
					internal_state->framebuffer_layercount = view_desc.subresourceRange.layerCount;
					return -1;
				}
				internal_state->subresources_dsv.push_back(dsv);
				internal_state->subresources_framebuffer_layercount.push_back(view_desc.subresourceRange.layerCount);
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
	int GraphicsDevice_Vulkan::CreateSubresource(GPUBuffer* buffer, SUBRESOURCE_TYPE type, uint64_t offset, uint64_t size) const
	{
		auto internal_state = to_internal(buffer);
		const GPUBufferDesc& desc = buffer->GetDesc();
		VkResult res;

		switch (type)
		{
		case wiGraphics::CBV:
		{
			int index = allocationhandler->bindlessUniformBuffers.allocate();
			if (index >= 0)
			{
				VkDescriptorBufferInfo bufferInfo = {};
				bufferInfo.buffer = internal_state->resource;
				bufferInfo.offset = offset;
				bufferInfo.range = size;
				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				write.dstBinding = 0;
				write.dstArrayElement = index;
				write.descriptorCount = 1;
				write.dstSet = allocationhandler->bindlessUniformBuffers.descriptorSet;
				write.pBufferInfo = &bufferInfo;
				vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
				internal_state->cbv_index = index;
			}
			return -1;
		}
		break;

		case wiGraphics::SRV:
		case wiGraphics::UAV:
		{
			if (desc.Format == FORMAT_UNKNOWN)
			{
				// Raw buffer
				int index = allocationhandler->bindlessStorageBuffers.allocate();
				if (index >= 0)
				{
					VkDescriptorBufferInfo bufferInfo = {};
					bufferInfo.buffer = internal_state->resource;
					bufferInfo.offset = offset;
					bufferInfo.range = size;
					VkWriteDescriptorSet write = {};
					write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					write.dstBinding = 0;
					write.dstArrayElement = index;
					write.descriptorCount = 1;
					write.dstSet = allocationhandler->bindlessStorageBuffers.descriptorSet;
					write.pBufferInfo = &bufferInfo;
					vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
				}

				if (type == SRV)
				{
					if (internal_state->srv_index == -1)
					{
						internal_state->srv_index = index;
						return -1;
					}
					internal_state->subresources_srv_index.push_back(index);
					return int(internal_state->subresources_srv_index.size() - 1);
				}
				else
				{
					if (internal_state->uav_index == -1)
					{
						internal_state->uav_index = index;
						return -1;
					}
					internal_state->subresources_uav_index.push_back(index);
					return int(internal_state->subresources_uav_index.size() - 1);
				}
			}
			else
			{
				// Typed buffer

				VkBufferViewCreateInfo srv_desc = {};
				srv_desc.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
				srv_desc.buffer = internal_state->resource;
				srv_desc.flags = 0;
				srv_desc.format = _ConvertFormat(desc.Format);
				srv_desc.offset = Align(offset, properties2.properties.limits.minTexelBufferOffsetAlignment); // damn, if this needs alignment, that could break a lot of things! (index buffer, index offset?)
				srv_desc.range = std::min(size, (uint64_t)desc.ByteWidth - srv_desc.offset);

				VkBufferView view;
				res = vkCreateBufferView(device, &srv_desc, nullptr, &view);

				if (res == VK_SUCCESS)
				{
					if (type == SRV)
					{
						int index = allocationhandler->bindlessUniformTexelBuffers.allocate();
						if (index >= 0)
						{
							VkWriteDescriptorSet write = {};
							write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
							write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
							write.dstBinding = 0;
							write.dstArrayElement = index;
							write.descriptorCount = 1;
							write.dstSet = allocationhandler->bindlessUniformTexelBuffers.descriptorSet;
							write.pTexelBufferView = &view;
							vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
						}

						if (internal_state->srv == VK_NULL_HANDLE)
						{
							internal_state->srv = view;
							internal_state->srv_index = index;
							return -1;
						}
						internal_state->subresources_srv.push_back(view);
						internal_state->subresources_srv_index.push_back(index);
						return int(internal_state->subresources_srv.size() - 1);
					}
					else
					{
						int index = allocationhandler->bindlessStorageTexelBuffers.allocate();
						if (index >= 0)
						{
							VkWriteDescriptorSet write = {};
							write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
							write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
							write.dstBinding = 0;
							write.dstArrayElement = index;
							write.descriptorCount = 1;
							write.dstSet = allocationhandler->bindlessStorageTexelBuffers.descriptorSet;
							write.pTexelBufferView = &view;
							vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
						}

						if (internal_state->uav == VK_NULL_HANDLE)
						{
							internal_state->uav = view;
							internal_state->uav_index = index;
							return -1;
						}
						internal_state->subresources_uav.push_back(view);
						internal_state->subresources_uav_index.push_back(index);
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

	int GraphicsDevice_Vulkan::GetDescriptorIndex(const GPUResource* resource, SUBRESOURCE_TYPE type, int subresource) const
	{
		if (resource == nullptr || !resource->IsValid())
			return -1;

		switch (type)
		{
		default:
		case wiGraphics::CBV:
			if (resource->IsBuffer())
			{
				auto internal_state = to_internal((const GPUBuffer*)resource);
				return internal_state->cbv_index;
			}
			break;
		case wiGraphics::SRV:
			if (resource->IsBuffer())
			{
				auto internal_state = to_internal((const GPUBuffer*)resource);
				if (subresource < 0)
				{
					return internal_state->srv_index;
				}
				else
				{
					return internal_state->subresources_srv_index[subresource];
				}
			}
			else if(resource->IsTexture())
			{
				auto internal_state = to_internal((const Texture*)resource);
				if (subresource < 0)
				{
					return internal_state->srv_index;
				}
				else
				{
					return internal_state->subresources_srv_index[subresource];
				}
			}
			else if (resource->IsAccelerationStructure())
			{
				auto internal_state = to_internal((const RaytracingAccelerationStructure*)resource);
				return internal_state->index;
			}
			break;
		case wiGraphics::UAV:
			if (resource->IsBuffer())
			{
				auto internal_state = to_internal((const GPUBuffer*)resource);
				if (subresource < 0)
				{
					return internal_state->uav_index;
				}
				else
				{
					return internal_state->subresources_uav_index[subresource];
				}
			}
			else if (resource->IsTexture())
			{
				auto internal_state = to_internal((const Texture*)resource);
				if (subresource < 0)
				{
					return internal_state->uav_index;
				}
				else
				{
					return internal_state->subresources_uav_index[subresource];
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

	void GraphicsDevice_Vulkan::WriteShadingRateValue(SHADING_RATE rate, void* dest) const
	{
		// How to compute shading rate value texel data:
		// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#primsrast-fragment-shading-rate-attachment

		switch (rate)
		{
		default:
		case wiGraphics::SHADING_RATE_1X1:
			*(uint8_t*)dest = 0;
			break;
		case wiGraphics::SHADING_RATE_1X2:
			*(uint8_t*)dest = 0x1;
			break;
		case wiGraphics::SHADING_RATE_2X1:
			*(uint8_t*)dest = 0x4;
			break;
		case wiGraphics::SHADING_RATE_2X2:
			*(uint8_t*)dest = 0x5;
			break;
		case wiGraphics::SHADING_RATE_2X4:
			*(uint8_t*)dest = 0x6;
			break;
		case wiGraphics::SHADING_RATE_4X2:
			*(uint8_t*)dest = 0x9;
			break;
		case wiGraphics::SHADING_RATE_4X4:
			*(uint8_t*)dest = 0xa;
			break;
		}

	}
	void GraphicsDevice_Vulkan::WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) const
	{
		VkAccelerationStructureInstanceKHR* desc = (VkAccelerationStructureInstanceKHR*)dest;
		memcpy(&desc->transform, &instance->transform, sizeof(desc->transform));
		desc->instanceCustomIndex = instance->InstanceID;
		desc->mask = instance->InstanceMask;
		desc->instanceShaderBindingTableRecordOffset = instance->InstanceContributionToHitGroupIndex;
		desc->flags = instance->Flags;

		assert(instance->bottomlevel.IsAccelerationStructure());
		auto internal_state = to_internal((RaytracingAccelerationStructure*)&instance->bottomlevel);
		desc->accelerationStructureReference = internal_state->as_address;
	}
	void GraphicsDevice_Vulkan::WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const
	{
		VkResult res = vkGetRayTracingShaderGroupHandlesKHR(device, to_internal(rtpso)->pipeline, group_index, 1, SHADER_IDENTIFIER_SIZE, dest);
		assert(res == VK_SUCCESS);
	}
	
	void GraphicsDevice_Vulkan::Map(const GPUResource* resource, Mapping* mapping) const
	{
		VkDeviceMemory memory = VK_NULL_HANDLE;

		if (resource->type == GPUResource::GPU_RESOURCE_TYPE::BUFFER)
		{
			const GPUBuffer* buffer = (const GPUBuffer*)resource;
			auto internal_state = to_internal(buffer);
			memory = internal_state->allocation->GetMemory();
			mapping->rowpitch = (uint32_t)buffer->desc.ByteWidth;
		}
		else if (resource->type == GPUResource::GPU_RESOURCE_TYPE::TEXTURE)
		{
			const Texture* texture = (const Texture*)resource;
			auto internal_state = to_internal(texture);
			memory = internal_state->allocation->GetMemory();
			mapping->rowpitch = (uint32_t)internal_state->subresourcelayout.rowPitch;
		}
		else
		{
			assert(0);
			return;
		}

		VkDeviceSize offset = mapping->offset;
		VkDeviceSize size = mapping->size;

		VkResult res = vkMapMemory(device, memory, offset, size, 0, &mapping->data);
		if (res != VK_SUCCESS)
		{
			assert(0);
			mapping->data = nullptr;
			mapping->rowpitch = 0;
		}
	}
	void GraphicsDevice_Vulkan::Unmap(const GPUResource* resource) const
	{
		if (resource->type == GPUResource::GPU_RESOURCE_TYPE::BUFFER)
		{
			const GPUBuffer* buffer = (const GPUBuffer*)resource;
			auto internal_state = to_internal(buffer);
			vkUnmapMemory(device, internal_state->allocation->GetMemory());
		}
		else if (resource->type == GPUResource::GPU_RESOURCE_TYPE::TEXTURE)
		{
			const Texture* texture = (const Texture*)resource;
			auto internal_state = to_internal(texture);
			vkUnmapMemory(device, internal_state->allocation->GetMemory());
		}
	}
	void GraphicsDevice_Vulkan::QueryRead(const GPUQueryHeap* heap, uint32_t index, uint32_t count, uint64_t* results) const
	{
		if (count == 0)
			return;

		auto internal_state = to_internal(heap);

		VkResult res = vkGetQueryPoolResults(
			device,
			internal_state->pool,
			index,
			count,
			sizeof(uint64_t) * count,
			results,
			sizeof(uint64_t),
			VK_QUERY_RESULT_64_BIT
		);

		vkResetQueryPool(
			device,
			internal_state->pool,
			index,
			count
		);
	}

	void GraphicsDevice_Vulkan::SetCommonSampler(const StaticSampler* sam)
	{
		common_samplers.push_back(*sam);
	}

	void GraphicsDevice_Vulkan::SetName(GPUResource* pResource, const char* name)
	{
		if (vkSetDebugUtilsObjectNameEXT != nullptr)
		{
			VkDebugUtilsObjectNameInfoEXT info = {};
			info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
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

			if (info.objectHandle == VK_NULL_HANDLE)
			{
				return;
			}

			VkResult res = vkSetDebugUtilsObjectNameEXT(device, &info);
			assert(res == VK_SUCCESS);
		}
	}

	void GraphicsDevice_Vulkan::PresentBegin(CommandList cmd)
	{
		VkResult res = vkAcquireNextImageKHR(
			device, swapChain,
			0xFFFFFFFFFFFFFFFF,
			GetFrameResources().swapchainAcquireSemaphore,
			VK_NULL_HANDLE,
			&swapChainImageIndex
		);
		if (res != VK_SUCCESS)
		{
			// Handle outdated error in acquire.
			if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
			{
				CreateBackBufferResources();
				PresentBegin(cmd);
				return;
			}
		}
		barrier_flush(cmd);

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = defaultRenderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[swapChainImageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		vkCmdBeginRenderPass(GetDirectCommandList(cmd), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	}
	void GraphicsDevice_Vulkan::PresentEnd(CommandList cmd)
	{
		vkCmdEndRenderPass(GetDirectCommandList(cmd));

		SubmitCommandLists();

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &frames[swapChainImageIndex].swapchainReleaseSemaphore;

		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain;
		presentInfo.pImageIndices = &swapChainImageIndex;

		VkResult res = vkQueuePresentKHR(presentQueue, &presentInfo);
		assert(res == VK_SUCCESS);

#if 0
		VmaStats stats = {};
		vmaCalculateStats(allocationhandler->allocator, &stats);
		std::cout << "VMA Stats: " << stats.total.usedBytes;
#endif
	}

	CommandList GraphicsDevice_Vulkan::BeginCommandList()
	{
		VkResult res;

		CommandList cmd = cmd_count.fetch_add(1);
		if (GetDirectCommandList(cmd) == VK_NULL_HANDLE)
		{
			// need to create one more command list:
			assert(cmd < COMMANDLIST_COUNT);

			for (auto& frame : frames)
			{
				VkCommandPoolCreateInfo poolInfo = {};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.queueFamilyIndex = graphicsFamily;
				poolInfo.flags = 0; // Optional

				res = vkCreateCommandPool(device, &poolInfo, nullptr, &frame.commandPools[cmd]);
				assert(res == VK_SUCCESS);

				VkCommandBufferAllocateInfo commandBufferInfo = {};
				commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				commandBufferInfo.commandBufferCount = 1;
				commandBufferInfo.commandPool = frame.commandPools[cmd];
				commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

				res = vkAllocateCommandBuffers(device, &commandBufferInfo, &frame.commandBuffers[cmd]);
				assert(res == VK_SUCCESS);

				frame.resourceBuffer[cmd].init(this, 1024 * 1024); // 1 MB starting size
				frame.descriptors[cmd].init(this);
			}
		}

		if (!stashed[cmd])
		{
			res = vkResetCommandPool(device, GetFrameResources().commandPools[cmd], 0);
			assert(res == VK_SUCCESS);

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional

			res = vkBeginCommandBuffer(GetFrameResources().commandBuffers[cmd], &beginInfo);
			assert(res == VK_SUCCESS);

			// reset descriptor allocators:
			GetFrameResources().descriptors[cmd].reset();

			// reset immediate resource allocators:
			GetFrameResources().resourceBuffer[cmd].clear();
		}

		Viewport viewports[6];
		for (uint32_t i = 0; i < arraysize(viewports); ++i)
		{
			viewports[i].TopLeftX = 0;
			viewports[i].TopLeftY = 0;
			viewports[i].Width = (float)RESOLUTIONWIDTH;
			viewports[i].Height = (float)RESOLUTIONHEIGHT;
			viewports[i].MinDepth = 0;
			viewports[i].MaxDepth = 1;
		}
		BindViewports(arraysize(viewports), viewports, cmd);

		VkRect2D scissors[8];
		for (int i = 0; i < arraysize(scissors); ++i)
		{
			scissors[i].offset.x = 0;
			scissors[i].offset.y = 0;
			scissors[i].extent.width = 65535;
			scissors[i].extent.height = 65535;
		}
		vkCmdSetScissor(GetDirectCommandList(cmd), 0, arraysize(scissors), scissors);

		float blendConstants[] = { 1,1,1,1 };
		vkCmdSetBlendConstants(GetDirectCommandList(cmd), blendConstants);

		prev_pipeline_hash[cmd] = 0;
		active_pso[cmd] = nullptr;
		active_cs[cmd] = nullptr;
		active_rt[cmd] = nullptr;
		active_renderpass[cmd] = VK_NULL_HANDLE;
		dirty_pso[cmd] = false;
		prev_shadingrate[cmd] = SHADING_RATE_INVALID;
		pushconstants[cmd] = {};
		vb_hash[cmd] = 0;
		for (int i = 0; i < arraysize(vb_strides[cmd]); ++i)
		{
			vb_strides[cmd][i] = 0;
		}

		return cmd;
	}
	void GraphicsDevice_Vulkan::SubmitCommandLists()
	{
		auto& frame = GetFrameResources();

		// Sync up copy queue and transitions:
		copyQueueLock.lock();
		if (copyQueueUse)
		{
			// Copies:
			{
				VkResult res = vkEndCommandBuffer(frame.copyCommandBuffer);
				assert(res == VK_SUCCESS);

				VkSemaphore semaphores[] = { copySemaphore };

				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &frame.copyCommandBuffer;
				submitInfo.pSignalSemaphores = semaphores;
				submitInfo.signalSemaphoreCount = arraysize(semaphores);

				res = vkQueueSubmit(copyQueue, 1, &submitInfo, VK_NULL_HANDLE);
				assert(res == VK_SUCCESS);

			}
		}

		// Execute deferred command lists:
		{

			VkCommandBuffer cmdLists[COMMANDLIST_COUNT];
			uint32_t counter = 0;

			// Transitions:
			{
				vkCmdPipelineBarrier(
					frame.transitionCommandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					0,
					0, nullptr,
					0, nullptr,
					(uint32_t)frame.loadedimagetransitions.size(), frame.loadedimagetransitions.data()
				);
				frame.loadedimagetransitions.clear();

				VkResult res = vkEndCommandBuffer(frame.transitionCommandBuffer);
				assert(res == VK_SUCCESS);

				cmdLists[counter++] = frame.transitionCommandBuffer;
			}

			CommandList cmd_last = cmd_count.load();
			cmd_count.store(0);
			for (CommandList cmd = 0; cmd < cmd_last; ++cmd)
			{
				stashed[cmd] = false;
				barrier_flush(cmd);

				VkResult res = vkEndCommandBuffer(GetDirectCommandList(cmd));
				assert(res == VK_SUCCESS);

				cmdLists[counter++] = GetDirectCommandList(cmd);

				for (auto& x : pipelines_worker[cmd])
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
				pipelines_worker[cmd].clear();
			}

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkSemaphore waitSemaphores[] = { frame.swapchainAcquireSemaphore, copySemaphore };
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
			if (copyQueueUse)
			{
				submitInfo.waitSemaphoreCount = 2;
			}
			else
			{
				submitInfo.waitSemaphoreCount = 1;
			}
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.commandBufferCount = counter;
			submitInfo.pCommandBuffers = cmdLists;

			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &frame.swapchainReleaseSemaphore;

			VkResult res = vkQueueSubmit(graphicsQueue, 1, &submitInfo, frame.frameFence);
			assert(res == VK_SUCCESS);
		}

		// This acts as a barrier, following this we will be using the next frame's resources when calling GetFrameResources()!
		FRAMECOUNT++;

		// Initiate stalling CPU when GPU is behind by more frames than would fit in the backbuffers:
		if (FRAMECOUNT >= BACKBUFFER_COUNT)
		{
			VkResult res = vkWaitForFences(device, 1, &GetFrameResources().frameFence, true, 0xFFFFFFFFFFFFFFFF);
			assert(res == VK_SUCCESS);

			res = vkResetFences(device, 1, &GetFrameResources().frameFence);
			assert(res == VK_SUCCESS);
		}

		allocationhandler->Update(FRAMECOUNT, BACKBUFFER_COUNT);

		// Restart transition command buffers:
		{
			auto& frame = GetFrameResources();

			VkResult res = vkResetCommandPool(device, frame.transitionCommandPool, 0);
			assert(res == VK_SUCCESS);

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional

			res = vkBeginCommandBuffer(frame.transitionCommandBuffer, &beginInfo);
			assert(res == VK_SUCCESS);
		}

		copyQueueUse = false;
		copyQueueLock.unlock();
	}
	void GraphicsDevice_Vulkan::StashCommandLists()
	{
		CommandList active_count = cmd_count.load();
		cmd_count.store(0);
		for (CommandList cmd = 0; cmd < active_count; ++cmd)
		{
			stashed[cmd] = true;
		}
	}

	void GraphicsDevice_Vulkan::WaitForGPU()
	{
		VkResult res = vkQueueWaitIdle(graphicsQueue);
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

		for (int i = 0; i < arraysize(pipelines_worker); ++i)
		{
			for (auto& x : pipelines_worker[i])
			{
				allocationhandler->destroyer_pipelines.push_back(std::make_pair(x.second, FRAMECOUNT));
			}
			pipelines_worker[i].clear();
		}
		allocationhandler->destroylocker.unlock();
	}


	void GraphicsDevice_Vulkan::RenderPassBegin(const RenderPass* renderpass, CommandList cmd)
	{
		barrier_flush(cmd);

		active_renderpass[cmd] = renderpass;

		auto internal_state = to_internal(renderpass);
		vkCmdBeginRenderPass(GetDirectCommandList(cmd), &internal_state->beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}
	void GraphicsDevice_Vulkan::RenderPassEnd(CommandList cmd)
	{
		vkCmdEndRenderPass(GetDirectCommandList(cmd));

		active_renderpass[cmd] = VK_NULL_HANDLE;
	}
	void GraphicsDevice_Vulkan::BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd) {
		assert(rects != nullptr);
		assert(numRects <= 8);
		VkRect2D scissors[8];
		for(uint32_t i = 0; i < numRects; ++i) {
			scissors[i].extent.width = abs(rects[i].right - rects[i].left);
			scissors[i].extent.height = abs(rects[i].top - rects[i].bottom);
			scissors[i].offset.x = std::max(0, rects[i].left);
			scissors[i].offset.y = std::max(0, rects[i].top);
		}
		vkCmdSetScissor(GetDirectCommandList(cmd), 0, numRects, scissors);
	}
	void GraphicsDevice_Vulkan::BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd)
	{
		assert(NumViewports <= 6);
		VkViewport viewports[6];
		for (uint32_t i = 0; i < NumViewports; ++i)
		{
			viewports[i].x = pViewports[i].TopLeftX;
			viewports[i].y = pViewports[i].TopLeftY + pViewports[i].Height;
			viewports[i].width = pViewports[i].Width;
			viewports[i].height = -pViewports[i].Height;
			viewports[i].minDepth = pViewports[i].MinDepth;
			viewports[i].maxDepth = pViewports[i].MaxDepth;
		}
		vkCmdSetViewport(GetDirectCommandList(cmd), 0, NumViewports, viewports);
	}
	void GraphicsDevice_Vulkan::BindResource(SHADERSTAGE stage, const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		assert(slot < GPU_RESOURCE_HEAP_SRV_COUNT);
		auto& descriptors = GetFrameResources().descriptors[cmd];
		if (descriptors.SRV[slot] != resource || descriptors.SRV_index[slot] != subresource)
		{
			descriptors.SRV[slot] = resource;
			descriptors.SRV_index[slot] = subresource;
			descriptors.dirty = true;
		}
	}
	void GraphicsDevice_Vulkan::BindResources(SHADERSTAGE stage, const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindResource(stage, resources[i], slot + i, cmd, -1);
			}
		}
	}
	void GraphicsDevice_Vulkan::BindUAV(SHADERSTAGE stage, const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		assert(slot < GPU_RESOURCE_HEAP_UAV_COUNT);
		auto& descriptors = GetFrameResources().descriptors[cmd];
		if (descriptors.UAV[slot] != resource || descriptors.UAV_index[slot] != subresource)
		{
			descriptors.UAV[slot] = resource;
			descriptors.UAV_index[slot] = subresource;
			descriptors.dirty = true;
		}
	}
	void GraphicsDevice_Vulkan::BindUAVs(SHADERSTAGE stage, const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindUAV(stage, resources[i], slot + i, cmd, -1);
			}
		}
	}
	void GraphicsDevice_Vulkan::UnbindResources(uint32_t slot, uint32_t num, CommandList cmd)
	{
	}
	void GraphicsDevice_Vulkan::UnbindUAVs(uint32_t slot, uint32_t num, CommandList cmd)
	{
	}
	void GraphicsDevice_Vulkan::BindSampler(SHADERSTAGE stage, const Sampler* sampler, uint32_t slot, CommandList cmd)
	{
		assert(slot < GPU_SAMPLER_HEAP_COUNT);
		auto& descriptors = GetFrameResources().descriptors[cmd];
		if (descriptors.SAM[slot] != sampler)
		{
			descriptors.SAM[slot] = sampler;
			descriptors.dirty = true;
		}
	}
	void GraphicsDevice_Vulkan::BindConstantBuffer(SHADERSTAGE stage, const GPUBuffer* buffer, uint32_t slot, CommandList cmd)
	{
		assert(slot < GPU_RESOURCE_HEAP_CBV_COUNT);
		auto& descriptors = GetFrameResources().descriptors[cmd];
		if (buffer->desc.Usage == USAGE_DYNAMIC || descriptors.CBV[slot] != buffer)
		{
			descriptors.CBV[slot] = buffer;
			descriptors.dirty = true;
		}
	}
	void GraphicsDevice_Vulkan::BindVertexBuffers(const GPUBuffer *const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint32_t* offsets, CommandList cmd)
	{
		size_t hash = 0;

		VkDeviceSize voffsets[8] = {};
		VkBuffer vbuffers[8] = {};
		assert(count <= 8);
		for (uint32_t i = 0; i < count; ++i)
		{
			wiHelper::hash_combine(hash, strides[i]);
			vb_strides[cmd][i] = strides[i];

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
					voffsets[i] = (VkDeviceSize)offsets[i];
				}
			}
		}
		for (int i = count; i < arraysize(vb_strides[cmd]); ++i)
		{
			vb_strides[cmd][i] = 0;
		}

		vkCmdBindVertexBuffers(GetDirectCommandList(cmd), static_cast<uint32_t>(slot), static_cast<uint32_t>(count), vbuffers, voffsets);

		if (hash != vb_hash[cmd])
		{
			vb_hash[cmd] = hash;
			dirty_pso[cmd] = true;
		}
	}
	void GraphicsDevice_Vulkan::BindIndexBuffer(const GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, uint32_t offset, CommandList cmd)
	{
		if (indexBuffer != nullptr)
		{
			auto internal_state = to_internal(indexBuffer);
			vkCmdBindIndexBuffer(GetDirectCommandList(cmd), internal_state->resource, (VkDeviceSize)offset, format == INDEXFORMAT_16BIT ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
		}
	}
	void GraphicsDevice_Vulkan::BindStencilRef(uint32_t value, CommandList cmd)
	{
		vkCmdSetStencilReference(GetDirectCommandList(cmd), VK_STENCIL_FRONT_AND_BACK, value);
	}
	void GraphicsDevice_Vulkan::BindBlendFactor(float r, float g, float b, float a, CommandList cmd)
	{
		float blendConstants[] = { r, g, b, a };
		vkCmdSetBlendConstants(GetDirectCommandList(cmd), blendConstants);
	}
	void GraphicsDevice_Vulkan::BindShadingRate(SHADING_RATE rate, CommandList cmd)
	{
		if (CheckCapability(GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING) && prev_shadingrate[cmd] != rate)
		{
			prev_shadingrate[cmd] = rate;

			VkExtent2D fragmentSize;
			switch (rate)
			{
			case wiGraphics::SHADING_RATE_1X1:
				fragmentSize.width = 1;
				fragmentSize.height = 1;
				break;
			case wiGraphics::SHADING_RATE_1X2:
				fragmentSize.width = 1;
				fragmentSize.height = 2;
				break;
			case wiGraphics::SHADING_RATE_2X1:
				fragmentSize.width = 2;
				fragmentSize.height = 1;
				break;
			case wiGraphics::SHADING_RATE_2X2:
				fragmentSize.width = 2;
				fragmentSize.height = 2;
				break;
			case wiGraphics::SHADING_RATE_2X4:
				fragmentSize.width = 2;
				fragmentSize.height = 4;
				break;
			case wiGraphics::SHADING_RATE_4X2:
				fragmentSize.width = 4;
				fragmentSize.height = 2;
				break;
			case wiGraphics::SHADING_RATE_4X4:
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
				GetDirectCommandList(cmd),
				&fragmentSize,
				combiner
			);
		}
	}
	void GraphicsDevice_Vulkan::BindPipelineState(const PipelineState* pso, CommandList cmd)
	{
		size_t pipeline_hash = 0;
		wiHelper::hash_combine(pipeline_hash, pso->hash);
		if (active_renderpass[cmd] != nullptr)
		{
			wiHelper::hash_combine(pipeline_hash, active_renderpass[cmd]->hash);
		}
		if (prev_pipeline_hash[cmd] == pipeline_hash)
		{
			return;
		}
		prev_pipeline_hash[cmd] = pipeline_hash;

		auto internal_state = to_internal(pso);

		if (active_pso[cmd] == nullptr)
		{
			GetFrameResources().descriptors[cmd].dirty = true;
		}
		else
		{
			auto active_internal = to_internal(active_pso[cmd]);
			if (internal_state->binding_hash != active_internal->binding_hash)
			{
				GetFrameResources().descriptors[cmd].dirty = true;
			}
		}

		if (!internal_state->bindlessSets.empty())
		{
			vkCmdBindDescriptorSets(
				GetDirectCommandList(cmd),
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				internal_state->pipelineLayout,
				internal_state->bindlessFirstSet,
				(uint32_t)internal_state->bindlessSets.size(),
				internal_state->bindlessSets.data(),
				0,
				nullptr
			);
		}

		active_pso[cmd] = pso;
		dirty_pso[cmd] = true;
	}
	void GraphicsDevice_Vulkan::BindComputeShader(const Shader* cs, CommandList cmd)
	{
		assert(cs->stage == CS || cs->stage == LIB);
		if (active_cs[cmd] != cs)
		{
			if (active_cs[cmd] == nullptr)
			{
				GetFrameResources().descriptors[cmd].dirty = true;
			}
			else
			{
				auto internal_state = to_internal(cs);
				auto active_internal = to_internal(active_cs[cmd]);
				if (internal_state->binding_hash != active_internal->binding_hash)
				{
					GetFrameResources().descriptors[cmd].dirty = true;
				}
			}

			active_cs[cmd] = cs;
			auto internal_state = to_internal(cs);

			if (cs->stage == CS)
			{
				vkCmdBindPipeline(GetDirectCommandList(cmd), VK_PIPELINE_BIND_POINT_COMPUTE, internal_state->pipeline_cs);

				if (!internal_state->bindlessSets.empty())
				{
					vkCmdBindDescriptorSets(
						GetDirectCommandList(cmd),
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
			else if (cs->stage == LIB)
			{
				if (!internal_state->bindlessSets.empty())
				{
					vkCmdBindDescriptorSets(
						GetDirectCommandList(cmd),
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
	}
	void GraphicsDevice_Vulkan::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		vkCmdDraw(GetDirectCommandList(cmd), static_cast<uint32_t>(vertexCount), 1, startVertexLocation, 0);
	}
	void GraphicsDevice_Vulkan::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		vkCmdDrawIndexed(GetDirectCommandList(cmd), static_cast<uint32_t>(indexCount), 1, startIndexLocation, baseVertexLocation, 0);
	}
	void GraphicsDevice_Vulkan::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		vkCmdDraw(GetDirectCommandList(cmd), static_cast<uint32_t>(vertexCount), static_cast<uint32_t>(instanceCount), startVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_Vulkan::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		vkCmdDrawIndexed(GetDirectCommandList(cmd), static_cast<uint32_t>(indexCount), static_cast<uint32_t>(instanceCount), startIndexLocation, baseVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_Vulkan::DrawInstancedIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		vkCmdDrawIndirect(GetDirectCommandList(cmd), internal_state->resource, (VkDeviceSize)args_offset, 1, (uint32_t)sizeof(IndirectDrawArgsInstanced));
	}
	void GraphicsDevice_Vulkan::DrawIndexedInstancedIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		vkCmdDrawIndexedIndirect(GetDirectCommandList(cmd), internal_state->resource, (VkDeviceSize)args_offset, 1, (uint32_t)sizeof(IndirectDrawArgsIndexedInstanced));
	}
	void GraphicsDevice_Vulkan::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predispatch(cmd);
		vkCmdDispatch(GetDirectCommandList(cmd), threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}
	void GraphicsDevice_Vulkan::DispatchIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
	{
		predispatch(cmd);
		auto internal_state = to_internal(args);
		vkCmdDispatchIndirect(GetDirectCommandList(cmd), internal_state->resource, (VkDeviceSize)args_offset);
	}
	void GraphicsDevice_Vulkan::DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predraw(cmd);
		vkCmdDrawMeshTasksNV(GetDirectCommandList(cmd), threadGroupCountX * threadGroupCountY * threadGroupCountZ, 0);
	}
	void GraphicsDevice_Vulkan::DispatchMeshIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		vkCmdDrawMeshTasksIndirectNV(GetDirectCommandList(cmd), internal_state->resource, (VkDeviceSize)args_offset,1,sizeof(IndirectDispatchArgs));
	}
	void GraphicsDevice_Vulkan::CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd)
	{
		barrier_flush(cmd);

		if (pDst->type == GPUResource::GPU_RESOURCE_TYPE::TEXTURE && pSrc->type == GPUResource::GPU_RESOURCE_TYPE::TEXTURE)
		{
			auto internal_state_src = to_internal((const Texture*)pSrc);
			auto internal_state_dst = to_internal((const Texture*)pDst);

			const TextureDesc& src_desc = ((const Texture*)pSrc)->GetDesc();
			const TextureDesc& dst_desc = ((const Texture*)pDst)->GetDesc();

			if (src_desc.Usage & USAGE_STAGING)
			{
				VkBufferImageCopy copy = {};
				copy.imageExtent.width = dst_desc.Width;
				copy.imageExtent.height = dst_desc.Height;
				copy.imageExtent.depth = 1;
				copy.imageExtent.width = dst_desc.Width;
				copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copy.imageSubresource.layerCount = 1;
				vkCmdCopyBufferToImage(
					GetDirectCommandList(cmd),
					internal_state_src->staging_resource,
					internal_state_dst->resource,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&copy
				);
			}
			else if (dst_desc.Usage & USAGE_STAGING)
			{
				VkBufferImageCopy copy = {};
				copy.imageExtent.width = src_desc.Width;
				copy.imageExtent.height = src_desc.Height;
				copy.imageExtent.depth = 1;
				copy.imageExtent.width = src_desc.Width;
				copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copy.imageSubresource.layerCount = 1;
				vkCmdCopyImageToBuffer(
					GetDirectCommandList(cmd),
					internal_state_src->resource,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					internal_state_dst->staging_resource,
					1,
					&copy
				);
			}
			else
			{
				VkImageCopy copy = {};
				copy.extent.width = dst_desc.Width;
				copy.extent.height = dst_desc.Height;
				copy.extent.depth = std::max(1u, dst_desc.Depth);

				copy.srcOffset.x = 0;
				copy.srcOffset.y = 0;
				copy.srcOffset.z = 0;

				copy.dstOffset.x = 0;
				copy.dstOffset.y = 0;
				copy.dstOffset.z = 0;

				if (src_desc.BindFlags & BIND_DEPTH_STENCIL)
				{
					copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(src_desc.Format))
					{
						copy.srcSubresource.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				copy.srcSubresource.baseArrayLayer = 0;
				copy.srcSubresource.layerCount = src_desc.ArraySize;
				copy.srcSubresource.mipLevel = 0;

				if (dst_desc.BindFlags & BIND_DEPTH_STENCIL)
				{
					copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(dst_desc.Format))
					{
						copy.dstSubresource.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				copy.dstSubresource.baseArrayLayer = 0;
				copy.dstSubresource.layerCount = dst_desc.ArraySize;
				copy.dstSubresource.mipLevel = 0;

				vkCmdCopyImage(GetDirectCommandList(cmd),
					internal_state_src->resource, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					internal_state_dst->resource, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &copy
				);
			}
		}
		else if (pDst->type == GPUResource::GPU_RESOURCE_TYPE::BUFFER && pSrc->type == GPUResource::GPU_RESOURCE_TYPE::BUFFER)
		{
			auto internal_state_src = to_internal((const GPUBuffer*)pSrc);
			auto internal_state_dst = to_internal((const GPUBuffer*)pDst);

			const GPUBufferDesc& src_desc = ((const GPUBuffer*)pSrc)->GetDesc();
			const GPUBufferDesc& dst_desc = ((const GPUBuffer*)pDst)->GetDesc();

			VkBufferCopy copy = {};
			copy.srcOffset = 0;
			copy.dstOffset = 0;
			copy.size = (VkDeviceSize)std::min(src_desc.ByteWidth, dst_desc.ByteWidth);

			vkCmdCopyBuffer(GetDirectCommandList(cmd),
				internal_state_src->resource,
				internal_state_dst->resource,
				1, &copy
			);
		}
	}
	void GraphicsDevice_Vulkan::UpdateBuffer(const GPUBuffer* buffer, const void* data, CommandList cmd, int dataSize)
	{
		assert(buffer->desc.Usage != USAGE_IMMUTABLE && "Cannot update IMMUTABLE GPUBuffer!");
		assert((int)buffer->desc.ByteWidth >= dataSize || dataSize < 0 && "Data size is too big!");

		if (dataSize == 0)
		{
			return;
		}
		auto internal_state = to_internal(buffer);

		dataSize = std::min((int)buffer->desc.ByteWidth, dataSize);
		dataSize = (dataSize >= 0 ? dataSize : buffer->desc.ByteWidth);


		if (buffer->desc.Usage == USAGE_DYNAMIC && buffer->desc.BindFlags & BIND_CONSTANT_BUFFER)
		{
			// Dynamic buffer will be used from host memory directly:
			GPUAllocation allocation = AllocateGPU(dataSize, cmd);
			memcpy(allocation.data, data, dataSize);
			internal_state->dynamic[cmd] = allocation;
			GetFrameResources().descriptors[cmd].dirty = true;
		}
		else
		{
			// Contents will be transferred to device memory:

			// barrier to transfer:

			assert(active_renderpass[cmd] == nullptr); // must not be inside render pass

			VkBufferMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			barrier.buffer = internal_state->resource;
			barrier.srcAccessMask = 0;
			if (buffer->desc.BindFlags & BIND_CONSTANT_BUFFER)
			{
				barrier.srcAccessMask |= VK_ACCESS_UNIFORM_READ_BIT;
			}
			if (buffer->desc.BindFlags & BIND_VERTEX_BUFFER)
			{
				barrier.srcAccessMask |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			}
			if (buffer->desc.BindFlags & BIND_INDEX_BUFFER)
			{
				barrier.srcAccessMask |= VK_ACCESS_INDEX_READ_BIT;
			}
			if (buffer->desc.BindFlags & BIND_SHADER_RESOURCE)
			{
				barrier.srcAccessMask |= VK_ACCESS_SHADER_READ_BIT;
			}
			if (buffer->desc.BindFlags & BIND_UNORDERED_ACCESS)
			{
				barrier.srcAccessMask |= VK_ACCESS_SHADER_WRITE_BIT;
			}
			if (buffer->desc.MiscFlags & RESOURCE_MISC_RAY_TRACING)
			{
				barrier.srcAccessMask |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
			}
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.size = VK_WHOLE_SIZE;

			frame_bufferBarriers[cmd].push_back(barrier);
			barrier_flush(cmd);

			// issue data copy:
			uint8_t* dest = GetFrameResources().resourceBuffer[cmd].allocate(dataSize, 1);
			memcpy(dest, data, dataSize);

			VkBufferCopy copyRegion = {};
			copyRegion.size = dataSize;
			copyRegion.srcOffset = GetFrameResources().resourceBuffer[cmd].calculateOffset(dest);
			copyRegion.dstOffset = 0;

			vkCmdCopyBuffer(GetDirectCommandList(cmd), 
				std::static_pointer_cast<Buffer_Vulkan>(GetFrameResources().resourceBuffer[cmd].buffer.internal_state)->resource,
				internal_state->resource, 1, &copyRegion);



			// reverse barrier:
			std::swap(barrier.srcAccessMask, barrier.dstAccessMask);

			frame_bufferBarriers[cmd].push_back(barrier);

		}

	}
	void GraphicsDevice_Vulkan::QueryBegin(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		auto internal_state = to_internal(heap);

		switch (heap->desc.type)
		{
		case GPU_QUERY_TYPE_OCCLUSION_BINARY:
			vkCmdBeginQuery(GetDirectCommandList(cmd), internal_state->pool, index, 0);
			break;
		case GPU_QUERY_TYPE_OCCLUSION:
			vkCmdBeginQuery(GetDirectCommandList(cmd), internal_state->pool, index, VK_QUERY_CONTROL_PRECISE_BIT);
			break;
		}
	}
	void GraphicsDevice_Vulkan::QueryEnd(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		auto internal_state = to_internal(heap);

		switch (heap->desc.type)
		{
		case GPU_QUERY_TYPE_TIMESTAMP:
			vkCmdWriteTimestamp(GetDirectCommandList(cmd), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, internal_state->pool, index);
			break;
		case GPU_QUERY_TYPE_OCCLUSION_BINARY:
		case GPU_QUERY_TYPE_OCCLUSION:
			vkCmdEndQuery(GetDirectCommandList(cmd), internal_state->pool, index);
			break;
		}
	}
	void GraphicsDevice_Vulkan::Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd)
	{
		auto& memoryBarriers = frame_memoryBarriers[cmd];
		auto& imageBarriers = frame_imageBarriers[cmd];
		auto& bufferBarriers = frame_bufferBarriers[cmd];

		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const GPUBarrier& barrier = barriers[i];

			switch (barrier.type)
			{
			default:
			case GPUBarrier::MEMORY_BARRIER:
			{
				VkMemoryBarrier barrierdesc = {};
				barrierdesc.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				barrierdesc.pNext = nullptr;
				barrierdesc.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
				barrierdesc.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
				if (CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING))
				{
					barrierdesc.srcAccessMask |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
					barrierdesc.dstAccessMask |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
				}

				memoryBarriers.push_back(barrierdesc);
			}
			break;
			case GPUBarrier::IMAGE_BARRIER:
			{
				const TextureDesc& desc = barrier.image.texture->GetDesc();
				auto internal_state = to_internal(barrier.image.texture);

				VkImageMemoryBarrier barrierdesc = {};
				barrierdesc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrierdesc.pNext = nullptr;
				barrierdesc.image = internal_state->resource;
				barrierdesc.oldLayout = _ConvertImageLayout(barrier.image.layout_before);
				barrierdesc.newLayout = _ConvertImageLayout(barrier.image.layout_after);
				barrierdesc.srcAccessMask = _ParseImageLayout(barrier.image.layout_before);
				barrierdesc.dstAccessMask = _ParseImageLayout(barrier.image.layout_after);
				if (desc.BindFlags & BIND_DEPTH_STENCIL)
				{
					barrierdesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(desc.Format))
					{
						barrierdesc.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					barrierdesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				if (barrier.image.mip >= 0 || barrier.image.slice >= 0)
				{
					barrierdesc.subresourceRange.baseMipLevel = (uint32_t)std::max(0, barrier.image.mip);
					barrierdesc.subresourceRange.levelCount = 1;
					barrierdesc.subresourceRange.baseArrayLayer = (uint32_t)std::max(0, barrier.image.slice);
					barrierdesc.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS; // this should be 1 but cubemap array complains in debug layer...
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

				bool found = false;
				for (auto& x : imageBarriers)
				{
					// Two duplicate barriers will be combined into one:
					if (x.image == barrierdesc.image &&
						x.subresourceRange.baseMipLevel == barrierdesc.subresourceRange.baseMipLevel &&
						x.subresourceRange.levelCount == barrierdesc.subresourceRange.levelCount &&
						x.subresourceRange.baseArrayLayer == barrierdesc.subresourceRange.baseArrayLayer &&
						x.subresourceRange.layerCount == barrierdesc.subresourceRange.layerCount
						)
					{
						found = true;
						x.newLayout = barrierdesc.newLayout;
						x.dstAccessMask |= barrierdesc.dstAccessMask;
						break;
					}
				}

				if (!found)
				{
					imageBarriers.push_back(barrierdesc);
				}
			}
			break;
			case GPUBarrier::BUFFER_BARRIER:
			{
				auto internal_state = to_internal(barrier.buffer.buffer);

				VkBufferMemoryBarrier barrierdesc = {};
				barrierdesc.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				barrierdesc.pNext = nullptr;
				barrierdesc.buffer = internal_state->resource;
				barrierdesc.size = barrier.buffer.buffer->GetDesc().ByteWidth;
				barrierdesc.offset = 0;
				barrierdesc.srcAccessMask = _ParseBufferState(barrier.buffer.state_before);
				barrierdesc.dstAccessMask = _ParseBufferState(barrier.buffer.state_after);
				barrierdesc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrierdesc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				bool found = false;
				for (auto& x : bufferBarriers)
				{
					// Two duplicate barriers will be combined into one:
					if (x.buffer == barrierdesc.buffer &&
						x.srcAccessMask == barrierdesc.srcAccessMask &&
						x.dstAccessMask == barrierdesc.dstAccessMask
						)
					{
						found = true;
						x.dstAccessMask |= barrierdesc.dstAccessMask;
						break;
					}
				}

				if (!found)
				{
					bufferBarriers.push_back(barrierdesc);
				}
			}
			break;
			}
		}
	}
	void GraphicsDevice_Vulkan::BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src)
	{
		barrier_flush(cmd);

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

		std::vector<VkAccelerationStructureGeometryKHR> geometries = dst_internal->geometries; // copy!
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges;

		info.type = dst_internal->createInfo.type;
		info.geometryCount = (uint32_t)geometries.size();
		ranges.reserve(info.geometryCount);

		switch (dst->desc.type)
		{
		case RaytracingAccelerationStructureDesc::BOTTOMLEVEL:
		{
			size_t i = 0;
			for (auto& x : dst->desc.bottomlevel.geometries)
			{
				auto& geometry = geometries[i];

				ranges.emplace_back();
				auto& range = ranges.back();
				range = {};

				if (x._flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE)
				{
					geometry.flags |= VK_GEOMETRY_OPAQUE_BIT_KHR;
				}
				if (x._flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_NO_DUPLICATE_ANYHIT_INVOCATION)
				{
					geometry.flags |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
				}

				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::TRIANGLES)
				{
					geometry.geometry.triangles.vertexData.deviceAddress = to_internal(&x.triangles.vertexBuffer)->address +
						x.triangles.vertexByteOffset;

					geometry.geometry.triangles.indexData.deviceAddress = to_internal(&x.triangles.indexBuffer)->address +
						x.triangles.indexOffset * (x.triangles.indexFormat == INDEXBUFFER_FORMAT::INDEXFORMAT_16BIT ? sizeof(uint16_t) : sizeof(uint32_t));

					if (x._flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
					{
						geometry.geometry.triangles.transformData.deviceAddress = to_internal(&x.triangles.transform3x4Buffer)->address;
						range.transformOffset = x.triangles.transform3x4BufferOffset;
					}

					range.primitiveCount = x.triangles.indexCount / 3;
					range.primitiveOffset = 0;
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::PROCEDURAL_AABBS)
				{
					geometry.geometry.aabbs.data.deviceAddress = to_internal(&x.aabbs.aabbBuffer)->address;

					range.primitiveCount = x.aabbs.count;
					range.primitiveOffset = x.aabbs.offset;
				}

				i++;
			}
		}
		break;
		case RaytracingAccelerationStructureDesc::TOPLEVEL:
		{
			auto& geometry = geometries.back();
			geometry.geometry.instances.data.deviceAddress = to_internal(&dst->desc.toplevel.instanceBuffer)->address;

			ranges.emplace_back();
			auto& range = ranges.back();
			range = {};
			range.primitiveCount = dst->desc.toplevel.count;
			range.primitiveOffset = dst->desc.toplevel.offset;
		}
		break;
		}

		info.pGeometries = geometries.data();

		VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = ranges.data();

		vkCmdBuildAccelerationStructuresKHR(
			GetDirectCommandList(cmd),
			1,
			&info,
			&pRangeInfo
		);
	}
	void GraphicsDevice_Vulkan::BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd)
	{
		prev_pipeline_hash[cmd] = 0;
		active_rt[cmd] = rtpso;

		BindComputeShader(rtpso->desc.shaderlibraries.front().shader, cmd);

		vkCmdBindPipeline(GetDirectCommandList(cmd), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, to_internal(rtpso)->pipeline);
	}
	void GraphicsDevice_Vulkan::DispatchRays(const DispatchRaysDesc* desc, CommandList cmd)
	{
		predispatch(cmd);

		VkStridedDeviceAddressRegionKHR raygen = {};
		raygen.deviceAddress = desc->raygeneration.buffer ? to_internal(desc->raygeneration.buffer)->address : 0;
		raygen.deviceAddress += desc->raygeneration.offset;
		raygen.size = desc->raygeneration.size;
		raygen.stride = raygen.size; // raygen specifically must be size == stride
		
		VkStridedDeviceAddressRegionKHR miss = {};
		miss.deviceAddress = desc->miss.buffer ? to_internal(desc->miss.buffer)->address : 0;
		miss.deviceAddress += desc->miss.offset;
		miss.size = desc->miss.size;
		miss.stride = desc->miss.stride;

		VkStridedDeviceAddressRegionKHR hitgroup = {};
		hitgroup.deviceAddress = desc->hitgroup.buffer ? to_internal(desc->hitgroup.buffer)->address : 0;
		hitgroup.deviceAddress += desc->hitgroup.offset;
		hitgroup.size = desc->hitgroup.size;
		hitgroup.stride = desc->hitgroup.stride;

		VkStridedDeviceAddressRegionKHR callable = {};
		callable.deviceAddress = desc->callable.buffer ? to_internal(desc->callable.buffer)->address : 0;
		callable.deviceAddress += desc->callable.offset;
		callable.size = desc->callable.size;
		callable.stride = desc->callable.stride;

		vkCmdTraceRaysKHR(
			GetDirectCommandList(cmd),
			&raygen,
			&miss,
			&hitgroup,
			&callable,
			desc->Width, 
			desc->Height, 
			desc->Depth
		);
	}
	void GraphicsDevice_Vulkan::PushConstants(const void* data, uint32_t size, CommandList cmd)
	{
		std::memcpy(pushconstants[cmd].data, data, size);
		pushconstants[cmd].size = size;
	}

	GraphicsDevice::GPUAllocation GraphicsDevice_Vulkan::AllocateGPU(size_t dataSize, CommandList cmd)
	{
		GPUAllocation result;
		if (dataSize == 0)
		{
			return result;
		}

		FrameResources::ResourceFrameAllocator& allocator = GetFrameResources().resourceBuffer[cmd];
		uint8_t* dest = allocator.allocate(dataSize, 256);
		assert(dest != nullptr);

		result.buffer = &allocator.buffer;
		result.offset = (uint32_t)allocator.calculateOffset(dest);
		result.data = (void*)dest;
		return result;
	}

	void GraphicsDevice_Vulkan::EventBegin(const char* name, CommandList cmd)
	{
		if (vkCmdBeginDebugUtilsLabelEXT != nullptr)
		{
			VkDebugUtilsLabelEXT label = {};
			label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			label.pLabelName = name;
			label.color[0] = 0;
			label.color[1] = 0;
			label.color[2] = 0;
			label.color[3] = 1;
			vkCmdBeginDebugUtilsLabelEXT(GetDirectCommandList(cmd), &label);
		}
	}
	void GraphicsDevice_Vulkan::EventEnd(CommandList cmd)
	{
		if (vkCmdEndDebugUtilsLabelEXT != nullptr)
		{
			vkCmdEndDebugUtilsLabelEXT(GetDirectCommandList(cmd));
		}
	}
	void GraphicsDevice_Vulkan::SetMarker(const char* name, CommandList cmd)
	{
		if (vkCmdInsertDebugUtilsLabelEXT != nullptr)
		{
			VkDebugUtilsLabelEXT label = {};
			label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			label.pLabelName = name;
			label.color[0] = 0;
			label.color[1] = 0;
			label.color[2] = 0;
			label.color[3] = 1;
			vkCmdInsertDebugUtilsLabelEXT(GetDirectCommandList(cmd), &label);
		}
	}

}


#endif // WICKEDENGINE_BUILD_VULKAN
