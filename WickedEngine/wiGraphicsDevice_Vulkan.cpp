#include "wiGraphicsDevice_Vulkan.h"

#ifdef WICKEDENGINE_BUILD_VULKAN
#pragma comment(lib,"vulkan-1.lib")

#include "Utility/spirv_reflect.hpp"

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

// Enabling ray tracing might crash RenderDoc:
#define ENABLE_RAYTRACING_EXTENSION

// These shifts are made so that Vulkan resource bindings slots don't interfere with each other across shader stages:
//	These are also defined in compile_shaders_spirv.py as shift numbers, and it needs to be synced!
#define VULKAN_BINDING_SHIFT_B 0
#define VULKAN_BINDING_SHIFT_T 1000
#define VULKAN_BINDING_SHIFT_U 2000
#define VULKAN_BINDING_SHIFT_S 3000

namespace wiGraphics
{

	PFN_vkCreateRayTracingPipelinesKHR GraphicsDevice_Vulkan::createRayTracingPipelinesKHR = nullptr;
	PFN_vkCreateAccelerationStructureKHR GraphicsDevice_Vulkan::createAccelerationStructureKHR = nullptr;
	PFN_vkBindAccelerationStructureMemoryKHR GraphicsDevice_Vulkan::bindAccelerationStructureMemoryKHR = nullptr;
	PFN_vkDestroyAccelerationStructureKHR GraphicsDevice_Vulkan::destroyAccelerationStructureKHR = nullptr;
	PFN_vkGetAccelerationStructureMemoryRequirementsKHR GraphicsDevice_Vulkan::getAccelerationStructureMemoryRequirementsKHR = nullptr;
	PFN_vkGetAccelerationStructureDeviceAddressKHR GraphicsDevice_Vulkan::getAccelerationStructureDeviceAddressKHR = nullptr;
	PFN_vkGetRayTracingShaderGroupHandlesKHR GraphicsDevice_Vulkan::getRayTracingShaderGroupHandlesKHR = nullptr;
	PFN_vkCmdBuildAccelerationStructureKHR GraphicsDevice_Vulkan::cmdBuildAccelerationStructureKHR = nullptr;
	PFN_vkCmdTraceRaysKHR GraphicsDevice_Vulkan::cmdTraceRaysKHR = nullptr;

	PFN_vkCmdDrawMeshTasksNV GraphicsDevice_Vulkan::cmdDrawMeshTasksNV = nullptr;
	PFN_vkCmdDrawMeshTasksIndirectNV GraphicsDevice_Vulkan::cmdDrawMeshTasksIndirectNV = nullptr;

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
		case wiGraphics::IMAGE_LAYOUT_GENERAL:
			return VK_IMAGE_LAYOUT_GENERAL;
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
		case wiGraphics::SHADERSTAGE_COUNT:
			return VK_SHADER_STAGE_ALL;
		}
	}


	inline VkAccessFlags _ParseImageLayout(IMAGE_LAYOUT value)
	{
		VkAccessFlags flags = 0;

		switch (value)
		{
		case wiGraphics::IMAGE_LAYOUT_GENERAL:
			flags |= VK_ACCESS_SHADER_READ_BIT;
			flags |= VK_ACCESS_SHADER_WRITE_BIT;
			flags |= VK_ACCESS_TRANSFER_READ_BIT;
			flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
			flags |= VK_ACCESS_MEMORY_READ_BIT;
			flags |= VK_ACCESS_MEMORY_WRITE_BIT;
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
		case wiGraphics::BUFFER_STATE_GENERAL:
			flags |= VK_ACCESS_SHADER_READ_BIT;
			flags |= VK_ACCESS_SHADER_WRITE_BIT;
			flags |= VK_ACCESS_TRANSFER_READ_BIT;
			flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
			flags |= VK_ACCESS_HOST_READ_BIT;
			flags |= VK_ACCESS_HOST_WRITE_BIT;
			flags |= VK_ACCESS_MEMORY_READ_BIT;
			flags |= VK_ACCESS_MEMORY_WRITE_BIT;
			flags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
			flags |= VK_ACCESS_INDEX_READ_BIT;
			flags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			flags |= VK_ACCESS_UNIFORM_READ_BIT;
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
	
	// Extension functions:
	PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT = nullptr;
	PFN_vkCmdBeginDebugUtilsLabelEXT cmdBeginDebugUtilsLabelEXT = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT cmdEndDebugUtilsLabelEXT = nullptr;
	PFN_vkCmdInsertDebugUtilsLabelEXT cmdInsertDebugUtilsLabelEXT = nullptr;

	bool checkDeviceExtensionSupport(const char* checkExtension, 
		const std::vector<VkExtensionProperties>& available_deviceExtensions) {

		for (const auto& x : available_deviceExtensions)
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
	bool checkValidationLayerSupport() {
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
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData) {

		std::stringstream ss("");
		ss << "[VULKAN validation layer]: " << msg << std::endl;

		std::clog << ss.str();
#ifdef _WIN32
		OutputDebugStringA(ss.str().c_str());
#endif

		return VK_FALSE;
	}
	VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
		auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pCallback);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}
	void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
		if (func != nullptr) {
			func(instance, callback, pAllocator);
		}
	}

	// Queue families:
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			VkBool32 presentSupport = false;
			VkResult res = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			assert(res == VK_SUCCESS);

			if (indices.presentFamily < 0 && queueFamily.queueCount > 0 && presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.graphicsFamily < 0 && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
				indices.copyFamily = i;
			}

			i++;
		}

		return indices;
	}

	// Swapchain helpers:
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
		SwapChainSupportDetails details;

		VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
		assert(res == VK_SUCCESS);

		uint32_t formatCount;
		res = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		assert(res == VK_SUCCESS);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			res = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
			assert(res == VK_SUCCESS);
		}

		uint32_t presentModeCount;
		res = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		assert(res == VK_SUCCESS);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			res = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
			assert(res == VK_SUCCESS);
		}

		return details;
	}

	uint32_t findMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		assert(0);
		return ~0;
	}

	// Device selection helpers:
	const std::vector<const char*> required_deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME,
	};
	bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) 
	{
		QueueFamilyIndices indices = findQueueFamilies(device, surface);
		if (!indices.isComplete())
		{
			return false;
		}

		uint32_t extensionCount;
		VkResult res = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		assert(res == VK_SUCCESS);
		std::vector<VkExtensionProperties> available(extensionCount);
		res = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, available.data());
		assert(res == VK_SUCCESS);

		for (auto& x : required_deviceExtensions)
		{
			if (!checkDeviceExtensionSupport(x, available))
			{
				return false; // device doesn't have a required extension
			}
		}

		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);

		return !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
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


	// Destroyers:
	struct Buffer_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VmaAllocation allocation = nullptr;
		VkBuffer resource = VK_NULL_HANDLE;
		VkBufferView cbv = VK_NULL_HANDLE;
		VkBufferView srv = VK_NULL_HANDLE;
		VkBufferView uav = VK_NULL_HANDLE;
		std::vector<VkBufferView> subresources_srv;
		std::vector<VkBufferView> subresources_uav;

		GraphicsDevice::GPUAllocation dynamic[COMMANDLIST_COUNT];

		~Buffer_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (resource) allocationhandler->destroyer_buffers.push_back(std::make_pair(std::make_pair(resource, allocation), framecount));
			if (cbv) allocationhandler->destroyer_bufferviews.push_back(std::make_pair(cbv, framecount));
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
		VkImageView uav = VK_NULL_HANDLE;
		VkImageView rtv = VK_NULL_HANDLE;
		VkImageView dsv = VK_NULL_HANDLE;
		std::vector<VkImageView> subresources_srv;
		std::vector<VkImageView> subresources_uav;
		std::vector<VkImageView> subresources_rtv;
		std::vector<VkImageView> subresources_dsv;

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
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Sampler_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VkSampler resource = VK_NULL_HANDLE;

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
	struct Query_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		GPU_QUERY_TYPE query_type = GPU_QUERY_TYPE_INVALID;
		uint32_t query_index = ~0;

		~Query_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			if (query_index != ~0)
			{
				allocationhandler->destroylocker.lock();
				uint64_t framecount = allocationhandler->framecount;
				switch (query_type)
				{
				case wiGraphics::GPU_QUERY_TYPE_OCCLUSION:
				case wiGraphics::GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
					allocationhandler->destroyer_queries_occlusion.push_back(std::make_pair(query_index, framecount));
					break;
				case wiGraphics::GPU_QUERY_TYPE_TIMESTAMP:
					allocationhandler->destroyer_queries_timestamp.push_back(std::make_pair(query_index, framecount));
					break;
				}
				allocationhandler->destroylocker.unlock();
			}
		}
	};
	struct Shader_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VkShaderModule shaderModule = VK_NULL_HANDLE;
		VkPipeline pipeline_cs = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout_cs = VK_NULL_HANDLE;
		VkPipelineShaderStageCreateInfo stageInfo = {};
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		std::vector<VkImageViewType> imageViewTypes;

		std::vector<spirv_cross::EntryPoint> entrypoints;

		~Shader_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (shaderModule) allocationhandler->destroyer_shadermodules.push_back(std::make_pair(shaderModule, framecount));
			if (pipeline_cs) allocationhandler->destroyer_pipelines.push_back(std::make_pair(pipeline_cs, framecount));
			if (pipelineLayout_cs) allocationhandler->destroyer_pipelineLayouts.push_back(std::make_pair(pipelineLayout_cs, framecount));
			if (descriptorSetLayout) allocationhandler->destroyer_descriptorSetLayouts.push_back(std::make_pair(descriptorSetLayout, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct PipelineState_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		std::vector<VkImageViewType> imageViewTypes;

		~PipelineState_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (pipelineLayout) allocationhandler->destroyer_pipelineLayouts.push_back(std::make_pair(pipelineLayout, framecount));
			if (descriptorSetLayout) allocationhandler->destroyer_descriptorSetLayouts.push_back(std::make_pair(descriptorSetLayout, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct RenderPass_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VkRenderPass renderpass = VK_NULL_HANDLE;
		VkFramebuffer framebuffer = VK_NULL_HANDLE;
		VkRenderPassBeginInfo beginInfo;
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

		VkAccelerationStructureCreateInfoKHR info = {};
		std::vector<VkAccelerationStructureCreateGeometryTypeInfoKHR> geometries;
		VkDeviceSize scratch_offset = 0;
		VkDeviceAddress as_address = 0;

		~BVH_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (buffer) allocationhandler->destroyer_buffers.push_back(std::make_pair(std::make_pair(buffer, allocation), framecount));
			if (resource) allocationhandler->destroyer_bvhs.push_back(std::make_pair(resource, framecount));
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
	struct DescriptorTable_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VkDescriptorSetLayout layout = VK_NULL_HANDLE;
		VkDescriptorUpdateTemplate updatetemplate = VK_NULL_HANDLE;

		std::vector<size_t> resource_write_remap;
		std::vector<size_t> sampler_write_remap;

		struct Descriptor
		{
			union
			{
				VkDescriptorImageInfo imageinfo;
				VkDescriptorBufferInfo bufferInfo;
				VkBufferView bufferView;
				VkAccelerationStructureKHR accelerationStructure;
			};
		};
		std::vector<Descriptor> descriptors;

		~DescriptorTable_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (layout) allocationhandler->destroyer_descriptorSetLayouts.push_back(std::make_pair(layout, framecount));
			if (updatetemplate) allocationhandler->destroyer_descriptorUpdateTemplates.push_back(std::make_pair(updatetemplate, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct RootSignature_Vulkan
	{
		std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

		bool dirty[COMMANDLIST_COUNT] = {};
		std::vector<const DescriptorTable*> last_tables[COMMANDLIST_COUNT];
		std::vector<VkDescriptorSet> last_descriptorsets[COMMANDLIST_COUNT];
		std::vector<const GPUBuffer*> root_descriptors[COMMANDLIST_COUNT];
		std::vector<uint32_t> root_offsets[COMMANDLIST_COUNT];

		struct RootRemap
		{
			uint32_t space = 0;
			uint32_t binding = 0;
			uint32_t rangeIndex = 0;
		};
		std::vector<RootRemap> root_remap;

		~RootSignature_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (pipelineLayout) allocationhandler->destroyer_pipelineLayouts.push_back(std::make_pair(pipelineLayout, framecount));
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
	Query_Vulkan* to_internal(const GPUQuery* param)
	{
		return static_cast<Query_Vulkan*>(param->internal_state.get());
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
	DescriptorTable_Vulkan* to_internal(const DescriptorTable* param)
	{
		return static_cast<DescriptorTable_Vulkan*>(param->internal_state.get());
	}
	RootSignature_Vulkan* to_internal(const RootSignature* param)
	{
		return static_cast<RootSignature_Vulkan*>(param->internal_state.get());
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
		bufferInfo.flags = 0;

		VkResult res;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		res = vmaCreateBuffer(device->allocationhandler->allocator, &bufferInfo, &allocInfo, &internal_state->resource, &internal_state->allocation, nullptr);
		assert(res == VK_SUCCESS);

		void* pData = internal_state->allocation->GetMappedData();
		dataCur = dataBegin = reinterpret_cast<uint8_t*>(pData);
		dataEnd = dataBegin + size;

		// Because the "buffer" is created by hand in this, fill the desc to indicate how it can be used:
		this->buffer.desc.ByteWidth = (uint32_t)((size_t)dataEnd - (size_t)dataBegin);
		this->buffer.desc.Usage = USAGE_DYNAMIC;
		this->buffer.desc.BindFlags = BIND_VERTEX_BUFFER | BIND_INDEX_BUFFER | BIND_SHADER_RESOURCE;
		this->buffer.desc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
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

	void GraphicsDevice_Vulkan::FrameResources::DescriptorTableFrameAllocator::init(GraphicsDevice_Vulkan* device)
	{
		this->device = device;

		// Important that these don't reallocate themselves during writing dexcriptors!
		descriptorWrites.reserve(128);
		bufferInfos.reserve(128);
		imageInfos.reserve(128);
		texelBufferViews.reserve(128);
		accelerationStructureViews.reserve(128);

		VkResult res;

		// Create descriptor pool:
		VkDescriptorPoolSize poolSizes[9] = {};

		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = GPU_RESOURCE_HEAP_CBV_COUNT * poolSize;

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		poolSizes[1].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT * poolSize;

		poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		poolSizes[2].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT * poolSize;

		poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[3].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT * poolSize;

		poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[4].descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT * poolSize;

		poolSizes[5].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		poolSizes[5].descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT * poolSize;

		poolSizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[6].descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT * poolSize;

		poolSizes[7].type = VK_DESCRIPTOR_TYPE_SAMPLER;
		poolSizes[7].descriptorCount = GPU_SAMPLER_HEAP_COUNT * poolSize;

		poolSizes[8].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		poolSizes[8].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT * poolSize;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = arraysize(poolSizes);
		poolInfo.pPoolSizes = poolSizes;
		poolInfo.maxSets = poolSize;
		//poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		res = vkCreateDescriptorPool(device->device, &poolInfo, nullptr, &descriptorPool);
		assert(res == VK_SUCCESS);

		reset();

	}
	void GraphicsDevice_Vulkan::FrameResources::DescriptorTableFrameAllocator::destroy()
	{
		if (descriptorPool != VK_NULL_HANDLE)
		{
			device->allocationhandler->destroyer_descriptorPools.push_back(std::make_pair(descriptorPool, device->FRAMECOUNT));
			descriptorPool = VK_NULL_HANDLE;
		}
	}
	void GraphicsDevice_Vulkan::FrameResources::DescriptorTableFrameAllocator::reset()
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
	void GraphicsDevice_Vulkan::FrameResources::DescriptorTableFrameAllocator::validate(bool graphics, CommandList cmd, bool raytracing)
	{
		if (!dirty)
			return;
		dirty = true;

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
				imageInfos.back().imageLayout = VK_IMAGE_LAYOUT_GENERAL;

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

					VkImageLayout layout = _ConvertImageLayout(texture->desc.layout);
					if (layout != VK_IMAGE_LAYOUT_GENERAL && layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
					{
						// Means texture initial layout is not compatible, so it must have been transitioned
						layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					}
					imageInfos.back().imageLayout = layout;
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

		vkUpdateDescriptorSets(device->device, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

		vkCmdBindDescriptorSets(device->GetDirectCommandList(cmd),
			graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : (raytracing ? VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR : VK_PIPELINE_BIND_POINT_COMPUTE),
			pipelineLayout, 0, 1, &descriptorSet, 0, nullptr
		);
	}
	VkDescriptorSet GraphicsDevice_Vulkan::FrameResources::DescriptorTableFrameAllocator::commit(const DescriptorTable* table)
	{
		auto internal_state = to_internal(table);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &internal_state->layout;

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

		vkUpdateDescriptorSetWithTemplate(
			device->device,
			descriptorSet,
			internal_state->updatetemplate,
			internal_state->descriptors.data()
		);

		return descriptorSet;
	}

	void GraphicsDevice_Vulkan::pso_validate(CommandList cmd)
	{
		if (!dirty_pso[cmd])
			return;

		const PipelineState* pso = active_pso[cmd];
		size_t pipeline_hash = prev_pipeline_hash[cmd];

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
				VkGraphicsPipelineCreateInfo pipelineInfo = {};
				//pipelineInfo.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
				pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				if (pso->desc.rootSignature == nullptr)
				{
					pipelineInfo.layout = to_internal(pso)->pipelineLayout;
				}
				else
				{
					pipelineInfo.layout = to_internal(pso->desc.rootSignature)->pipelineLayout;
				}
				pipelineInfo.renderPass = active_renderpass[cmd] == nullptr ? defaultRenderPass : to_internal(active_renderpass[cmd])->renderpass;
				pipelineInfo.subpass = 0;
				pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

				// Shaders:

				uint32_t shaderStageCount = 0;
				VkPipelineShaderStageCreateInfo shaderStages[SHADERSTAGE_COUNT - 1];
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

				// Input layout:
				VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
				vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				std::vector<VkVertexInputBindingDescription> bindings;
				std::vector<VkVertexInputAttributeDescription> attributes;
				if (pso->desc.il != nullptr)
				{
					uint32_t lastBinding = 0xFFFFFFFF;
					for (auto& x : pso->desc.il->desc)
					{
						VkVertexInputBindingDescription bind = {};
						bind.binding = x.InputSlot;
						bind.inputRate = x.InputSlotClass == INPUT_PER_VERTEX_DATA ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
						bind.stride = x.AlignedByteOffset;
						if (bind.stride == InputLayoutDesc::APPEND_ALIGNED_ELEMENT)
						{
							// need to manually resolve this from the format spec.
							bind.stride = GetFormatStride(x.Format);
						}

						if (lastBinding != bind.binding)
						{
							bindings.push_back(bind);
							lastBinding = bind.binding;
						}
						else
						{
							bindings.back().stride += bind.stride;
						}
					}

					uint32_t offset = 0;
					uint32_t i = 0;
					lastBinding = 0xFFFFFFFF;
					for (auto& x : pso->desc.il->desc)
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
						if (attr.offset == InputLayoutDesc::APPEND_ALIGNED_ELEMENT)
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

				// Primitive type:
				VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
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
				VkPipelineRasterizationStateCreateInfo rasterizer = {};
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
				VkPipelineRasterizationDepthClipStateCreateInfoEXT depthclip = {};
				depthclip.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;
				depthclip.depthClipEnable = VK_TRUE;
				rasterizer.pNext = &depthclip;

				if (pso->desc.rs != nullptr)
				{
					const RasterizerStateDesc& desc = pso->desc.rs->desc;

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
				VkViewport viewport = {};
				viewport.x = 0;
				viewport.y = 0;
				viewport.width = 65535;
				viewport.height = 65535;
				viewport.minDepth = 0;
				viewport.maxDepth = 1;

				VkRect2D scissor = {};
				scissor.extent.width = 65535;
				scissor.extent.height = 65535;

				VkPipelineViewportStateCreateInfo viewportState = {};
				viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				viewportState.viewportCount = 1;
				viewportState.pViewports = &viewport;
				viewportState.scissorCount = 1;
				viewportState.pScissors = &scissor;

				pipelineInfo.pViewportState = &viewportState;


				// Depth-Stencil:
				VkPipelineDepthStencilStateCreateInfo depthstencil = {};
				depthstencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
				if (pso->desc.dss != nullptr)
				{
					depthstencil.depthTestEnable = pso->desc.dss->desc.DepthEnable ? VK_TRUE : VK_FALSE;
					depthstencil.depthWriteEnable = pso->desc.dss->desc.DepthWriteMask == DEPTH_WRITE_MASK_ZERO ? VK_FALSE : VK_TRUE;
					depthstencil.depthCompareOp = _ConvertComparisonFunc(pso->desc.dss->desc.DepthFunc);

					depthstencil.stencilTestEnable = pso->desc.dss->desc.StencilEnable ? VK_TRUE : VK_FALSE;

					depthstencil.front.compareMask = pso->desc.dss->desc.StencilReadMask;
					depthstencil.front.writeMask = pso->desc.dss->desc.StencilWriteMask;
					depthstencil.front.reference = 0; // runtime supplied
					depthstencil.front.compareOp = _ConvertComparisonFunc(pso->desc.dss->desc.FrontFace.StencilFunc);
					depthstencil.front.passOp = _ConvertStencilOp(pso->desc.dss->desc.FrontFace.StencilPassOp);
					depthstencil.front.failOp = _ConvertStencilOp(pso->desc.dss->desc.FrontFace.StencilFailOp);
					depthstencil.front.depthFailOp = _ConvertStencilOp(pso->desc.dss->desc.FrontFace.StencilDepthFailOp);

					depthstencil.back.compareMask = pso->desc.dss->desc.StencilReadMask;
					depthstencil.back.writeMask = pso->desc.dss->desc.StencilWriteMask;
					depthstencil.back.reference = 0; // runtime supplied
					depthstencil.back.compareOp = _ConvertComparisonFunc(pso->desc.dss->desc.BackFace.StencilFunc);
					depthstencil.back.passOp = _ConvertStencilOp(pso->desc.dss->desc.BackFace.StencilPassOp);
					depthstencil.back.failOp = _ConvertStencilOp(pso->desc.dss->desc.BackFace.StencilFailOp);
					depthstencil.back.depthFailOp = _ConvertStencilOp(pso->desc.dss->desc.BackFace.StencilDepthFailOp);

					depthstencil.depthBoundsTestEnable = VK_FALSE;
				}

				pipelineInfo.pDepthStencilState = &depthstencil;


				// MSAA:
				VkPipelineMultisampleStateCreateInfo multisampling = {};
				multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisampling.sampleShadingEnable = VK_FALSE;
				multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
				if (active_renderpass[cmd] != nullptr && active_renderpass[cmd]->desc.attachments.size() > 0)
				{
					multisampling.rasterizationSamples = (VkSampleCountFlagBits)active_renderpass[cmd]->desc.attachments[0].texture->desc.SampleCount;
				}
				multisampling.minSampleShading = 1.0f;
				VkSampleMask samplemask = pso->desc.sampleMask;
				multisampling.pSampleMask = &samplemask;
				multisampling.alphaToCoverageEnable = VK_FALSE;
				multisampling.alphaToOneEnable = VK_FALSE;

				pipelineInfo.pMultisampleState = &multisampling;


				// Blending:
				uint32_t numBlendAttachments = 0;
				VkPipelineColorBlendAttachmentState colorBlendAttachments[8];
				const size_t blend_loopCount = active_renderpass[cmd] == nullptr ? 1 : active_renderpass[cmd]->desc.attachments.size();
				for (size_t i = 0; i < blend_loopCount; ++i)
				{
					if (active_renderpass[cmd] != nullptr && active_renderpass[cmd]->desc.attachments[i].type != RenderPassAttachment::RENDERTARGET)
					{
						continue;
					}

					RenderTargetBlendStateDesc desc = pso->desc.bs->desc.RenderTarget[numBlendAttachments];
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


				// Tessellation:
				VkPipelineTessellationStateCreateInfo tessellationInfo = {};
				tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
				tessellationInfo.patchControlPoints = 3;

				pipelineInfo.pTessellationState = &tessellationInfo;




				// Dynamic state will be specified at runtime:
				VkDynamicState dynamicStates[] = {
					VK_DYNAMIC_STATE_VIEWPORT,
					VK_DYNAMIC_STATE_SCISSOR,
					VK_DYNAMIC_STATE_STENCIL_REFERENCE,
					VK_DYNAMIC_STATE_BLEND_CONSTANTS
				};

				VkPipelineDynamicStateCreateInfo dynamicState = {};
				dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				dynamicState.dynamicStateCount = arraysize(dynamicStates);
				dynamicState.pDynamicStates = dynamicStates;

				pipelineInfo.pDynamicState = &dynamicState;

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

	void GraphicsDevice_Vulkan::predraw(CommandList cmd)
	{
		pso_validate(cmd);

		if (active_pso[cmd]->desc.rootSignature == nullptr)
		{
			GetFrameResources().descriptors[cmd].validate(true, cmd);
		}
		else
		{
			auto rootsig_internal = to_internal(active_pso[cmd]->desc.rootSignature);
			if (rootsig_internal->dirty[cmd])
			{
				rootsig_internal->dirty[cmd] = false;
				vkCmdBindDescriptorSets(
					GetDirectCommandList(cmd),
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					rootsig_internal->pipelineLayout,
					0,
					(uint32_t)rootsig_internal->last_descriptorsets[cmd].size(),
					rootsig_internal->last_descriptorsets[cmd].data(),
					(uint32_t)rootsig_internal->root_offsets[cmd].size(),
					rootsig_internal->root_offsets[cmd].data()
				);
			}
		}
	}
	void GraphicsDevice_Vulkan::predispatch(CommandList cmd)
	{
		if (active_cs[cmd]->rootSignature == nullptr)
		{
			GetFrameResources().descriptors[cmd].validate(false, cmd);
		}
		else
		{
			auto rootsig_internal = to_internal(active_cs[cmd]->rootSignature);
			if (rootsig_internal->dirty[cmd])
			{
				rootsig_internal->dirty[cmd] = false;
				vkCmdBindDescriptorSets(
					GetDirectCommandList(cmd),
					VK_PIPELINE_BIND_POINT_COMPUTE,
					rootsig_internal->pipelineLayout,
					0,
					(uint32_t)rootsig_internal->last_descriptorsets[cmd].size(),
					rootsig_internal->last_descriptorsets[cmd].data(),
					(uint32_t)rootsig_internal->root_offsets[cmd].size(),
					rootsig_internal->root_offsets[cmd].data()
				);
			}
		}

	}
	void GraphicsDevice_Vulkan::preraytrace(CommandList cmd)
	{
		if (active_rt[cmd]->desc.rootSignature == nullptr)
		{
			GetFrameResources().descriptors[cmd].validate(false, cmd, true);
		}
		else
		{
			auto rootsig_internal = to_internal(active_rt[cmd]->desc.rootSignature);
			if (rootsig_internal->dirty[cmd])
			{
				rootsig_internal->dirty[cmd] = false;
				vkCmdBindDescriptorSets(
					GetDirectCommandList(cmd),
					VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
					rootsig_internal->pipelineLayout,
					0,
					(uint32_t)rootsig_internal->last_descriptorsets[cmd].size(),
					rootsig_internal->last_descriptorsets[cmd].data(),
					(uint32_t)rootsig_internal->root_offsets[cmd].size(),
					rootsig_internal->root_offsets[cmd].data()
				);
			}
		}
	}

	// Engine functions
	GraphicsDevice_Vulkan::GraphicsDevice_Vulkan(wiPlatform::window_type window, bool fullscreen, bool debuglayer)
	{
		DESCRIPTOR_MANAGEMENT = true;
		TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = sizeof(VkAccelerationStructureInstanceKHR);

		DEBUGDEVICE = debuglayer;

		FULLSCREEN = fullscreen;

#ifdef _WIN32
		RECT rect = RECT();
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
		std::vector<VkExtensionProperties> extensions(extensionCount);
		res = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		assert(res == VK_SUCCESS);

		std::vector<const char*> extensionNames;
		extensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
		extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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

		bool enableValidationLayers = debuglayer;
		
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			wiHelper::messageBox("Vulkan validation layer requested but not available!");
			enableValidationLayers = false;
		}
		else if (enableValidationLayers)
		{
			extensionNames.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}

		// Create instance:
		{
			VkInstanceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pApplicationInfo = &appInfo;
			createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
			createInfo.ppEnabledExtensionNames = extensionNames.data();
			createInfo.enabledLayerCount = 0;
			if (enableValidationLayers)
			{
				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();
			}
			res = vkCreateInstance(&createInfo, nullptr, &instance);
			assert(res == VK_SUCCESS);
		}

		// Register validation layer callback:
		if (enableValidationLayers)
		{
			VkDebugReportCallbackCreateInfoEXT createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
			createInfo.pfnCallback = debugCallback;
			res = CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback);
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

			device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			device_properties_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
			device_properties_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
			raytracing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_KHR;
			mesh_shader_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;

			device_properties.pNext = &device_properties_1_1;
			device_properties_1_1.pNext = &device_properties_1_2;
			device_properties_1_2.pNext = &raytracing_properties;
			raytracing_properties.pNext = &mesh_shader_properties;

			for (const auto& device : devices) 
			{
				if (isDeviceSuitable(device, surface)) 
				{
					vkGetPhysicalDeviceProperties2(device, &device_properties);
					bool discrete = device_properties.properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
					if (discrete || physicalDevice == VK_NULL_HANDLE)
					{
						physicalDevice = device;
						if (discrete)
						{
							break; // if this is discrete GPU, look no further (prioritize discrete GPU)
						}
					}
				}
			}

			if (physicalDevice == VK_NULL_HANDLE) {
				wiHelper::messageBox("failed to find a suitable GPU!");
				assert(0);
			}

			queueIndices = findQueueFamilies(physicalDevice, surface);

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			std::set<int> uniqueQueueFamilies = { queueIndices.graphicsFamily, queueIndices.presentFamily, queueIndices.copyFamily };

			float queuePriority = 1.0f;
			for (int queueFamily : uniqueQueueFamilies) {
				VkDeviceQueueCreateInfo queueCreateInfo = {};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueFamily;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueCreateInfo);
			}

			assert(device_properties.properties.limits.timestampComputeAndGraphics == VK_TRUE);


			std::vector<const char*> enabled_deviceExtensions = required_deviceExtensions;
			res = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
			assert(res == VK_SUCCESS);
			std::vector<VkExtensionProperties> available_deviceExtensions(extensionCount);
			res = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, available_deviceExtensions.data());
			assert(res == VK_SUCCESS);

			if (checkDeviceExtensionSupport(VK_KHR_SPIRV_1_4_EXTENSION_NAME, available_deviceExtensions))
			{
				enabled_deviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
			}

			device_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			features_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
			features_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

			device_features2.pNext = &features_1_1;
			features_1_1.pNext = &features_1_2;

#ifdef ENABLE_RAYTRACING_EXTENSION
			raytracing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_FEATURES_KHR;
			if (checkDeviceExtensionSupport(VK_KHR_RAY_TRACING_EXTENSION_NAME, available_deviceExtensions))
			{
				SHADER_IDENTIFIER_SIZE = raytracing_properties.shaderGroupHandleSize;
				RAYTRACING = true;
				enabled_deviceExtensions.push_back(VK_KHR_RAY_TRACING_EXTENSION_NAME);
				enabled_deviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
				enabled_deviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
				enabled_deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
				features_1_2.pNext = &raytracing_features;
			}
#endif // ENABLE_RAYTRACING_EXTENSION

			mesh_shader_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
			if (checkDeviceExtensionSupport(VK_NV_MESH_SHADER_EXTENSION_NAME, available_deviceExtensions))
			{
				enabled_deviceExtensions.push_back(VK_NV_MESH_SHADER_EXTENSION_NAME);
				if (RAYTRACING)
				{
					raytracing_features.pNext = &mesh_shader_features;
				}
				else
				{
					features_1_2.pNext = &mesh_shader_features;
				}
			}

			vkGetPhysicalDeviceFeatures2(physicalDevice, &device_features2);

			assert(device_features2.features.imageCubeArray == VK_TRUE);
			assert(device_features2.features.independentBlend == VK_TRUE);
			assert(device_features2.features.geometryShader == VK_TRUE);
			assert(device_features2.features.samplerAnisotropy == VK_TRUE);
			assert(device_features2.features.shaderClipDistance == VK_TRUE);
			assert(device_features2.features.textureCompressionBC == VK_TRUE);
			assert(device_features2.features.occlusionQueryPrecise == VK_TRUE);
			TESSELLATION = device_features2.features.tessellationShader == VK_TRUE;
			UAV_LOAD_FORMAT_COMMON = device_features2.features.shaderStorageImageExtendedFormats == VK_TRUE;
			RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS = true; // let's hope for the best...

			if (RAYTRACING)
			{
				assert(features_1_2.bufferDeviceAddress == VK_TRUE);
			}

			if (mesh_shader_features.meshShader == VK_TRUE && mesh_shader_features.taskShader == VK_TRUE)
			{
				// Enable mesh shader here (problematic with certain driver versions, disabled by default): 
				//MESH_SHADER = true;
			}
			
			VkFormatProperties formatProperties = { 0 };
			vkGetPhysicalDeviceFormatProperties(physicalDevice, _ConvertFormat(FORMAT_R11G11B10_FLOAT), &formatProperties);
			UAV_LOAD_FORMAT_R11G11B10_FLOAT = formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;

			VkDeviceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

			createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			createInfo.pQueueCreateInfos = queueCreateInfos.data();

			createInfo.pEnabledFeatures = nullptr;
			createInfo.pNext = &device_features2;

			createInfo.enabledExtensionCount = static_cast<uint32_t>(enabled_deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = enabled_deviceExtensions.data();

			if (enableValidationLayers) {
				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();
			}
			else {
				createInfo.enabledLayerCount = 0;
			}

			res = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
			assert(res == VK_SUCCESS);

			vkGetDeviceQueue(device, queueIndices.graphicsFamily, 0, &graphicsQueue);
			vkGetDeviceQueue(device, queueIndices.presentFamily, 0, &presentQueue);
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

		// Extension functions:
		setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
		cmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT");
		cmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT");
		cmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdInsertDebugUtilsLabelEXT");

		if (RAYTRACING)
		{
			createRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");
			createAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
			bindAccelerationStructureMemoryKHR = (PFN_vkBindAccelerationStructureMemoryKHR)vkGetDeviceProcAddr(device, "vkBindAccelerationStructureMemoryKHR");
			destroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");
			getAccelerationStructureMemoryRequirementsKHR = (PFN_vkGetAccelerationStructureMemoryRequirementsKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureMemoryRequirementsKHR");
			getAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
			getRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR");
			cmdBuildAccelerationStructureKHR = (PFN_vkCmdBuildAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructureKHR");
			cmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");
		}

		if (MESH_SHADER)
		{
			cmdDrawMeshTasksNV = (PFN_vkCmdDrawMeshTasksNV)vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksNV");
			cmdDrawMeshTasksIndirectNV = (PFN_vkCmdDrawMeshTasksIndirectNV)vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectNV");
		}

		CreateBackBufferResources();

		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

		// Create frame resources:
		{
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
					poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
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
					vkGetDeviceQueue(device, queueIndices.copyFamily, 0, &frames[fr].copyQueue);

					VkCommandPoolCreateInfo poolInfo = {};
					poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
					poolInfo.queueFamilyIndex = queueFamilyIndices.copyFamily;
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
					beginInfo.pInheritanceInfo = nullptr; // Optional

					res = vkBeginCommandBuffer(frames[fr].copyCommandBuffer, &beginInfo);
					assert(res == VK_SUCCESS);
				}
			}
		}

		// Create semaphores:
		{
			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			res = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore);
			assert(res == VK_SUCCESS);
			res = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore);
			assert(res == VK_SUCCESS);
			res = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &copySema);
			assert(res == VK_SUCCESS);
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
					beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
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

		// GPU Queries:
		{
			timestamp_frequency = uint64_t(1.0 / double(device_properties.properties.limits.timestampPeriod) * 1000 * 1000 * 1000);

			VkQueryPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;

			for (uint32_t i = 0; i < timestamp_query_count; ++i)
			{
				allocationhandler->free_timestampqueries.push_back(i);
			}
			poolInfo.queryCount = timestamp_query_count;
			poolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
			res = vkCreateQueryPool(device, &poolInfo, nullptr, &querypool_timestamp);
			assert(res == VK_SUCCESS);
			timestamps_to_reset.reserve(timestamp_query_count);

			for (uint32_t i = 0; i < occlusion_query_count; ++i)
			{
				allocationhandler->free_occlusionqueries.push_back(i);
			}
			poolInfo.queryCount = occlusion_query_count;
			poolInfo.queryType = VK_QUERY_TYPE_OCCLUSION;
			res = vkCreateQueryPool(device, &poolInfo, nullptr, &querypool_occlusion);
			assert(res == VK_SUCCESS);
			occlusions_to_reset.reserve(occlusion_query_count);
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
		}

		vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(device, copySema, nullptr);

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

		vkDestroyQueryPool(device, querypool_timestamp, nullptr);
		vkDestroyQueryPool(device, querypool_occlusion, nullptr);

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

		DestroyDebugReportCallbackEXT(instance, callback, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
	}

	void GraphicsDevice_Vulkan::CreateBackBufferResources()
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

		VkSurfaceFormatKHR surfaceFormat = {};
		surfaceFormat.format = _ConvertFormat(BACKBUFFER_FORMAT);
		bool valid = false;

		for (const auto& format : swapChainSupport.formats)
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

		swapChainExtent = { static_cast<uint32_t>(RESOLUTIONWIDTH), static_cast<uint32_t>(RESOLUTIONHEIGHT) };
		swapChainExtent.width = std::max(swapChainSupport.capabilities.minImageExtent.width, std::min(swapChainSupport.capabilities.maxImageExtent.width, swapChainExtent.width));
		swapChainExtent.height = std::max(swapChainSupport.capabilities.minImageExtent.height, std::min(swapChainSupport.capabilities.maxImageExtent.height, swapChainExtent.height));


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

		uint32_t queueFamilyIndices[] = { (uint32_t)queueIndices.graphicsFamily, (uint32_t)queueIndices.presentFamily };

		if (queueIndices.graphicsFamily != queueIndices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // The only one that is always supported
		if (!VSYNC)
		{
			// The immediate present mode is not necessarily supported:
			for (auto& presentmode : swapChainSupport.presentModes)
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

		VkResult res = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain);
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

		VkDebugUtilsObjectNameInfoEXT info = {};
		info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		info.pObjectName = "SWAPCHAIN";
		info.objectType = VK_OBJECT_TYPE_IMAGE;
		for (auto& x : swapChainImages)
		{
			info.objectHandle = (uint64_t)x;

			res = setDebugUtilsObjectNameEXT(device, &info);
			assert(res == VK_SUCCESS);
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

	bool GraphicsDevice_Vulkan::CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *pBuffer)
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
			bufferInfo.usage |= VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR;
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

		res = vmaCreateBuffer(allocationhandler->allocator, &bufferInfo, &allocInfo, &internal_state->resource, &internal_state->allocation, nullptr);
		assert(res == VK_SUCCESS);

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
					beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
					beginInfo.pInheritanceInfo = nullptr; // Optional

					res = vkBeginCommandBuffer(frame.copyCommandBuffer, &beginInfo);
					assert(res == VK_SUCCESS);
				}

				VkBufferCopy copyRegion = {};
				copyRegion.size = pBuffer->desc.ByteWidth;
				copyRegion.srcOffset = 0;
				copyRegion.dstOffset = 0;

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


				vkCmdCopyBuffer(frame.copyCommandBuffer, upload_resource, internal_state->resource, 1, &copyRegion);


				VkAccessFlags tmp = barrier.srcAccessMask;
				barrier.srcAccessMask = barrier.dstAccessMask;
				barrier.dstAccessMask = 0;

				if (pBuffer->desc.BindFlags & BIND_CONSTANT_BUFFER)
				{
					barrier.dstAccessMask |= VK_ACCESS_UNIFORM_READ_BIT;
				}
				if (pBuffer->desc.BindFlags & BIND_VERTEX_BUFFER)
				{
					barrier.dstAccessMask |= VK_ACCESS_INDEX_READ_BIT;
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
				barrier.srcQueueFamilyIndex = queueIndices.copyFamily;
				barrier.dstQueueFamilyIndex = queueIndices.graphicsFamily;

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


		// Create resource views if needed
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
	bool GraphicsDevice_Vulkan::CreateTexture(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture *pTexture)
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
			for (uint32_t slice = 0; slice < pDesc->ArraySize; ++slice)
			{
				uint32_t width = pDesc->Width;
				uint32_t height = pDesc->Height;
				for (uint32_t mip = 0; mip < pDesc->MipLevels; ++mip)
				{
					const SubresourceData& subresourceData = pInitialData[initDataIdx++];
					size_t cpysize = subresourceData.SysMemPitch * height;
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
					copyRegion.imageSubresource.baseArrayLayer = slice;
					copyRegion.imageSubresource.layerCount = 1;

					copyRegion.imageOffset = { 0, 0, 0 };
					copyRegion.imageExtent = {
						width,
						height,
						1
					};

					width = std::max(1u, width / 2);
					height = std::max(1u, height / 2);

					copyRegions.push_back(copyRegion);

					cpyoffset += Align(cpysize, GetFormatStride(pDesc->Format));
				}
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
					beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
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
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

				frame.loadedimagetransitions.push_back(barrier);

			}
			copyQueueLock.unlock();
		}
		else
		{
			copyQueueLock.lock();

			auto& frame = GetFrameResources();
			if (!copyQueueUse)
			{
				copyQueueUse = true;

				res = vkResetCommandPool(device, frame.copyCommandPool, 0);
				assert(res == VK_SUCCESS);

				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
				beginInfo.pInheritanceInfo = nullptr; // Optional

				res = vkBeginCommandBuffer(frame.copyCommandBuffer, &beginInfo);
				assert(res == VK_SUCCESS);
			}

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = internal_state->resource;
			barrier.oldLayout = imageInfo.initialLayout;
			barrier.newLayout = _ConvertImageLayout(pTexture->desc.layout);
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
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
	bool GraphicsDevice_Vulkan::CreateInputLayout(const InputLayoutDesc *pInputElementDescs, uint32_t NumElements, const Shader* shader, InputLayout *pInputLayout)
	{
		pInputLayout->internal_state = allocationhandler;

		pInputLayout->desc.clear();
		pInputLayout->desc.reserve((size_t)NumElements);
		for (uint32_t i = 0; i < NumElements; ++i)
		{
			pInputLayout->desc.push_back(pInputElementDescs[i]);
		}

		return true;
	}
	bool GraphicsDevice_Vulkan::CreateShader(SHADERSTAGE stage, const void *pShaderBytecode, size_t BytecodeLength, Shader *pShader)
	{
		auto internal_state = std::make_shared<Shader_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		pShader->internal_state = internal_state;

		pShader->code.resize(BytecodeLength);
		std::memcpy(pShader->code.data(), pShaderBytecode, BytecodeLength);
		pShader->stage = stage;

		VkResult res = VK_SUCCESS;

		VkShaderModuleCreateInfo moduleInfo = {};
		moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleInfo.codeSize = pShader->code.size();
		moduleInfo.pCode = (uint32_t*)pShader->code.data();
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
			internal_state->stageInfo.stage = VK_SHADER_STAGE_ALL;
			// library shader (ray tracing)
			break;
		}

		if (pShader->rootSignature == nullptr)
		{
			// Perform shader reflection for shaders that don't specify a root signature:
			spirv_cross::Compiler comp((uint32_t*)pShader->code.data(), pShader->code.size() / sizeof(uint32_t));
			auto entrypoints = comp.get_entry_points_and_stages();
			auto active = comp.get_active_interface_variables();
			spirv_cross::ShaderResources resources = comp.get_shader_resources(active);
			comp.set_enabled_interface_variables(move(active));

			internal_state->entrypoints.reserve(entrypoints.size());
			for (auto& x : entrypoints)
			{
				internal_state->entrypoints.push_back(x);
			}

			std::vector<VkDescriptorSetLayoutBinding>& layoutBindings = internal_state->layoutBindings;
			std::vector<VkImageViewType>& imageViewTypes = internal_state->imageViewTypes;

			for (auto& x : resources.separate_samplers)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.stageFlags = internal_state->stageInfo.stage;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				layoutBinding.binding = comp.get_decoration(x.id, spv::Decoration::DecorationBinding);
				layoutBinding.descriptorCount = 1;
				layoutBindings.push_back(layoutBinding);
				imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_MAX_ENUM);
			}
			for (auto& x : resources.separate_images)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.stageFlags = internal_state->stageInfo.stage;
				auto image = comp.get_type_from_variable(x.id).image;
				switch (image.dim)
				{
				case spv::Dim1D:
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					if (image.arrayed)
					{
						imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_1D_ARRAY);
					}
					else
					{
						imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_1D);
					}
					break;
				case spv::Dim2D:
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					if (image.arrayed)
					{
						imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_2D_ARRAY);
					}
					else
					{
						imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_2D);
					}
					break;
				case spv::Dim3D:
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_3D);
					break;
				case spv::DimCube:
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					if (image.arrayed)
					{
						imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_CUBE_ARRAY);
					}
					else
					{
						imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_CUBE);
					}
					break;
				case spv::DimBuffer:
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
					imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_MAX_ENUM);
					break;
				default:
					imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_MAX_ENUM);
					break;
				}
				layoutBinding.binding = comp.get_decoration(x.id, spv::Decoration::DecorationBinding);
				layoutBinding.descriptorCount = 1;
				layoutBindings.push_back(layoutBinding);
			}
			for (auto& x : resources.storage_images)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.stageFlags = internal_state->stageInfo.stage;
				auto image = comp.get_type_from_variable(x.id).image;
				switch (image.dim)
				{
				case spv::Dim1D:
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					if (image.arrayed)
					{
						imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_1D_ARRAY);
					}
					else
					{
						imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_1D);
					}
					break;
				case spv::Dim2D:
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					if (image.arrayed)
					{
						imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_2D_ARRAY);
					}
					else
					{
						imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_2D);
					}
					break;
				case spv::Dim3D:
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_3D);
					break;
				case spv::DimCube:
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					if (image.arrayed)
					{
						imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_CUBE_ARRAY);
					}
					else
					{
						imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_CUBE);
					}
					break;
				case spv::DimBuffer:
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
					imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_MAX_ENUM);
					break;
				default:
					imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_MAX_ENUM);
					break;
				}
				layoutBinding.binding = comp.get_decoration(x.id, spv::Decoration::DecorationBinding);
				layoutBinding.descriptorCount = 1;
				layoutBindings.push_back(layoutBinding);
			}
			for (auto& x : resources.uniform_buffers)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.stageFlags = internal_state->stageInfo.stage;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				layoutBinding.binding = comp.get_decoration(x.id, spv::Decoration::DecorationBinding);
				layoutBinding.descriptorCount = 1;
				layoutBindings.push_back(layoutBinding);
				imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_MAX_ENUM);
			}
			for (auto& x : resources.storage_buffers)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.stageFlags = internal_state->stageInfo.stage;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				layoutBinding.binding = comp.get_decoration(x.id, spv::Decoration::DecorationBinding);
				layoutBinding.descriptorCount = 1;
				layoutBindings.push_back(layoutBinding);
				imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_MAX_ENUM);
			}
			for (auto& x : resources.acceleration_structures)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.stageFlags = internal_state->stageInfo.stage;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
				layoutBinding.binding = comp.get_decoration(x.id, spv::Decoration::DecorationBinding);
				layoutBinding.descriptorCount = 1;
				layoutBindings.push_back(layoutBinding);
				imageViewTypes.push_back(VK_IMAGE_VIEW_TYPE_MAX_ENUM);
			}

			if (stage == CS || stage == SHADERSTAGE_COUNT)
			{
				VkDescriptorSetLayoutCreateInfo descriptorSetlayoutInfo = {};
				descriptorSetlayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				descriptorSetlayoutInfo.pBindings = layoutBindings.data();
				descriptorSetlayoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
				res = vkCreateDescriptorSetLayout(device, &descriptorSetlayoutInfo, nullptr, &internal_state->descriptorSetLayout);
				assert(res == VK_SUCCESS);

				VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
				pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutInfo.pSetLayouts = &internal_state->descriptorSetLayout;
				pipelineLayoutInfo.setLayoutCount = 1; // cs
				pipelineLayoutInfo.pushConstantRangeCount = 0;

				res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &internal_state->pipelineLayout_cs);
				assert(res == VK_SUCCESS);
			}
		}

		if (stage == CS)
		{
			VkComputePipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			if (pShader->rootSignature == nullptr)
			{
				// pipeline layout from reflection:
				pipelineInfo.layout = internal_state->pipelineLayout_cs;
			}
			else
			{
				// pipeline layout from root signature:
				pipelineInfo.layout = to_internal(pShader->rootSignature)->pipelineLayout;
			}
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

			// Create compute pipeline state in place:
			pipelineInfo.stage = internal_state->stageInfo;


			res = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &internal_state->pipeline_cs);
			assert(res == VK_SUCCESS);
		}

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState)
	{
		pBlendState->internal_state = allocationhandler;

		pBlendState->desc = *pBlendStateDesc;
		return true;
	}
	bool GraphicsDevice_Vulkan::CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState)
	{
		pDepthStencilState->internal_state = allocationhandler;

		pDepthStencilState->desc = *pDepthStencilStateDesc;
		return true;
	}
	bool GraphicsDevice_Vulkan::CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState)
	{
		pRasterizerState->internal_state = allocationhandler;

		pRasterizerState->desc = *pRasterizerStateDesc;
		return true;
	}
	bool GraphicsDevice_Vulkan::CreateSampler(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState)
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

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery)
	{
		auto internal_state = std::make_shared<Query_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		pQuery->internal_state = internal_state;

		bool hr = false;

		pQuery->desc = *pDesc;
		internal_state->query_type = pQuery->desc.Type;

		switch (pDesc->Type)
		{
		case GPU_QUERY_TYPE_TIMESTAMP:
			if (allocationhandler->free_timestampqueries.pop_front(internal_state->query_index))
			{
				hr = true;
			}
			else
			{
				internal_state->query_type = GPU_QUERY_TYPE_INVALID;
				assert(0);
			}
			break;
		case GPU_QUERY_TYPE_TIMESTAMP_DISJOINT:
			hr = true;
			break;
		case GPU_QUERY_TYPE_OCCLUSION:
		case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
			if (allocationhandler->free_occlusionqueries.pop_front(internal_state->query_index))
			{
				hr = true;
			}
			else
			{
				internal_state->query_type = GPU_QUERY_TYPE_INVALID;
				assert(0);
			}
			break;
		}

		assert(hr);

		return hr;
	}
	bool GraphicsDevice_Vulkan::CreatePipelineState(const PipelineStateDesc* pDesc, PipelineState* pso)
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

		if (pDesc->rootSignature == nullptr)
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
			};

			insert_shader(pDesc->ms);
			insert_shader(pDesc->as);
			insert_shader(pDesc->vs);
			insert_shader(pDesc->hs);
			insert_shader(pDesc->ds);
			insert_shader(pDesc->gs);
			insert_shader(pDesc->ps);

			VkDescriptorSetLayoutCreateInfo descriptorSetlayoutInfo = {};
			descriptorSetlayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetlayoutInfo.pBindings = internal_state->layoutBindings.data();
			descriptorSetlayoutInfo.bindingCount = static_cast<uint32_t>(internal_state->layoutBindings.size());
			VkResult res = vkCreateDescriptorSetLayout(device, &descriptorSetlayoutInfo, nullptr, &internal_state->descriptorSetLayout);
			assert(res == VK_SUCCESS);

			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.pSetLayouts = &internal_state->descriptorSetLayout;
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pushConstantRangeCount = 0;

			res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &internal_state->pipelineLayout);
			assert(res == VK_SUCCESS);

			return res == VK_SUCCESS;
		}

		return true;
	}
	bool GraphicsDevice_Vulkan::CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass)
	{
		auto internal_state = std::make_shared<RenderPass_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		renderpass->internal_state = internal_state;

		renderpass->desc = *pDesc;

		renderpass->hash = 0;
		wiHelper::hash_combine(renderpass->hash, pDesc->attachments.size());
		for (auto& attachment : pDesc->attachments)
		{
			wiHelper::hash_combine(renderpass->hash, attachment.texture->desc.Format);
			wiHelper::hash_combine(renderpass->hash, attachment.texture->desc.SampleCount);
		}

		VkResult res;

		VkImageView attachments[17] = {};
		VkAttachmentDescription attachmentDescriptions[17] = {};
		VkAttachmentReference colorAttachmentRefs[8] = {};
		VkAttachmentReference resolveAttachmentRefs[8] = {};
		VkAttachmentReference depthAttachmentRef = {};

		int resolvecount = 0;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		const RenderPassDesc& desc = renderpass->desc;

		uint32_t validAttachmentCount = 0;
		for (auto& attachment : renderpass->desc.attachments)
		{
			const Texture* texture = attachment.texture;
			const TextureDesc& texdesc = texture->desc;
			int subresource = attachment.subresource;
			auto texture_internal_state = to_internal(texture);

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

				colorAttachmentRefs[subpass.colorAttachmentCount].attachment = validAttachmentCount;
				colorAttachmentRefs[subpass.colorAttachmentCount].layout = _ConvertImageLayout(attachment.subpass_layout);
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

				if (IsFormatStencilSupport(texdesc.Format))
				{
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

				depthAttachmentRef.attachment = validAttachmentCount;
				depthAttachmentRef.layout = _ConvertImageLayout(attachment.subpass_layout);
				subpass.pDepthStencilAttachment = &depthAttachmentRef;
			}
			else if (attachment.type == RenderPassAttachment::RESOLVE)
			{
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
				}

				resolvecount++;
				subpass.pResolveAttachments = resolveAttachmentRefs;
			}

			validAttachmentCount++;
		}
		assert(renderpass->desc.attachments.size() == validAttachmentCount);

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = validAttachmentCount;
		renderPassInfo.pAttachments = attachmentDescriptions;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		res = vkCreateRenderPass(device, &renderPassInfo, nullptr, &internal_state->renderpass);
		assert(res == VK_SUCCESS);

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = internal_state->renderpass;
		framebufferInfo.attachmentCount = validAttachmentCount;
		framebufferInfo.pAttachments = attachments;

		if (validAttachmentCount > 0)
		{
			const TextureDesc& texdesc = desc.attachments[0].texture->desc;
			framebufferInfo.width = texdesc.Width;
			framebufferInfo.height = texdesc.Height;
			framebufferInfo.layers = texdesc.MiscFlags & RESOURCE_MISC_TEXTURECUBE ? 6 : 1; // todo figure out better! can't use ArraySize here, it will crash!
		}
		else
		{
			framebufferInfo.width = 1;
			framebufferInfo.height = 1;
			framebufferInfo.layers = 1;
		}

		res = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &internal_state->framebuffer);
		assert(res == VK_SUCCESS);


		internal_state->beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		internal_state->beginInfo.renderPass = internal_state->renderpass;
		internal_state->beginInfo.framebuffer = internal_state->framebuffer;

		if (validAttachmentCount > 0)
		{
			const TextureDesc& texdesc = desc.attachments[0].texture->desc;

			internal_state->beginInfo.renderArea.offset = { 0, 0 };
			internal_state->beginInfo.renderArea.extent.width = texdesc.Width;
			internal_state->beginInfo.renderArea.extent.height = texdesc.Height;
			internal_state->beginInfo.clearValueCount = validAttachmentCount;
			internal_state->beginInfo.pClearValues = internal_state->clearColors;

			int i = 0;
			for (auto& attachment : desc.attachments)
			{
				if (desc.attachments[i].type == RenderPassAttachment::RESOLVE || attachment.texture == nullptr)
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
	bool GraphicsDevice_Vulkan::CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* pDesc, RaytracingAccelerationStructure* bvh)
	{
		auto internal_state = std::make_shared<BVH_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		bvh->internal_state = internal_state;
		bvh->type = GPUResource::GPU_RESOURCE_TYPE::RAYTRACING_ACCELERATION_STRUCTURE;

		bvh->desc = *pDesc;

		VkAccelerationStructureCreateInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;

		if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE)
		{
			info.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		}
		if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_COMPACTION)
		{
			info.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
		}
		if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_TRACE)
		{
			info.flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		}
		if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD)
		{
			info.flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
		}
		if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_MINIMIZE_MEMORY)
		{
			info.flags |= VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;
		}

		switch (pDesc->type)
		{
		case RaytracingAccelerationStructureDesc::BOTTOMLEVEL:
		{
			info.type = VkAccelerationStructureTypeKHR::VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

			for (auto& x : pDesc->bottomlevel.geometries)
			{
				internal_state->geometries.emplace_back();
				auto& geometry = internal_state->geometries.back();
				geometry = {};
				geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;

				if (x._flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
				{
					geometry.allowsTransforms = VK_TRUE;
				}

				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::TRIANGLES)
				{
					geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
					geometry.maxPrimitiveCount = x.triangles.indexCount / 3;
					geometry.indexType = x.triangles.indexFormat == INDEXFORMAT_16BIT ? VkIndexType::VK_INDEX_TYPE_UINT16 : VkIndexType::VK_INDEX_TYPE_UINT32;
					geometry.maxVertexCount = x.triangles.vertexCount;
					geometry.vertexFormat = _ConvertFormat(x.triangles.vertexFormat);
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::PROCEDURAL_AABBS)
				{
					geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
					geometry.maxPrimitiveCount = x.aabbs.count;
				}
			}


		}
		break;
		case RaytracingAccelerationStructureDesc::TOPLEVEL:
		{
			info.type = VkAccelerationStructureTypeKHR::VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

			internal_state->geometries.emplace_back();
			auto& geometry = internal_state->geometries.back();
			geometry = {};
			geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
			geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
			geometry.allowsTransforms = VK_TRUE;
			geometry.maxPrimitiveCount = pDesc->toplevel.count;
		}
		break;
		}

		info.pGeometryInfos = internal_state->geometries.data();
		info.maxGeometryCount = (uint32_t)internal_state->geometries.size();
		internal_state->info = info;

		VkResult res = createAccelerationStructureKHR(device, &info, nullptr, &internal_state->resource);
		assert(res == VK_SUCCESS);

		VkAccelerationStructureMemoryRequirementsInfoKHR meminfo = {};
		meminfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
		meminfo.accelerationStructure = internal_state->resource;

		meminfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR;
		VkMemoryRequirements2 memrequirements = {};
		memrequirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		getAccelerationStructureMemoryRequirementsKHR(device, &meminfo, &memrequirements);


		meminfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
		VkMemoryRequirements2 memrequirements_scratch_build = {};
		memrequirements_scratch_build.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		getAccelerationStructureMemoryRequirementsKHR(device, &meminfo, &memrequirements_scratch_build);

		meminfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_KHR;
		VkMemoryRequirements2 memrequirements_scratch_update = {};
		memrequirements_scratch_update.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		getAccelerationStructureMemoryRequirementsKHR(device, &meminfo, &memrequirements_scratch_update);


		// Main backing memory:
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = memrequirements.memoryRequirements.size + 
			std::max(memrequirements_scratch_build.memoryRequirements.size, memrequirements_scratch_update.memoryRequirements.size);
		bufferInfo.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		assert(features_1_2.bufferDeviceAddress == VK_TRUE);
		bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		bufferInfo.flags = 0;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

		res = vmaCreateBuffer(allocationhandler->allocator, &bufferInfo, &allocInfo, &internal_state->buffer, &internal_state->allocation, nullptr);
		assert(res == VK_SUCCESS);

		VkBindAccelerationStructureMemoryInfoKHR bind_info = {};
		bind_info.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR;
		bind_info.accelerationStructure = internal_state->resource;
		bind_info.memory = internal_state->allocation->GetMemory();
		res = bindAccelerationStructureMemoryKHR(device, 1, &bind_info);
		assert(res == VK_SUCCESS);

		VkAccelerationStructureDeviceAddressInfoKHR addrinfo = {};
		addrinfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		addrinfo.accelerationStructure = internal_state->resource;
		internal_state->as_address = getAccelerationStructureDeviceAddressKHR(device, &addrinfo);

		internal_state->scratch_offset = memrequirements.memoryRequirements.size;

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* pDesc, RaytracingPipelineState* rtpso)
	{
		auto internal_state = std::make_shared<RTPipelineState_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		rtpso->internal_state = internal_state;

		rtpso->desc = *pDesc;

		VkRayTracingPipelineCreateInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		info.flags = 0;

		info.libraries.sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;

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

		info.maxRecursionDepth = pDesc->max_trace_recursion_depth;

		if (pDesc->rootSignature == nullptr)
		{
			info.layout = to_internal(pDesc->shaderlibraries.front().shader)->pipelineLayout_cs; // think better way
		}
		else
		{
			info.layout = to_internal(pDesc->rootSignature)->pipelineLayout;
		}

		VkRayTracingPipelineInterfaceCreateInfoKHR library_interface = {};
		library_interface.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_INTERFACE_CREATE_INFO_KHR;
		library_interface.maxPayloadSize = pDesc->max_payload_size_in_bytes;
		library_interface.maxAttributeSize = pDesc->max_attribute_size_in_bytes;
		library_interface.maxCallableSize = 0;

		info.basePipelineHandle = VK_NULL_HANDLE;
		info.basePipelineIndex = 0;

		VkResult res = createRayTracingPipelinesKHR(device, VK_NULL_HANDLE, 1, &info, nullptr, &internal_state->pipeline);
		assert(res == VK_SUCCESS);

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateDescriptorTable(DescriptorTable* table)
	{
		auto internal_state = std::make_shared<DescriptorTable_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		table->internal_state = internal_state;

		std::vector<VkDescriptorSetLayoutBinding> bindings;
		bindings.reserve(table->samplers.size() + table->resources.size() + table->staticsamplers.size());

		std::vector<VkDescriptorUpdateTemplateEntry> entries;
		entries.reserve(table->samplers.size() + table->resources.size());

		internal_state->descriptors.reserve(table->samplers.size() + table->resources.size());

		size_t offset = 0;
		for (auto& x : table->resources)
		{
			bindings.emplace_back();
			auto& binding = bindings.back();
			binding = {};
			binding.stageFlags = _ConvertStageFlags(table->stage);
			binding.descriptorCount = x.count;

			switch (x.binding)
			{
			case ROOT_CONSTANTBUFFER:
				binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				binding.binding = x.slot + VULKAN_BINDING_SHIFT_B;
				break;
			case ROOT_RAWBUFFER:
			case ROOT_STRUCTUREDBUFFER:
			case ROOT_RWRAWBUFFER:
			case ROOT_RWSTRUCTUREDBUFFER:
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
				binding.binding = x.slot + VULKAN_BINDING_SHIFT_T;
				break;

			case CONSTANTBUFFER:
				binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				binding.binding = x.slot + VULKAN_BINDING_SHIFT_B;
				break;
			case RAWBUFFER:
			case STRUCTUREDBUFFER:
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				binding.binding = x.slot + VULKAN_BINDING_SHIFT_T;
				break;
			case TYPEDBUFFER:
				binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
				binding.binding = x.slot + VULKAN_BINDING_SHIFT_T;
				break;
			case TEXTURE1D:
			case TEXTURE1DARRAY:
			case TEXTURE2D:
			case TEXTURE2DARRAY:
			case TEXTURECUBE:
			case TEXTURECUBEARRAY:
			case TEXTURE3D:
				binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				binding.binding = x.slot + VULKAN_BINDING_SHIFT_T;
				break;
			case ACCELERATIONSTRUCTURE:
				binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
				binding.binding = x.slot + VULKAN_BINDING_SHIFT_T;
				break;
			case RWRAWBUFFER:
			case RWSTRUCTUREDBUFFER:
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				binding.binding = x.slot + VULKAN_BINDING_SHIFT_U;
				break;
			case RWTYPEDBUFFER:
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
				binding.binding = x.slot + VULKAN_BINDING_SHIFT_U;
				break;
			case RWTEXTURE1D:
			case RWTEXTURE1DARRAY:
			case RWTEXTURE2D:
			case RWTEXTURE2DARRAY:
			case RWTEXTURE3D:
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				binding.binding = x.slot + VULKAN_BINDING_SHIFT_U;
				break;
			default:
				assert(0);
				break;
			}

			// Unroll, because we need the ability to update an array element individually:
			internal_state->resource_write_remap.push_back(entries.size());
			for (uint32_t i = 0; i < binding.descriptorCount; ++i)
			{
				internal_state->descriptors.emplace_back();
				entries.emplace_back();
				auto& entry = entries.back();
				entry = {};
				entry.descriptorCount = 1;
				entry.descriptorType = binding.descriptorType;
				entry.dstArrayElement = i;
				entry.dstBinding = binding.binding;
				entry.offset = offset;
				entry.stride = sizeof(DescriptorTable_Vulkan::Descriptor);

				offset += entry.stride;
			}
		}
		for (auto& x : table->samplers)
		{
			internal_state->descriptors.emplace_back();
			bindings.emplace_back();
			auto& binding = bindings.back();
			binding = {};
			binding.stageFlags = _ConvertStageFlags(table->stage);
			binding.descriptorCount = x.count;
			binding.binding = x.slot + VULKAN_BINDING_SHIFT_S;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

			// Unroll, because we need the ability to update an array element individually:
			internal_state->sampler_write_remap.push_back(entries.size());
			for (uint32_t i = 0; i < binding.descriptorCount; ++i)
			{
				entries.emplace_back();
				auto& entry = entries.back();
				entry = {};
				entry.descriptorCount = 1;
				entry.descriptorType = binding.descriptorType;
				entry.dstArrayElement = i;
				entry.dstBinding = binding.binding;
				entry.offset = offset;
				entry.stride = sizeof(DescriptorTable_Vulkan::Descriptor);

				offset += entry.stride;
			}
		}

		std::vector<VkSampler> immutableSamplers;
		immutableSamplers.reserve(table->staticsamplers.size());

		for (auto& x : table->staticsamplers)
		{
			immutableSamplers.emplace_back();
			auto& immutablesampler = immutableSamplers.back();
			immutablesampler = to_internal(&x.sampler)->resource;

			bindings.emplace_back();
			auto& binding = bindings.back();
			binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			binding.stageFlags = VK_SHADER_STAGE_ALL;
			binding.binding = x.slot + VULKAN_BINDING_SHIFT_S;
			binding.descriptorCount = 1;
			binding.pImmutableSamplers = &immutablesampler;
		}

		VkDescriptorSetLayoutCreateInfo layoutinfo = {};
		layoutinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutinfo.flags = 0;
		layoutinfo.pBindings = bindings.data();
		layoutinfo.bindingCount = (uint32_t)bindings.size();
		VkResult res = vkCreateDescriptorSetLayout(device, &layoutinfo, nullptr, &internal_state->layout);
		assert(res == VK_SUCCESS);

		VkDescriptorUpdateTemplateCreateInfo updatetemplateinfo = {};
		updatetemplateinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO;
		updatetemplateinfo.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET;
		updatetemplateinfo.flags = 0;
		updatetemplateinfo.descriptorSetLayout = internal_state->layout;
		updatetemplateinfo.pDescriptorUpdateEntries = entries.data();
		updatetemplateinfo.descriptorUpdateEntryCount = (uint32_t)entries.size();
		res = vkCreateDescriptorUpdateTemplate(device, &updatetemplateinfo, nullptr, &internal_state->updatetemplate);
		assert(res == VK_SUCCESS);

		uint32_t slot = 0;
		for (auto& x : table->resources)
		{
			for (uint32_t i = 0; i < x.count; ++i)
			{
				WriteDescriptor(table, slot, i, (const GPUResource*)nullptr);
			}
			slot++;
		}
		slot = 0;
		for (auto& x : table->samplers)
		{
			for (uint32_t i = 0; i < x.count; ++i)
			{
				WriteDescriptor(table, slot, i, (const Sampler*)nullptr);
			}
			slot++;
		}

		return res == VK_SUCCESS;
	}
	bool GraphicsDevice_Vulkan::CreateRootSignature(RootSignature* rootsig)
	{
		auto internal_state = std::make_shared<RootSignature_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		rootsig->internal_state = internal_state;

		std::vector<VkDescriptorSetLayout> layouts;
		layouts.reserve(rootsig->tables.size());
		uint32_t space = 0;
		for (auto& x : rootsig->tables)
		{
			layouts.push_back(to_internal(&x)->layout);

			uint32_t rangeIndex = 0;
			for (auto& binding : x.resources)
			{
				if (binding.binding < CONSTANTBUFFER)
				{
					assert(binding.count == 1); // descriptor array not allowed in the root
					internal_state->root_remap.emplace_back();
					internal_state->root_remap.back().space = space;
					internal_state->root_remap.back().binding = binding.slot;
					internal_state->root_remap.back().rangeIndex = rangeIndex;
				}
				rangeIndex++;
			}
			space++;
		}

		for (CommandList cmd = 0; cmd < COMMANDLIST_COUNT; ++cmd)
		{
			internal_state->last_tables[cmd].resize(layouts.size());
			internal_state->last_descriptorsets[cmd].resize(layouts.size());

			for (auto& x : rootsig->tables)
			{
				for (auto& binding : x.resources)
				{
					if (binding.binding < CONSTANTBUFFER)
					{
						internal_state->root_descriptors[cmd].emplace_back();
						internal_state->root_offsets[cmd].emplace_back();
					}
				}
			}
		}

		std::vector<VkPushConstantRange> pushranges;
		pushranges.reserve(rootsig->rootconstants.size());
		for (auto& x : rootsig->rootconstants)
		{
			pushranges.emplace_back();
			pushranges.back() = {};
			pushranges.back().stageFlags = _ConvertStageFlags(x.stage);
			pushranges.back().offset = 0;
			pushranges.back().size = x.size;
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pSetLayouts = layouts.data();
		pipelineLayoutInfo.setLayoutCount = (uint32_t)layouts.size();
		pipelineLayoutInfo.pPushConstantRanges = pushranges.data();
		pipelineLayoutInfo.pushConstantRangeCount = (uint32_t)pushranges.size();

		VkResult res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &internal_state->pipelineLayout);
		assert(res == VK_SUCCESS);

		return res == VK_SUCCESS;
	}

	int GraphicsDevice_Vulkan::CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
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
					if (texture->desc.ArraySize > 6)
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

			if (res == VK_SUCCESS)
			{
				if (internal_state->srv == VK_NULL_HANDLE)
				{
					internal_state->srv = srv;
					return -1;
				}
				internal_state->subresources_srv.push_back(srv);
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
			VkImageView uav;
			VkResult res = vkCreateImageView(device, &view_desc, nullptr, &uav);

			if (res == VK_SUCCESS)
			{
				if (internal_state->uav == VK_NULL_HANDLE)
				{
					internal_state->uav = uav;
					return -1;
				}
				internal_state->subresources_uav.push_back(uav);
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
					return -1;
				}
				internal_state->subresources_rtv.push_back(rtv);
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
					return -1;
				}
				internal_state->subresources_dsv.push_back(dsv);
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
	int GraphicsDevice_Vulkan::CreateSubresource(GPUBuffer* buffer, SUBRESOURCE_TYPE type, uint64_t offset, uint64_t size)
	{
		auto internal_state = to_internal(buffer);
		const GPUBufferDesc& desc = buffer->GetDesc();
		VkResult res;

		switch (type)
		{
		case wiGraphics::SRV:
		case wiGraphics::UAV:
		{
			if (desc.Format == FORMAT_UNKNOWN)
			{
				return -1;
			}

			VkBufferViewCreateInfo srv_desc = {};
			srv_desc.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
			srv_desc.buffer = internal_state->resource;
			srv_desc.flags = 0;
			srv_desc.format = _ConvertFormat(desc.Format);
			srv_desc.offset = Align(offset, device_properties.properties.limits.minTexelBufferOffsetAlignment); // damn, if this needs alignment, that could break a lot of things! (index buffer, index offset?)
			srv_desc.range = std::min(size, (uint64_t)desc.ByteWidth - srv_desc.offset);

			VkBufferView view;
			res = vkCreateBufferView(device, &srv_desc, nullptr, &view);

			if (res == VK_SUCCESS)
			{
				if (type == SRV)
				{
					if (internal_state->srv == VK_NULL_HANDLE)
					{
						internal_state->srv = view;
						return -1;
					}
					internal_state->subresources_srv.push_back(view);
					return int(internal_state->subresources_srv.size() - 1);
				}
				else
				{
					if (internal_state->uav == VK_NULL_HANDLE)
					{
						internal_state->uav = view;
						return -1;
					}
					internal_state->subresources_uav.push_back(view);
					return int(internal_state->subresources_uav.size() - 1);
				}
			}
			else
			{
				assert(0);
			}
		}
		break;
		default:
			assert(0);
			break;
		}
		return -1;
	}

	void GraphicsDevice_Vulkan::WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest)
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
	void GraphicsDevice_Vulkan::WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest)
	{
		VkResult res = getRayTracingShaderGroupHandlesKHR(device, to_internal(rtpso)->pipeline, group_index, 1, SHADER_IDENTIFIER_SIZE, dest);
		assert(res == VK_SUCCESS);
	}
	void GraphicsDevice_Vulkan::WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const GPUResource* resource, int subresource, uint64_t offset)
	{
		auto table_internal = to_internal(table);
		size_t remap = table_internal->resource_write_remap[rangeIndex];
		auto& descriptor = table_internal->descriptors[remap + arrayIndex];

		switch (table->resources[rangeIndex].binding)
		{
		case CONSTANTBUFFER:
		case RAWBUFFER:
		case STRUCTUREDBUFFER:
		case ROOT_CONSTANTBUFFER:
		case ROOT_RAWBUFFER:
		case ROOT_STRUCTUREDBUFFER:
			if (resource == nullptr || !resource->IsValid())
			{
				descriptor.bufferInfo.buffer = nullBuffer;
				descriptor.bufferInfo.offset = 0;
				descriptor.bufferInfo.range = VK_WHOLE_SIZE;
			}
			else if (resource->IsBuffer())
			{
				const GPUBuffer* buffer = (const GPUBuffer*)resource;
				auto internal_state = to_internal(buffer);
				descriptor.bufferInfo.buffer = internal_state->resource;
				descriptor.bufferInfo.offset = offset;
				descriptor.bufferInfo.range = VK_WHOLE_SIZE;
			}
			else
			{
				assert(0);
			}
			break;
		case TYPEDBUFFER:
			if (resource == nullptr || !resource->IsValid())
			{
				descriptor.bufferView = nullBufferView;
			}
			else if (resource->IsBuffer())
			{
				const GPUBuffer* buffer = (const GPUBuffer*)resource;
				auto internal_state = to_internal(buffer);
				descriptor.bufferView = subresource < 0 ? internal_state->srv : internal_state->subresources_srv[subresource];
			}
			else
			{
				assert(0);
			}
			break;
		case TEXTURE1D:
		case TEXTURE1DARRAY:
		case TEXTURE2D:
		case TEXTURE2DARRAY:
		case TEXTURECUBE:
		case TEXTURECUBEARRAY:
		case TEXTURE3D:
			descriptor.imageinfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			descriptor.imageinfo.sampler = VK_NULL_HANDLE;
			if (resource == nullptr || !resource->IsValid())
			{
				switch (table->resources[rangeIndex].binding)
				{
				case TEXTURE1D:
					descriptor.imageinfo.imageView = nullImageView1D;
					break;
				case TEXTURE1DARRAY:
					descriptor.imageinfo.imageView = nullImageView1DArray;
					break;
				case TEXTURE2D:
					descriptor.imageinfo.imageView = nullImageView2D;
					break;
				case TEXTURE2DARRAY:
					descriptor.imageinfo.imageView = nullImageView2DArray;
					break;
				case TEXTURECUBE:
					descriptor.imageinfo.imageView = nullImageViewCube;
					break;
				case TEXTURECUBEARRAY:
					descriptor.imageinfo.imageView = nullImageViewCubeArray;
					break;
				case TEXTURE3D:
					descriptor.imageinfo.imageView = nullImageView3D;
					break;
				};
			}
			else if (resource->IsTexture())
			{
				const Texture* texture = (const Texture*)resource;
				auto internal_state = to_internal(texture);
				if (subresource < 0)
				{
					descriptor.imageinfo.imageView = internal_state->srv;
				}
				else
				{
					descriptor.imageinfo.imageView = internal_state->subresources_srv[subresource];
				}
				VkImageLayout layout = _ConvertImageLayout(texture->desc.layout);
				if (layout != VK_IMAGE_LAYOUT_GENERAL && layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				{
					// Means texture initial layout is not compatible, so it must have been transitioned
					layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				}
				descriptor.imageinfo.imageLayout = layout;
			}
			else
			{
				assert(0);
			}
			break;
		case ACCELERATIONSTRUCTURE:
			if (resource == nullptr || !resource->IsValid())
			{
				// nothing
			}
			else if (resource->IsAccelerationStructure())
			{
				auto internal_state = to_internal((const RaytracingAccelerationStructure*)resource);
				descriptor.accelerationStructure = internal_state->resource;
			}
			else
			{
				assert(0);
			}
			break;
		case RWRAWBUFFER:
		case RWSTRUCTUREDBUFFER:
		case ROOT_RWRAWBUFFER:
		case ROOT_RWSTRUCTUREDBUFFER:
			if (resource == nullptr || !resource->IsValid())
			{
				descriptor.bufferInfo.buffer = nullBuffer;
				descriptor.bufferInfo.offset = 0;
				descriptor.bufferInfo.range = VK_WHOLE_SIZE;
			}
			else if (resource->IsBuffer())
			{
				const GPUBuffer* buffer = (const GPUBuffer*)resource;
				auto internal_state = to_internal(buffer);
				descriptor.bufferInfo.buffer = internal_state->resource;
				descriptor.bufferInfo.offset = offset;
				descriptor.bufferInfo.range = VK_WHOLE_SIZE;
			}
			else
			{
				assert(0);
			}
			break;
		case RWTYPEDBUFFER:
			if (resource == nullptr || !resource->IsValid())
			{
				descriptor.bufferView = nullBufferView;
			}
			else if (resource->IsBuffer())
			{
				const GPUBuffer* buffer = (const GPUBuffer*)resource;
				auto internal_state = to_internal(buffer);
				descriptor.bufferView = subresource < 0 ? internal_state->uav : internal_state->subresources_uav[subresource];
			}
			else
			{
				assert(0);
			}
			break;
		case RWTEXTURE1D:
		case RWTEXTURE1DARRAY:
		case RWTEXTURE2D:
		case RWTEXTURE2DARRAY:
		case RWTEXTURE3D:
			descriptor.imageinfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			descriptor.imageinfo.sampler = VK_NULL_HANDLE;
			if (resource == nullptr || !resource->IsValid())
			{
				switch (table->resources[rangeIndex].binding)
				{
				case TEXTURE1D:
					descriptor.imageinfo.imageView = nullImageView1D;
					break;
				case TEXTURE1DARRAY:
					descriptor.imageinfo.imageView = nullImageView1DArray;
					break;
				case TEXTURE2D:
					descriptor.imageinfo.imageView = nullImageView2D;
					break;
				case TEXTURE2DARRAY:
					descriptor.imageinfo.imageView = nullImageView2DArray;
					break;
				case TEXTURECUBE:
					descriptor.imageinfo.imageView = nullImageViewCube;
					break;
				case TEXTURECUBEARRAY:
					descriptor.imageinfo.imageView = nullImageViewCubeArray;
					break;
				case TEXTURE3D:
					descriptor.imageinfo.imageView = nullImageView3D;
					break;
				};
			}
			else if (resource->IsTexture())
			{
				const Texture* texture = (const Texture*)resource;
				auto internal_state = to_internal(texture);
				if (subresource < 0)
				{
					descriptor.imageinfo.imageView = internal_state->uav;
				}
				else
				{
					descriptor.imageinfo.imageView = internal_state->subresources_uav[subresource];
				}
			}
			else
			{
				assert(0);
			}
			break;
		default:
			break;
		}
	}
	void GraphicsDevice_Vulkan::WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const Sampler* sampler)
	{
		auto table_internal = to_internal(table);
		size_t sampler_remap = table->resources.size() + (size_t)rangeIndex;
		size_t remap = table_internal->sampler_write_remap[rangeIndex];
		auto& descriptor = table_internal->descriptors[remap];
		descriptor.imageinfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		descriptor.imageinfo.imageView = VK_NULL_HANDLE;

		if (sampler == nullptr || !sampler->IsValid())
		{
			descriptor.imageinfo.sampler = nullSampler;
		}
		else
		{
			auto internal_state = to_internal(sampler);
			descriptor.imageinfo.sampler = internal_state->resource;
		}
	}

	void GraphicsDevice_Vulkan::Map(const GPUResource* resource, Mapping* mapping)
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
	void GraphicsDevice_Vulkan::Unmap(const GPUResource* resource)
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
	bool GraphicsDevice_Vulkan::QueryRead(const GPUQuery* query, GPUQueryResult* result)
	{
		auto internal_state = to_internal(query);

		VkResult res = VK_SUCCESS;

		switch (query->desc.Type)
		{
		case GPU_QUERY_TYPE_EVENT:
			assert(0); // not implemented yet
			break;
		case GPU_QUERY_TYPE_TIMESTAMP:
			res = vkGetQueryPoolResults(device, querypool_timestamp, (uint32_t)internal_state->query_index, 1, sizeof(uint64_t),
				&result->result_timestamp, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
			if (timestamps_to_reset.empty() || timestamps_to_reset.back() != (uint32_t)internal_state->query_index)
			{
				timestamps_to_reset.push_back((uint32_t)internal_state->query_index);
			}
			break;
		case GPU_QUERY_TYPE_TIMESTAMP_DISJOINT:
			result->result_timestamp_frequency = timestamp_frequency;
			break;
		case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
		case GPU_QUERY_TYPE_OCCLUSION:
			res = vkGetQueryPoolResults(device, querypool_occlusion, (uint32_t)internal_state->query_index, 1, sizeof(uint64_t),
				&result->result_passed_sample_count, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_PARTIAL_BIT);
			if (occlusions_to_reset.empty() || occlusions_to_reset.back() != (uint32_t)internal_state->query_index)
			{
				occlusions_to_reset.push_back((uint32_t)internal_state->query_index);
			}
			break;
		}

		return res == VK_SUCCESS;
	}

	void GraphicsDevice_Vulkan::SetName(GPUResource* pResource, const char* name)
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

		VkResult res = setDebugUtilsObjectNameEXT(device, &info);
		assert(res == VK_SUCCESS);
	}

	void GraphicsDevice_Vulkan::PresentBegin(CommandList cmd)
	{
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkResult res = vkAcquireNextImageKHR(device, swapChain, 0xFFFFFFFFFFFFFFFF, imageAvailableSemaphore, VK_NULL_HANDLE, &swapChainImageIndex);
		assert(res == VK_SUCCESS);

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

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		presentInfo.waitSemaphoreCount = arraysize(signalSemaphores);
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &swapChainImageIndex;
		presentInfo.pResults = nullptr; // Optional

		VkResult res = vkQueuePresentKHR(presentQueue, &presentInfo);
		assert(res == VK_SUCCESS);
	}

	CommandList GraphicsDevice_Vulkan::BeginCommandList()
	{
		VkResult res;

		CommandList cmd = cmd_count.fetch_add(1);
		if (GetDirectCommandList(cmd) == VK_NULL_HANDLE)
		{
			// need to create one more command list:
			assert(cmd < COMMANDLIST_COUNT);

			QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

			for (auto& frame : frames)
			{
				VkCommandPoolCreateInfo poolInfo = {};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
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
		res = vkResetCommandPool(device, GetFrameResources().commandPools[cmd], 0);
		assert(res == VK_SUCCESS);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		res = vkBeginCommandBuffer(GetFrameResources().commandBuffers[cmd], &beginInfo);
		assert(res == VK_SUCCESS);

		VkViewport viewports[6];
		for (uint32_t i = 0; i < arraysize(viewports); ++i)
		{
			viewports[i].x = 0;
			viewports[i].y = 0;
			viewports[i].width = (float)RESOLUTIONWIDTH;
			viewports[i].height = (float)RESOLUTIONHEIGHT;
			viewports[i].minDepth = 0;
			viewports[i].maxDepth = 1;
		}
		vkCmdSetViewport(GetDirectCommandList(cmd), 0, arraysize(viewports), viewports);

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

		// reset descriptor allocators:
		GetFrameResources().descriptors[cmd].reset();

		// reset immediate resource allocators:
		GetFrameResources().resourceBuffer[cmd].clear();

		if (!initial_querypool_reset)
		{
			initial_querypool_reset = true;
			vkCmdResetQueryPool(GetDirectCommandList(cmd), querypool_timestamp, 0, timestamp_query_count);
			vkCmdResetQueryPool(GetDirectCommandList(cmd), querypool_occlusion, 0, occlusion_query_count);
		}
		for (auto& x : timestamps_to_reset)
		{
			vkCmdResetQueryPool(GetDirectCommandList(cmd), querypool_timestamp, x, 1);
		}
		timestamps_to_reset.clear();
		for (auto& x : occlusions_to_reset)
		{
			vkCmdResetQueryPool(GetDirectCommandList(cmd), querypool_occlusion, x, 1);
		}
		occlusions_to_reset.clear();

		prev_pipeline_hash[cmd] = 0;
		active_pso[cmd] = nullptr;
		active_cs[cmd] = nullptr;
		active_rt[cmd] = nullptr;
		active_renderpass[cmd] = VK_NULL_HANDLE;
		dirty_pso[cmd] = false;

		return cmd;
	}
	void GraphicsDevice_Vulkan::SubmitCommandLists()
	{
		// Sync up copy queue and transitions:
		copyQueueLock.lock();
		if (copyQueueUse)
		{
			auto& frame = GetFrameResources();

			// Copies:
			{
				VkResult res = vkEndCommandBuffer(frame.copyCommandBuffer);
				assert(res == VK_SUCCESS);

				VkSemaphore semaphores[] = { copySema };

				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &frame.copyCommandBuffer;
				submitInfo.pSignalSemaphores = semaphores;
				submitInfo.signalSemaphoreCount = arraysize(semaphores);

				res = vkQueueSubmit(frame.copyQueue, 1, &submitInfo, VK_NULL_HANDLE);
				assert(res == VK_SUCCESS);

			}

			// Transitions:
			{
				for (auto& barrier : frame.loadedimagetransitions)
				{
					vkCmdPipelineBarrier(
						frame.transitionCommandBuffer,
						VK_PIPELINE_STAGE_TRANSFER_BIT,
						VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
						0,
						0, nullptr,
						0, nullptr,
						1, &barrier
					);
				}
				frame.loadedimagetransitions.clear();

				VkResult res = vkEndCommandBuffer(frame.transitionCommandBuffer);
				assert(res == VK_SUCCESS);

				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &frame.transitionCommandBuffer;

				res = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
				assert(res == VK_SUCCESS);
			}
		}

		// Execute deferred command lists:
		{
			auto& frame = GetFrameResources();

			VkCommandBuffer cmdLists[COMMANDLIST_COUNT];
			CommandList cmds[COMMANDLIST_COUNT];
			uint32_t counter = 0;

			CommandList cmd_last = cmd_count.load();
			cmd_count.store(0);
			for (CommandList cmd = 0; cmd < cmd_last; ++cmd)
			{
				VkResult res = vkEndCommandBuffer(GetDirectCommandList(cmd));
				assert(res == VK_SUCCESS);

				cmdLists[counter] = GetDirectCommandList(cmd);
				cmds[counter] = cmd;
				counter++;

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

			VkSemaphore waitSemaphores[] = { imageAvailableSemaphore, copySema };
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

			VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
			submitInfo.signalSemaphoreCount = arraysize(signalSemaphores);
			submitInfo.pSignalSemaphores = signalSemaphores;

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
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional

			res = vkBeginCommandBuffer(frame.transitionCommandBuffer, &beginInfo);
			assert(res == VK_SUCCESS);
		}

		copyQueueUse = false;
		copyQueueLock.unlock();
	}

	void GraphicsDevice_Vulkan::WaitForGPU()
	{
		VkResult res = vkQueueWaitIdle(graphicsQueue);
		assert(res == VK_SUCCESS);
	}
	void GraphicsDevice_Vulkan::ClearPipelineStateCache()
	{
		allocationhandler->destroylocker.lock();
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
			viewports[i].y = pViewports[i].TopLeftY;
			viewports[i].width = pViewports[i].Width;
			viewports[i].height = pViewports[i].Height;
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
		VkDeviceSize voffsets[8] = {};
		VkBuffer vbuffers[8] = {};
		assert(count <= 8);
		for (uint32_t i = 0; i < count; ++i)
		{
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

		vkCmdBindVertexBuffers(GetDirectCommandList(cmd), static_cast<uint32_t>(slot), static_cast<uint32_t>(count), vbuffers, voffsets);
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

		GetFrameResources().descriptors[cmd].dirty = true;
		active_pso[cmd] = pso;
		dirty_pso[cmd] = true;
	}
	void GraphicsDevice_Vulkan::BindComputeShader(const Shader* cs, CommandList cmd)
	{
		assert(cs->stage == CS);
		if (active_cs[cmd] != cs)
		{
			GetFrameResources().descriptors[cmd].dirty = true;
			active_cs[cmd] = cs;
			auto internal_state = to_internal(cs);
			vkCmdBindPipeline(GetDirectCommandList(cmd), VK_PIPELINE_BIND_POINT_COMPUTE, internal_state->pipeline_cs);
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
		cmdDrawMeshTasksNV(GetDirectCommandList(cmd), threadGroupCountX * threadGroupCountY * threadGroupCountZ, 0);
	}
	void GraphicsDevice_Vulkan::DispatchMeshIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		cmdDrawMeshTasksIndirectNV(GetDirectCommandList(cmd), internal_state->resource, (VkDeviceSize)args_offset,1,sizeof(IndirectDispatchArgs));
	}
	void GraphicsDevice_Vulkan::CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd)
	{
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

			VkPipelineStageFlags stages = 0;

			VkBufferMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			barrier.buffer = internal_state->resource;
			barrier.srcAccessMask = 0;
			if (buffer->desc.BindFlags & BIND_CONSTANT_BUFFER)
			{
				barrier.srcAccessMask |= VK_ACCESS_UNIFORM_READ_BIT;
				stages = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			}
			if (buffer->desc.BindFlags & BIND_VERTEX_BUFFER)
			{
				barrier.srcAccessMask |= VK_ACCESS_INDEX_READ_BIT;
				stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			}
			if (buffer->desc.BindFlags & BIND_INDEX_BUFFER)
			{
				barrier.srcAccessMask |= VK_ACCESS_INDEX_READ_BIT;
				stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			}
			if (buffer->desc.BindFlags & BIND_SHADER_RESOURCE)
			{
				barrier.srcAccessMask |= VK_ACCESS_SHADER_READ_BIT;
				stages = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			}
			if (buffer->desc.BindFlags & BIND_UNORDERED_ACCESS)
			{
				barrier.srcAccessMask |= VK_ACCESS_SHADER_WRITE_BIT;
				stages = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			}
			if (buffer->desc.MiscFlags & RESOURCE_MISC_RAY_TRACING)
			{
				barrier.srcAccessMask |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
				stages = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
			}
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.size = VK_WHOLE_SIZE;

			vkCmdPipelineBarrier(
				GetDirectCommandList(cmd),
				stages,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				1, &barrier,
				0, nullptr
			);


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

			vkCmdPipelineBarrier(
				GetDirectCommandList(cmd),
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				stages,
				0,
				0, nullptr,
				1, &barrier,
				0, nullptr
			);

		}

	}
	void GraphicsDevice_Vulkan::QueryBegin(const GPUQuery *query, CommandList cmd)
	{
		auto internal_state = to_internal(query);

		switch (query->desc.Type)
		{
		case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
			vkCmdBeginQuery(GetDirectCommandList(cmd), querypool_occlusion, (uint32_t)internal_state->query_index, 0);
			break;
		case GPU_QUERY_TYPE_OCCLUSION:
			vkCmdBeginQuery(GetDirectCommandList(cmd), querypool_occlusion, (uint32_t)internal_state->query_index, VK_QUERY_CONTROL_PRECISE_BIT);
			break;
		}
	}
	void GraphicsDevice_Vulkan::QueryEnd(const GPUQuery *query, CommandList cmd)
	{
		auto internal_state = to_internal(query);

		switch (query->desc.Type)
		{
		case GPU_QUERY_TYPE_TIMESTAMP:
			vkCmdWriteTimestamp(GetDirectCommandList(cmd), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, querypool_timestamp, internal_state->query_index);
			break;
		case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
			vkCmdEndQuery(GetDirectCommandList(cmd), querypool_occlusion, internal_state->query_index);
			break;
		case GPU_QUERY_TYPE_OCCLUSION:
			vkCmdEndQuery(GetDirectCommandList(cmd), querypool_occlusion, internal_state->query_index);
			break;
		}
	}
	void GraphicsDevice_Vulkan::Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd)
	{
		VkMemoryBarrier memorybarriers[8];
		VkImageMemoryBarrier imagebarriers[8];
		VkBufferMemoryBarrier bufferbarriers[8];
		uint32_t memorybarrier_count = 0;
		uint32_t imagebarrier_count = 0;
		uint32_t bufferbarrier_count = 0;

		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const GPUBarrier& barrier = barriers[i];

			switch (barrier.type)
			{
			default:
			case GPUBarrier::MEMORY_BARRIER:
			{
				VkMemoryBarrier& barrierdesc = memorybarriers[memorybarrier_count++];
				barrierdesc.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				barrierdesc.pNext = nullptr;
				barrierdesc.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
				barrierdesc.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
				if (RAYTRACING)
				{
					barrierdesc.srcAccessMask |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
					barrierdesc.dstAccessMask |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
				}
			}
			break;
			case GPUBarrier::IMAGE_BARRIER:
			{
				const TextureDesc& desc = barrier.image.texture->GetDesc();
				auto internal_state = to_internal(barrier.image.texture);

				VkImageMemoryBarrier& barrierdesc = imagebarriers[imagebarrier_count++];
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
				barrierdesc.subresourceRange.baseArrayLayer = 0;
				barrierdesc.subresourceRange.layerCount = desc.ArraySize;
				barrierdesc.subresourceRange.baseMipLevel = 0;
				barrierdesc.subresourceRange.levelCount = desc.MipLevels;
				barrierdesc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrierdesc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			}
			break;
			case GPUBarrier::BUFFER_BARRIER:
			{
				auto internal_state = to_internal(barrier.buffer.buffer);

				VkBufferMemoryBarrier& barrierdesc = bufferbarriers[bufferbarrier_count++];
				barrierdesc.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				barrierdesc.pNext = nullptr;
				barrierdesc.buffer = internal_state->resource;
				barrierdesc.size = barrier.buffer.buffer->GetDesc().ByteWidth;
				barrierdesc.offset = 0;
				barrierdesc.srcAccessMask = _ParseBufferState(barrier.buffer.state_before);
				barrierdesc.dstAccessMask = _ParseBufferState(barrier.buffer.state_after);
				barrierdesc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrierdesc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			}
			break;
			}
		}

		VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		if (RAYTRACING)
		{
			srcStage |= VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
			dstStage |= VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
		}

		vkCmdPipelineBarrier(GetDirectCommandList(cmd),
			srcStage,
			dstStage,
			0,
			memorybarrier_count, memorybarriers,
			bufferbarrier_count, bufferbarriers,
			imagebarrier_count, imagebarriers
		);
	}
	void GraphicsDevice_Vulkan::BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src)
	{
		auto dst_internal = to_internal(dst);

		VkAccelerationStructureBuildGeometryInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		info.flags = dst_internal->info.flags;
		info.dstAccelerationStructure = dst_internal->resource;
		info.srcAccelerationStructure = VK_NULL_HANDLE;
		info.update = VK_FALSE;
		info.geometryArrayOfPointers = VK_FALSE;

		VkBufferDeviceAddressInfo addressinfo = {};
		addressinfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		addressinfo.buffer = dst_internal->buffer;
		info.scratchData.deviceAddress = vkGetBufferDeviceAddress(device, &addressinfo) + dst_internal->scratch_offset;

		if (src != nullptr)
		{
			info.update = VK_TRUE;

			auto src_internal = to_internal(src);
			info.srcAccelerationStructure = src_internal->resource;
		}

		std::vector<VkAccelerationStructureGeometryKHR> geometries;
		std::vector<VkAccelerationStructureBuildOffsetInfoKHR> offsetinfos;

		switch (dst->desc.type)
		{
		case RaytracingAccelerationStructureDesc::BOTTOMLEVEL:
		{
			info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			info.geometryCount = (uint32_t)dst->desc.bottomlevel.geometries.size();
			geometries.reserve(info.geometryCount);
			offsetinfos.reserve(info.geometryCount);

			size_t i = 0;
			for (auto& x : dst->desc.bottomlevel.geometries)
			{
				geometries.emplace_back();
				offsetinfos.emplace_back();

				auto& geometry = geometries.back();
				geometry = {};
				geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;

				auto& offset = offsetinfos.back();
				offset = {};

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
					geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
					geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
					geometry.geometry.triangles.vertexStride = x.triangles.vertexStride;
					geometry.geometry.triangles.vertexFormat = _ConvertFormat(x.triangles.vertexFormat);
					geometry.geometry.triangles.indexType = x.triangles.indexFormat == INDEXFORMAT_16BIT ? VkIndexType::VK_INDEX_TYPE_UINT16 : VkIndexType::VK_INDEX_TYPE_UINT32;

					addressinfo.buffer = to_internal(&x.triangles.vertexBuffer)->resource;
					geometry.geometry.triangles.vertexData.deviceAddress = vkGetBufferDeviceAddress(device, &addressinfo) + 
						x.triangles.vertexByteOffset;

					addressinfo.buffer = to_internal(&x.triangles.indexBuffer)->resource;
					geometry.geometry.triangles.indexData.deviceAddress = vkGetBufferDeviceAddress(device, &addressinfo) +
						x.triangles.indexOffset * (x.triangles.indexFormat == INDEXBUFFER_FORMAT::INDEXFORMAT_16BIT ? sizeof(uint16_t) : sizeof(uint32_t));

					if (x._flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
					{
						addressinfo.buffer = to_internal(&x.triangles.transform3x4Buffer)->resource;
						geometry.geometry.triangles.transformData.deviceAddress = vkGetBufferDeviceAddress(device, &addressinfo);
						offset.transformOffset = x.triangles.transform3x4BufferOffset;
					}

					offset.primitiveCount = x.triangles.indexCount / 3;
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::PROCEDURAL_AABBS)
				{
					geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
					geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
					geometry.geometry.aabbs.stride = x.aabbs.stride;

					addressinfo.buffer = to_internal(&x.aabbs.aabbBuffer)->resource;
					geometry.geometry.aabbs.data.deviceAddress = vkGetBufferDeviceAddress(device, &addressinfo);

					offset.primitiveCount = x.aabbs.offset;
					offset.primitiveOffset = x.aabbs.offset;
				}
			}
		}
		break;
		case RaytracingAccelerationStructureDesc::TOPLEVEL:
		{
			info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
			info.geometryCount = 1;
			geometries.reserve(info.geometryCount);
			offsetinfos.reserve(info.geometryCount);

			geometries.emplace_back();
			offsetinfos.emplace_back();

			auto& geometry = geometries.back();
			geometry = {};
			geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
			geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
			geometry.geometry.instances.arrayOfPointers = VK_FALSE;

			addressinfo.buffer = to_internal(&dst->desc.toplevel.instanceBuffer)->resource;
			geometry.geometry.instances.data.deviceAddress = vkGetBufferDeviceAddress(device, &addressinfo);

			auto& offset = offsetinfos.back();
			offset = {};
			offset.primitiveCount = dst->desc.toplevel.count;
			offset.primitiveOffset = dst->desc.toplevel.offset;
		}
		break;
		}

		VkAccelerationStructureGeometryKHR* pGeomtries = geometries.data();
		info.ppGeometries = &pGeomtries;

		VkAccelerationStructureBuildOffsetInfoKHR* pOffsetinfo = offsetinfos.data();

		cmdBuildAccelerationStructureKHR(GetDirectCommandList(cmd), 1, &info, &pOffsetinfo);
	}
	void GraphicsDevice_Vulkan::BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd)
	{
		prev_pipeline_hash[cmd] = 0;
		GetFrameResources().descriptors[cmd].dirty = true;
		active_cs[cmd] = rtpso->desc.shaderlibraries.front().shader; // we just take the first shader (todo: better)
		active_rt[cmd] = rtpso;

		vkCmdBindPipeline(GetDirectCommandList(cmd), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, to_internal(rtpso)->pipeline);
	}
	void GraphicsDevice_Vulkan::DispatchRays(const DispatchRaysDesc* desc, CommandList cmd)
	{
		preraytrace(cmd);

		VkStridedBufferRegionKHR raygen = {};
		raygen.buffer = desc->raygeneration.buffer ? to_internal(desc->raygeneration.buffer)->resource : VK_NULL_HANDLE;
		raygen.offset = desc->raygeneration.offset;
		raygen.size = desc->raygeneration.size;
		raygen.stride = desc->raygeneration.stride;

		VkStridedBufferRegionKHR miss = {};
		miss.buffer = desc->miss.buffer ? to_internal(desc->miss.buffer)->resource : VK_NULL_HANDLE;
		miss.offset = desc->miss.offset;
		miss.size = desc->miss.size;
		miss.stride = desc->miss.stride;

		VkStridedBufferRegionKHR hitgroup = {};
		hitgroup.buffer = desc->hitgroup.buffer ? to_internal(desc->hitgroup.buffer)->resource : VK_NULL_HANDLE;
		hitgroup.offset = desc->hitgroup.offset;
		hitgroup.size = desc->hitgroup.size;
		hitgroup.stride = desc->hitgroup.stride;

		VkStridedBufferRegionKHR callable = {};
		callable.buffer = desc->callable.buffer ? to_internal(desc->callable.buffer)->resource : VK_NULL_HANDLE;
		callable.offset = desc->callable.offset;
		callable.size = desc->callable.size;
		callable.stride = desc->callable.stride;

		cmdTraceRaysKHR(
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

	void GraphicsDevice_Vulkan::BindDescriptorTable(BINDPOINT bindpoint, uint32_t space, const DescriptorTable* table, CommandList cmd)
	{
		const RootSignature* rootsig = nullptr;
		switch (bindpoint)
		{
		default:
		case wiGraphics::GRAPHICS:
			rootsig = active_pso[cmd]->desc.rootSignature;
			break;
		case wiGraphics::COMPUTE:
			rootsig = active_cs[cmd]->rootSignature;
			break;
		case wiGraphics::RAYTRACING:
			rootsig = active_rt[cmd]->desc.rootSignature;
			break;
		}
		auto rootsig_internal = to_internal(rootsig);
		auto& descriptors = GetFrameResources().descriptors[cmd];
		rootsig_internal->last_tables[cmd][space] = table;
		rootsig_internal->last_descriptorsets[cmd][space] = descriptors.commit(table);
		rootsig_internal->dirty[cmd] = true;
		for (auto& x : rootsig_internal->root_descriptors[cmd])
		{
			x = nullptr;
		}
	}
	void GraphicsDevice_Vulkan::BindRootDescriptor(BINDPOINT bindpoint, uint32_t index, const GPUBuffer* buffer, uint32_t offset, CommandList cmd)
	{
		const RootSignature* rootsig = nullptr;
		switch (bindpoint)
		{
		default:
		case wiGraphics::GRAPHICS:
			rootsig = active_pso[cmd]->desc.rootSignature;
			break;
		case wiGraphics::COMPUTE:
			rootsig = active_cs[cmd]->rootSignature;
			break;
		case wiGraphics::RAYTRACING:
			rootsig = active_rt[cmd]->desc.rootSignature;
			break;
		}
		auto rootsig_internal = to_internal(rootsig);
		rootsig_internal->root_offsets[cmd][index] = offset;

		if (buffer != rootsig_internal->root_descriptors[cmd][index])
		{
			rootsig_internal->root_descriptors[cmd][index] = buffer;

			auto remap = rootsig_internal->root_remap[index];

			if (!rootsig_internal->dirty[cmd])
			{
				// Need to recommit descriptor set if root descriptor changes and the set is already bound:
				auto& descriptors = GetFrameResources().descriptors[cmd];
				VkDescriptorSet set = descriptors.commit(rootsig_internal->last_tables[cmd][remap.space]);
				rootsig_internal->last_descriptorsets[cmd][remap.space] = set;
			}

			// Then write root descriptor on top:
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = to_internal(buffer)->resource;
			bufferInfo.offset = 0;

			VkWriteDescriptorSet write = {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = rootsig_internal->last_descriptorsets[cmd][remap.space];
			write.dstBinding = remap.binding;
			write.dstArrayElement = 0;
			write.descriptorCount = 1;
			write.pBufferInfo = &bufferInfo;

			switch (rootsig_internal->last_tables[cmd][remap.space]->resources[remap.rangeIndex].binding)
			{
			case ROOT_CONSTANTBUFFER:
				bufferInfo.range = std::min(buffer->desc.ByteWidth, device_properties.properties.limits.maxUniformBufferRange);
				write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				break;
			case ROOT_RAWBUFFER:
			case ROOT_STRUCTUREDBUFFER:
			case ROOT_RWRAWBUFFER:
			case ROOT_RWSTRUCTUREDBUFFER:
				bufferInfo.range = VK_WHOLE_SIZE;
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
				break;
			default:
				assert(0); // this function is only usable for root buffers!
				break;
			}

			vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
		}

		rootsig_internal->dirty[cmd] = true;
	}
	void GraphicsDevice_Vulkan::BindRootConstants(BINDPOINT bindpoint, uint32_t index, const void* srcdata, CommandList cmd)
	{
		const RootSignature* rootsig = nullptr;
		switch (bindpoint)
		{
		default:
		case wiGraphics::GRAPHICS:
			rootsig = active_pso[cmd]->desc.rootSignature;
			break;
		case wiGraphics::COMPUTE:
			rootsig = active_cs[cmd]->rootSignature;
			break;
		case wiGraphics::RAYTRACING:
			rootsig = active_rt[cmd]->desc.rootSignature;
			break;
		}
		auto rootsig_internal = to_internal(rootsig);

		const RootConstantRange& range = rootsig->rootconstants[index];
		vkCmdPushConstants(
			GetDirectCommandList(cmd),
			rootsig_internal->pipelineLayout,
			_ConvertStageFlags(range.stage),
			range.offset,
			range.size,
			srcdata
		);
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
		if (cmdBeginDebugUtilsLabelEXT != nullptr)
		{
			VkDebugUtilsLabelEXT label = {};
			label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			label.pLabelName = name;
			label.color[0] = 0;
			label.color[1] = 0;
			label.color[2] = 0;
			label.color[3] = 1;
			cmdBeginDebugUtilsLabelEXT(GetDirectCommandList(cmd), &label);
		}
	}
	void GraphicsDevice_Vulkan::EventEnd(CommandList cmd)
	{
		if (cmdEndDebugUtilsLabelEXT != nullptr)
		{
			cmdEndDebugUtilsLabelEXT(GetDirectCommandList(cmd));
		}
	}
	void GraphicsDevice_Vulkan::SetMarker(const char* name, CommandList cmd)
	{
		if (cmdInsertDebugUtilsLabelEXT != nullptr)
		{
			VkDebugUtilsLabelEXT label = {};
			label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			label.pLabelName = name;
			label.color[0] = 0;
			label.color[1] = 0;
			label.color[2] = 0;
			label.color[3] = 1;
			cmdInsertDebugUtilsLabelEXT(GetDirectCommandList(cmd), &label);
		}
	}

}


#endif // WICKEDENGINE_BUILD_VULKAN
