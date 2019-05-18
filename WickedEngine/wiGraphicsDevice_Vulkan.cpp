#include "wiGraphicsDevice_Vulkan.h"
#include "wiGraphicsDevice_SharedInternals.h"
#include "wiHelper.h"
#include "ShaderInterop_Vulkan.h"
#include "wiBackLog.h"

#include <sstream>
#include <vector>
#include <cstring>
#include <iostream>
#include <set>
#include <algorithm>


#ifdef WICKEDENGINE_BUILD_VULKAN
#pragma comment(lib,"vulkan-1.lib")

namespace wiGraphics
{



	// Converters:
	inline VkFormat _ConvertFormat(FORMAT value)
	{
		// _TYPELESS is converted to _UINT or _FLOAT or _UNORM in that order depending on availability!
		// X channel is converted to regular missing channel (eg. FORMAT_B8G8R8X8_UNORM -> VK_FORMAT_B8G8R8A8_UNORM)
		switch (value)
		{
		case FORMAT_UNKNOWN:
			return VK_FORMAT_UNDEFINED;
			break;
		case FORMAT_R32G32B32A32_TYPELESS:
			return VK_FORMAT_R32G32B32A32_UINT;
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
		case FORMAT_R32G32B32_TYPELESS:
			return VK_FORMAT_R32G32B32_UINT;
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
		case FORMAT_R16G16B16A16_TYPELESS:
			return VK_FORMAT_R16G16B16A16_UINT;
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
		case FORMAT_R32G32_TYPELESS:
			return VK_FORMAT_R32G32_UINT;
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
			return VK_FORMAT_D32_SFLOAT_S8_UINT; // possible mismatch!
			break;
		case FORMAT_D32_FLOAT_S8X24_UINT:
			return VK_FORMAT_D32_SFLOAT_S8_UINT;
			break;
		case FORMAT_R32_FLOAT_X8X24_TYPELESS:
			return VK_FORMAT_R32G32_SFLOAT; // possible mismatch!
			break;
		case FORMAT_X32_TYPELESS_G8X24_UINT:
			return VK_FORMAT_R32G32_UINT; // possible mismatch!
			break;
		case FORMAT_R10G10B10A2_TYPELESS:
			return VK_FORMAT_A2B10G10R10_UINT_PACK32;
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
		case FORMAT_R8G8B8A8_TYPELESS:
			return VK_FORMAT_R8G8B8A8_UINT;
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
		case FORMAT_R16G16_TYPELESS:
			return VK_FORMAT_R16G16_UINT;
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
		case FORMAT_R24_UNORM_X8_TYPELESS:
			return VK_FORMAT_D24_UNORM_S8_UINT;
			break;
		case FORMAT_X24_TYPELESS_G8_UINT:
			return VK_FORMAT_D24_UNORM_S8_UINT;
			break;
		case FORMAT_R8G8_TYPELESS:
			return VK_FORMAT_R8G8_UINT;
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
		case FORMAT_R8_TYPELESS:
			return VK_FORMAT_R8_UINT;
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
		case FORMAT_A8_UNORM:
			return VK_FORMAT_R8_UNORM; // mismatch!
			break;
		case FORMAT_R1_UNORM:
			return VK_FORMAT_R8_UNORM; // mismatch!
			break;
		case FORMAT_R9G9B9E5_SHAREDEXP:
			return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32; // maybe ok
			break;
		case FORMAT_R8G8_B8G8_UNORM:
			return VK_FORMAT_R8G8B8A8_UNORM; // mismatch
			break;
		case FORMAT_G8R8_G8B8_UNORM:
			return VK_FORMAT_R8G8B8A8_UNORM; // mismatch
			break;
		case FORMAT_BC1_TYPELESS:
			return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			break;
		case FORMAT_BC1_UNORM:
			return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			break;
		case FORMAT_BC1_UNORM_SRGB:
			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
			break;
		case FORMAT_BC2_TYPELESS:
			return VK_FORMAT_BC2_UNORM_BLOCK;
			break;
		case FORMAT_BC2_UNORM:
			return VK_FORMAT_BC2_UNORM_BLOCK;
			break;
		case FORMAT_BC2_UNORM_SRGB:
			return VK_FORMAT_BC2_SRGB_BLOCK;
			break;
		case FORMAT_BC3_TYPELESS:
			return VK_FORMAT_BC3_UNORM_BLOCK;
			break;
		case FORMAT_BC3_UNORM:
			return VK_FORMAT_BC3_UNORM_BLOCK;
			break;
		case FORMAT_BC3_UNORM_SRGB:
			return VK_FORMAT_BC3_SRGB_BLOCK;
			break;
		case FORMAT_BC4_TYPELESS:
			return VK_FORMAT_BC4_UNORM_BLOCK;
			break;
		case FORMAT_BC4_UNORM:
			return VK_FORMAT_BC4_UNORM_BLOCK;
			break;
		case FORMAT_BC4_SNORM:
			return VK_FORMAT_BC4_SNORM_BLOCK;
			break;
		case FORMAT_BC5_TYPELESS:
			return VK_FORMAT_BC5_UNORM_BLOCK;
			break;
		case FORMAT_BC5_UNORM:
			return VK_FORMAT_BC5_UNORM_BLOCK;
			break;
		case FORMAT_BC5_SNORM:
			return VK_FORMAT_BC5_SNORM_BLOCK;
			break;
		case FORMAT_B5G6R5_UNORM:
			return VK_FORMAT_B5G6R5_UNORM_PACK16;
			break;
		case FORMAT_B5G5R5A1_UNORM:
			return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
			break;
		case FORMAT_B8G8R8A8_UNORM:
			return VK_FORMAT_B8G8R8A8_UNORM;
			break;
		case FORMAT_B8G8R8X8_UNORM:
			return VK_FORMAT_B8G8R8A8_UNORM;
			break;
		case FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
			return VK_FORMAT_B10G11R11_UFLOAT_PACK32; // mismatch
			break;
		case FORMAT_B8G8R8A8_TYPELESS:
			return VK_FORMAT_B8G8R8A8_UINT;
			break;
		case FORMAT_B8G8R8A8_UNORM_SRGB:
			return VK_FORMAT_B8G8R8A8_SRGB;
			break;
		case FORMAT_B8G8R8X8_TYPELESS:
			return VK_FORMAT_B8G8R8A8_UINT;
			break;
		case FORMAT_B8G8R8X8_UNORM_SRGB:
			return VK_FORMAT_B8G8R8A8_SRGB;
			break;
		case FORMAT_BC6H_TYPELESS:
			return VK_FORMAT_BC6H_SFLOAT_BLOCK;
			break;
		case FORMAT_BC6H_UF16:
			return VK_FORMAT_BC6H_UFLOAT_BLOCK;
			break;
		case FORMAT_BC6H_SF16:
			return VK_FORMAT_BC6H_SFLOAT_BLOCK;
			break;
		case FORMAT_BC7_TYPELESS:
			return VK_FORMAT_BC7_UNORM_BLOCK; // maybe mismatch
			break;
		case FORMAT_BC7_UNORM:
			return VK_FORMAT_BC7_UNORM_BLOCK;
			break;
		case FORMAT_BC7_UNORM_SRGB:
			return VK_FORMAT_BC7_SRGB_BLOCK;
			break;
		case FORMAT_B4G4R4A4_UNORM:
			return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
			break;
		default:
			break;
		}
		return VK_FORMAT_UNDEFINED;
	}
	inline VkCompareOp _ConvertComparisonFunc(COMPARISON_FUNC value)
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
	inline VkBlendFactor _ConvertBlend(BLEND value)
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
	inline VkBlendOp _ConvertBlendOp(BLEND_OP value)
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
	inline VkSamplerAddressMode _ConvertTextureAddressMode(TEXTURE_ADDRESS_MODE value)
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
		case TEXTURE_ADDRESS_MIRROR_ONCE:
			return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
			break;
		default:
			break;
		}
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}


	// Validation layer helpers:
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};
	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

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

		std::cerr << ss.str();
		OutputDebugStringA(ss.str().c_str());

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
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (queueFamily.queueCount > 0 && presentSupport) {
				indices.presentFamily = i;
			}

			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			if (queueFamily.queueCount > 0 && queueFamily.queueFlags == VK_QUEUE_TRANSFER_BIT) {
				indices.copyFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}

	// Swapchain helpers:
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
			return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
		VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
			else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
				bestMode = availablePresentMode;
			}
		}

		return bestMode;
	}

	uint32_t findMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	// Device selection helpers:
	bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
		QueueFamilyIndices indices = findQueueFamilies(device, surface);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.isComplete() && extensionsSupported && swapChainAdequate;
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




	GraphicsDevice_Vulkan::FrameResources::ResourceFrameAllocator::ResourceFrameAllocator(VkPhysicalDevice physicalDevice, VkDevice device, size_t size) : device(device)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		bufferInfo.flags = 0;

		VkResult res = vkCreateBuffer(device, &bufferInfo, nullptr, (VkBuffer*)&buffer.resource);
		assert(res == VK_SUCCESS);


		// Allocate resource backing memory:
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, (VkBuffer)buffer.resource, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, (VkDeviceMemory*)&buffer.resourceMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate staging memory!");
		}

		res = vkBindBufferMemory(device, (VkBuffer)buffer.resource, (VkDeviceMemory)buffer.resourceMemory, 0);
		assert(res == VK_SUCCESS);




		void* pData;
		//
		// No CPU reads will be done from the resource.
		//
		vkMapMemory(device, (VkDeviceMemory)buffer.resourceMemory, 0, bufferInfo.size, 0, &pData);
		dataCur = dataBegin = reinterpret_cast<uint8_t*>(pData);
		dataEnd = dataBegin + size;

		// Because the "buffer" is created by hand in this, fill the desc to indicate how it can be used:
		buffer.desc.ByteWidth = (UINT)((size_t)dataEnd - (size_t)dataBegin);
		buffer.desc.Usage = USAGE_DYNAMIC;
		buffer.desc.BindFlags = BIND_VERTEX_BUFFER | BIND_INDEX_BUFFER | BIND_SHADER_RESOURCE;
		buffer.desc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	}
	GraphicsDevice_Vulkan::FrameResources::ResourceFrameAllocator::~ResourceFrameAllocator()
	{
		vkDestroyBuffer(device, (VkBuffer)buffer.resource, nullptr);
	}
	uint8_t* GraphicsDevice_Vulkan::FrameResources::ResourceFrameAllocator::allocate(size_t dataSize, size_t alignment)
	{
		dataCur = reinterpret_cast<uint8_t*>(Align(reinterpret_cast<size_t>(dataCur), alignment));

		if (dataCur + dataSize > dataEnd)
		{
			return nullptr; // failed allocation. TODO: create new heap chunk and allocate from that
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



	GraphicsDevice_Vulkan::UploadBuffer::UploadBuffer(VkPhysicalDevice physicalDevice, VkDevice device, const QueueFamilyIndices& queueIndices, size_t size) : device(device)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.flags = 0;


		// Allow access from copy queue:
		bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

		uint32_t queueFamilyIndices[] = {
			static_cast<uint32_t>(queueIndices.graphicsFamily),
			static_cast<uint32_t>(queueIndices.copyFamily)
		};
		bufferInfo.pQueueFamilyIndices = queueFamilyIndices;
		bufferInfo.queueFamilyIndexCount = ARRAYSIZE(queueFamilyIndices);


		VkResult res = vkCreateBuffer(device, &bufferInfo, nullptr, &resource);
		assert(res == VK_SUCCESS);


		// Allocate resource backing memory:
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, resource, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &resourceMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate staging memory!");
		}

		res = vkBindBufferMemory(device, resource, resourceMemory, 0);
		assert(res == VK_SUCCESS);




		void* pData;
		//
		// No CPU reads will be done from the resource.
		//
		vkMapMemory(device, resourceMemory, 0, bufferInfo.size, 0, &pData);
		dataCur = dataBegin = reinterpret_cast< UINT8* >(pData);
		dataEnd = dataBegin + size;
	}
	GraphicsDevice_Vulkan::UploadBuffer::~UploadBuffer()
	{
		vkDestroyBuffer(device, resource, nullptr);
	}
	uint8_t* GraphicsDevice_Vulkan::UploadBuffer::allocate(size_t dataSize, size_t alignment)
	{
		lock.lock();

		//dataCur = reinterpret_cast<uint8_t*>(Align(reinterpret_cast<size_t>(dataCur), alignment));

		dataSize = Align(dataSize, alignment);
		assert(dataCur + dataSize <= dataEnd);

		uint8_t* retVal = dataCur;

		dataCur += dataSize;

		lock.unlock();

		return retVal;
	}
	void GraphicsDevice_Vulkan::UploadBuffer::clear()
	{
		lock.lock();
		dataCur = dataBegin;
		lock.unlock();
	}
	uint64_t GraphicsDevice_Vulkan::UploadBuffer::calculateOffset(uint8_t* address)
	{
		assert(address >= dataBegin && address < dataEnd);
		return static_cast<uint64_t>(address - dataBegin);
	}




	GraphicsDevice_Vulkan::FrameResources::DescriptorTableFrameAllocator::DescriptorTableFrameAllocator(GraphicsDevice_Vulkan* device, UINT maxRenameCount) : device(device)
	{
		// Create descriptor pool:
		{
			uint32_t numTables = SHADERSTAGE_COUNT * (maxRenameCount + 1); // (gpu * maxRenameCount) + (1 * cpu staging table)

			VkDescriptorPoolSize tableLayout[8] = {};

			tableLayout[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			tableLayout[0].descriptorCount = GPU_RESOURCE_HEAP_CBV_COUNT;

			tableLayout[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			tableLayout[1].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT;

			tableLayout[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
			tableLayout[2].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT;

			tableLayout[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			tableLayout[3].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT;

			tableLayout[4].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			tableLayout[4].descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT;

			tableLayout[5].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
			tableLayout[5].descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT;

			tableLayout[6].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			tableLayout[6].descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT;

			tableLayout[7].type = VK_DESCRIPTOR_TYPE_SAMPLER;
			tableLayout[7].descriptorCount = GPU_SAMPLER_HEAP_COUNT;


			std::vector<VkDescriptorPoolSize> poolSizes;
			poolSizes.reserve(ARRAYSIZE(tableLayout) * numTables);
			for (uint32_t i = 0; i < numTables; ++i)
			{
				for (int j = 0; j < ARRAYSIZE(tableLayout); ++j)
				{
					poolSizes.push_back(tableLayout[j]);
				}
			}


			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			poolInfo.pPoolSizes = poolSizes.data();
			poolInfo.maxSets = numTables;
			//poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

			if (vkCreateDescriptorPool(device->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
				throw std::runtime_error("failed to create descriptor pool!");
			}

		}


		// Create staging descriptor table:
		{
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = descriptorPool;
			allocInfo.descriptorSetCount = SHADERSTAGE_COUNT;
			allocInfo.pSetLayouts = device->defaultDescriptorSetlayouts;

			if (vkAllocateDescriptorSets(device->device, &allocInfo, descriptorSet_CPU) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate descriptor set!");
			}
		}

		// Create GPU-visible descriptor tables:
		{
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = descriptorPool;
			allocInfo.descriptorSetCount = 1;


			for (int stage = 0; stage < SHADERSTAGE_COUNT; ++stage)
			{
				allocInfo.pSetLayouts = &device->defaultDescriptorSetlayouts[stage];
				descriptorSet_GPU[stage].resize(SHADERSTAGE_COUNT * maxRenameCount);

				for (uint32_t i = 0; i < maxRenameCount; ++i)
				{
					if (vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet_GPU[stage][i]) != VK_SUCCESS) {
						throw std::runtime_error("failed to allocate descriptor set!");
					}
				}
			}

		}


		// Preload default descriptor tables:

		for (int i = 0; i < ARRAYSIZE(bufferInfo); ++i)
		{
			bufferInfo[i].buffer = device->nullBuffer;
			bufferInfo[i].offset = 0;
			bufferInfo[i].range = VK_WHOLE_SIZE;
		}

		for (int i = 0; i < ARRAYSIZE(imageInfo); ++i)
		{
			imageInfo[i].imageView = device->nullImageView;
			imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		}

		for (int i = 0; i < ARRAYSIZE(bufferViews); ++i)
		{
			bufferViews[i] = device->nullBufferView;
		}

		for (int i = 0; i < ARRAYSIZE(samplerInfo); ++i)
		{
			samplerInfo[i].imageView = VK_NULL_HANDLE;
			samplerInfo[i].sampler = device->nullSampler;
		}


		for (int stage = 0; stage < SHADERSTAGE_COUNT; ++stage)
		{
			int offset = 0;

			// CBV:
			{
				assert(offset == VULKAN_DESCRIPTOR_SET_OFFSET_CBV);

				VkWriteDescriptorSet writeDescriptors = {};
				writeDescriptors.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptors.dstSet = descriptorSet_CPU[stage];
				writeDescriptors.dstArrayElement = 0;
				writeDescriptors.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptors.dstBinding = offset;
				writeDescriptors.descriptorCount = GPU_RESOURCE_HEAP_CBV_COUNT;
				writeDescriptors.pBufferInfo = bufferInfo;
				writeDescriptors.pImageInfo = nullptr;
				writeDescriptors.pTexelBufferView = nullptr;
				initWrites[stage].push_back(writeDescriptors);

				offset += writeDescriptors.descriptorCount;

			}

			// SRV - Texture:
			{
				assert(offset == VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TEXTURE);

				VkWriteDescriptorSet writeDescriptors = {};
				writeDescriptors.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptors.dstSet = descriptorSet_CPU[stage];
				writeDescriptors.dstArrayElement = 0;
				writeDescriptors.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				writeDescriptors.dstBinding = offset;
				writeDescriptors.descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT;
				writeDescriptors.pBufferInfo = nullptr;
				writeDescriptors.pImageInfo = imageInfo;
				writeDescriptors.pTexelBufferView = nullptr;
				initWrites[stage].push_back(writeDescriptors);

				offset += writeDescriptors.descriptorCount;

			}
			// SRV - Typed Buffer:
			{
				assert(offset == VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TYPEDBUFFER);

				VkWriteDescriptorSet writeDescriptors = {};
				writeDescriptors.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptors.dstSet = descriptorSet_CPU[stage];
				writeDescriptors.dstArrayElement = 0;
				writeDescriptors.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
				writeDescriptors.dstBinding = offset;
				writeDescriptors.descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT;
				writeDescriptors.pBufferInfo = nullptr;
				writeDescriptors.pImageInfo = nullptr;
				writeDescriptors.pTexelBufferView = bufferViews;
				initWrites[stage].push_back(writeDescriptors);

				offset += writeDescriptors.descriptorCount;

			}
			// SRV - Untyped Buffer:
			{
				assert(offset == VULKAN_DESCRIPTOR_SET_OFFSET_SRV_UNTYPEDBUFFER);

				VkWriteDescriptorSet writeDescriptors = {};
				writeDescriptors.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptors.dstSet = descriptorSet_CPU[stage];
				writeDescriptors.dstArrayElement = 0;
				writeDescriptors.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				writeDescriptors.dstBinding = offset;
				writeDescriptors.descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT;
				writeDescriptors.pBufferInfo = bufferInfo;
				writeDescriptors.pImageInfo = nullptr;
				writeDescriptors.pTexelBufferView = nullptr;
				initWrites[stage].push_back(writeDescriptors);

				offset += writeDescriptors.descriptorCount;

			}

			// UAV - Texture:
			{
				assert(offset == VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TEXTURE);

				VkWriteDescriptorSet writeDescriptors = {};
				writeDescriptors.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptors.dstSet = descriptorSet_CPU[stage];
				writeDescriptors.dstArrayElement = 0;
				writeDescriptors.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				writeDescriptors.dstBinding = offset;
				writeDescriptors.descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT;
				writeDescriptors.pBufferInfo = nullptr;
				writeDescriptors.pImageInfo = imageInfo;
				writeDescriptors.pTexelBufferView = nullptr;
				initWrites[stage].push_back(writeDescriptors);

				offset += writeDescriptors.descriptorCount;

			}
			// UAV - Typed Buffer:
			{
				assert(offset == VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TYPEDBUFFER);

				VkWriteDescriptorSet writeDescriptors = {};
				writeDescriptors.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptors.dstSet = descriptorSet_CPU[stage];
				writeDescriptors.dstArrayElement = 0;
				writeDescriptors.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
				writeDescriptors.dstBinding = offset;
				writeDescriptors.descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT;
				writeDescriptors.pBufferInfo = nullptr;
				writeDescriptors.pImageInfo = nullptr;
				writeDescriptors.pTexelBufferView = bufferViews;
				initWrites[stage].push_back(writeDescriptors);

				offset += writeDescriptors.descriptorCount;

			}
			// UAV - Untyped Buffer:
			{
				assert(offset == VULKAN_DESCRIPTOR_SET_OFFSET_UAV_UNTYPEDBUFFER);

				VkWriteDescriptorSet writeDescriptors = {};
				writeDescriptors.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptors.dstSet = descriptorSet_CPU[stage];
				writeDescriptors.dstArrayElement = 0;
				writeDescriptors.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				writeDescriptors.dstBinding = offset;
				writeDescriptors.descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT;
				writeDescriptors.pBufferInfo = bufferInfo;
				writeDescriptors.pImageInfo = nullptr;
				writeDescriptors.pTexelBufferView = nullptr;
				initWrites[stage].push_back(writeDescriptors);

				offset += writeDescriptors.descriptorCount;

			}


			// Sampler:
			{
				assert(offset == VULKAN_DESCRIPTOR_SET_OFFSET_SAMPLER);

				VkWriteDescriptorSet writeDescriptors = {};
				writeDescriptors.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptors.dstSet = descriptorSet_CPU[stage];
				writeDescriptors.dstArrayElement = 0;
				writeDescriptors.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				writeDescriptors.dstBinding = offset;
				writeDescriptors.descriptorCount = GPU_SAMPLER_HEAP_COUNT;
				writeDescriptors.pBufferInfo = nullptr;
				writeDescriptors.pImageInfo = samplerInfo;
				writeDescriptors.pTexelBufferView = nullptr;
				initWrites[stage].push_back(writeDescriptors);

				offset += writeDescriptors.descriptorCount;

			}

			boundDescriptors[stage].resize(offset);
		}


		reset();

	}
	GraphicsDevice_Vulkan::FrameResources::DescriptorTableFrameAllocator::~DescriptorTableFrameAllocator()
	{
		vkDestroyDescriptorPool(device->device, descriptorPool, nullptr);
	}
	void GraphicsDevice_Vulkan::FrameResources::DescriptorTableFrameAllocator::reset()
	{
		for (int stage = 0; stage < SHADERSTAGE_COUNT; ++stage)
		{
			ringOffset[stage] = 0;
			dirty[stage] = true; 
			

			// STAGING CPU descriptor table needs to be initialized:
			vkUpdateDescriptorSets(device->device, static_cast<uint32_t>(initWrites[stage].size()), initWrites[stage].data(), 0, nullptr);

			std::fill(boundDescriptors[stage].begin(), boundDescriptors[stage].end(), WI_NULL_HANDLE);

		}
	}
	void GraphicsDevice_Vulkan::FrameResources::DescriptorTableFrameAllocator::update(SHADERSTAGE stage, UINT offset, VkBuffer descriptor, VkCommandBuffer commandList)
	{
		//if (descriptor == nullptr)
		//{
		//	return;
		//}
		//UINT idx = stage * itemCount + offset;

		dirty[stage] = true;


		//D3D12_CPU_DESCRIPTOR_HANDLE dst_staging = heap_CPU->GetCPUDescriptorHandleForHeapStart();
		//dst_staging.ptr += idx * itemSize;

		//device->CopyDescriptorsSimple(1, dst_staging, *descriptor, (D3D12_DESCRIPTOR_HEAP_TYPE)descriptorType);
	}
	void GraphicsDevice_Vulkan::FrameResources::DescriptorTableFrameAllocator::validate(VkCommandBuffer commandList)
	{
		for (int stage = 0; stage < SHADERSTAGE_COUNT; ++stage)
		{
			if (dirty[stage])
			{

				// 1.) Copy descriptors from STAGING -> to GPU visible table:

				VkCopyDescriptorSet copyDescriptors[8] = {};

				// CBV:
				{
					copyDescriptors[0].sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
					copyDescriptors[0].descriptorCount = GPU_RESOURCE_HEAP_CBV_COUNT;
					copyDescriptors[0].srcSet = descriptorSet_CPU[stage];
					copyDescriptors[0].srcBinding = VULKAN_DESCRIPTOR_SET_OFFSET_CBV;
					copyDescriptors[0].srcArrayElement = 0;
					copyDescriptors[0].dstSet = descriptorSet_GPU[stage][ringOffset[stage]];
					copyDescriptors[0].dstBinding = VULKAN_DESCRIPTOR_SET_OFFSET_CBV;
					copyDescriptors[0].dstArrayElement = 0;
				}

				// SRV - Texture:
				{
					copyDescriptors[1].sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
					copyDescriptors[1].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT;
					copyDescriptors[1].srcSet = descriptorSet_CPU[stage];
					copyDescriptors[1].srcBinding = VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TEXTURE;
					copyDescriptors[1].srcArrayElement = 0;
					copyDescriptors[1].dstSet = descriptorSet_GPU[stage][ringOffset[stage]];
					copyDescriptors[1].dstBinding = VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TEXTURE;
					copyDescriptors[1].dstArrayElement = 0;
				}
				// SRV - Typed Buffer:
				{
					copyDescriptors[2].sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
					copyDescriptors[2].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT;
					copyDescriptors[2].srcSet = descriptorSet_CPU[stage];
					copyDescriptors[2].srcBinding = VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TYPEDBUFFER;
					copyDescriptors[2].srcArrayElement = 0;
					copyDescriptors[2].dstSet = descriptorSet_GPU[stage][ringOffset[stage]];
					copyDescriptors[2].dstBinding = VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TYPEDBUFFER;
					copyDescriptors[2].dstArrayElement = 0;
				}
				// SRV - Untyped Buffer:
				{
					copyDescriptors[3].sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
					copyDescriptors[3].descriptorCount = GPU_RESOURCE_HEAP_SRV_COUNT;
					copyDescriptors[3].srcSet = descriptorSet_CPU[stage];
					copyDescriptors[3].srcBinding = VULKAN_DESCRIPTOR_SET_OFFSET_SRV_UNTYPEDBUFFER;
					copyDescriptors[3].srcArrayElement = 0;
					copyDescriptors[3].dstSet = descriptorSet_GPU[stage][ringOffset[stage]];
					copyDescriptors[3].dstBinding = VULKAN_DESCRIPTOR_SET_OFFSET_SRV_UNTYPEDBUFFER;
					copyDescriptors[3].dstArrayElement = 0;
				}

				// UAV - Texture:
				{
					copyDescriptors[4].sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
					copyDescriptors[4].descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT;
					copyDescriptors[4].srcSet = descriptorSet_CPU[stage];
					copyDescriptors[4].srcBinding = VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TEXTURE;
					copyDescriptors[4].srcArrayElement = 0;
					copyDescriptors[4].dstSet = descriptorSet_GPU[stage][ringOffset[stage]];
					copyDescriptors[4].dstBinding = VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TEXTURE;
					copyDescriptors[4].dstArrayElement = 0;
				}
				// UAV - Typed Buffer:
				{
					copyDescriptors[5].sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
					copyDescriptors[5].descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT;
					copyDescriptors[5].srcSet = descriptorSet_CPU[stage];
					copyDescriptors[5].srcBinding = VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TYPEDBUFFER;
					copyDescriptors[5].srcArrayElement = 0;
					copyDescriptors[5].dstSet = descriptorSet_GPU[stage][ringOffset[stage]];
					copyDescriptors[5].dstBinding = VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TYPEDBUFFER;
					copyDescriptors[5].dstArrayElement = 0;
				}
				// UAV - Untyped Buffer:
				{
					copyDescriptors[6].sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
					copyDescriptors[6].descriptorCount = GPU_RESOURCE_HEAP_UAV_COUNT;
					copyDescriptors[6].srcSet = descriptorSet_CPU[stage];
					copyDescriptors[6].srcBinding = VULKAN_DESCRIPTOR_SET_OFFSET_UAV_UNTYPEDBUFFER;
					copyDescriptors[6].srcArrayElement = 0;
					copyDescriptors[6].dstSet = descriptorSet_GPU[stage][ringOffset[stage]];
					copyDescriptors[6].dstBinding = VULKAN_DESCRIPTOR_SET_OFFSET_UAV_UNTYPEDBUFFER;
					copyDescriptors[6].dstArrayElement = 0;
				}

				// Sampler:
				{
					copyDescriptors[7].sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
					copyDescriptors[7].descriptorCount = GPU_SAMPLER_HEAP_COUNT;
					copyDescriptors[7].srcSet = descriptorSet_CPU[stage];
					copyDescriptors[7].srcBinding = VULKAN_DESCRIPTOR_SET_OFFSET_SAMPLER;
					copyDescriptors[7].srcArrayElement = 0;
					copyDescriptors[7].dstSet = descriptorSet_GPU[stage][ringOffset[stage]];
					copyDescriptors[7].dstBinding = VULKAN_DESCRIPTOR_SET_OFFSET_SAMPLER;
					copyDescriptors[7].dstArrayElement = 0;
				}

				vkUpdateDescriptorSets(device->device, 0, nullptr, ARRAYSIZE(copyDescriptors), copyDescriptors);


				// 2.) Bind GPU visible descriptor table which we just updated:
				if (stage == CS)
				{
					vkCmdBindDescriptorSets(commandList, VK_PIPELINE_BIND_POINT_COMPUTE, device->defaultPipelineLayout_Compute, 0, 1, &descriptorSet_GPU[stage][ringOffset[stage]], 0, nullptr);
				}
				else
				{
					vkCmdBindDescriptorSets(commandList, VK_PIPELINE_BIND_POINT_GRAPHICS, device->defaultPipelineLayout_Graphics, stage, 1, &descriptorSet_GPU[stage][ringOffset[stage]], 0, nullptr);
				}


				// mark the descriptors of this stage as up to date
				dirty[stage] = false;

				// allocate next chunk for GPU visible descriptor table:
				ringOffset[stage]++;

				if (ringOffset[stage] >= descriptorSet_GPU[stage].size())
				{
					// ran out of descriptor allocation space, stall CPU and wrap the ring buffer:
					assert(0 && "TODO Stall");
					ringOffset[stage] = 0;
				}

			}
		}
	}




	void GraphicsDevice_Vulkan::RenderPassManager::reset()
	{
		dirty = true;

		memset(attachments, 0, sizeof(attachments));
		attachmentCount = 0;
		attachmentLayers = 1;

		memset(clearColor, 0, sizeof(clearColor));

		activeRTHash = 0;
		pDesc = nullptr;

		overrideRenderPass = VK_NULL_HANDLE;
		overrideFramebuffer = VK_NULL_HANDLE;

		clearRequests.clear();
	}
	void GraphicsDevice_Vulkan::RenderPassManager::disable(VkCommandBuffer commandBuffer)
	{
		if (activeRTHash)
		{
			vkCmdEndRenderPass(commandBuffer);
		}
		activeRTHash = 0;
	}
	void GraphicsDevice_Vulkan::RenderPassManager::validate(VkDevice device, VkCommandBuffer commandBuffer)
	{
		if (attachmentCount == 0)
		{
			return;
		}

		uint64_t requestRTHash = 0;

		if (!overrideRenderPass && !overrideFramebuffer)
		{
			requestRTHash = (uint64_t)pDesc->DSFormat;
			for (UINT i = 0; i < pDesc->numRTs; ++i)
			{
				requestRTHash = (requestRTHash ^ ((uint64_t)pDesc->RTFormats[i] << 1)) >> 1; // primary hash based on PSO formats description
				requestRTHash = (requestRTHash ^ ((uint64_t)attachments[i] << 1)) >> 1; // setrendertarget <-> PSO layout might mismatch so we HAVE to also include this in the hash :(
			}
			requestRTHash = requestRTHash ^ 73856093 * attachmentsExtents.width ^ 19349663 * attachmentsExtents.height; // also hash based on render area extent. Maybe not necessary but keep it for safety now...
		}
		else
		{
			requestRTHash = 0xFFFFFFFF; // override setrendertarget hashing with custom renderpass (eg. presentation render pass because it has some custom setup)
		}

		if (dirty || activeRTHash == 0 || activeRTHash != requestRTHash)
		{
			VkRenderPass renderPass = overrideRenderPass;
			VkFramebuffer frameBuffer = overrideFramebuffer;

			if (renderPass == VK_NULL_HANDLE || frameBuffer == VK_NULL_HANDLE)
			{
				assert(pDesc != nullptr);

				uint32_t psoAttachmentCount = pDesc->numRTs + (pDesc->DSFormat == FORMAT_UNKNOWN ? 0 : 1);

				assert(psoAttachmentCount <= attachmentCount);


				RenderPassAndFramebuffer& states = renderPassCollection[requestRTHash];

				if (states.renderPass == VK_NULL_HANDLE)
				{
					VkAttachmentDescription attachmentDescriptions[9];
					VkAttachmentReference colorAttachmentRefs[9];

					for (UINT i = 0; i < pDesc->numRTs; ++i)
					{
						VkAttachmentDescription attachment = {};
						attachment.format = _ConvertFormat(pDesc->RTFormats[i]);
						attachment.samples = VK_SAMPLE_COUNT_1_BIT;
						attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
						attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
						attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
						attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
						attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						attachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
						attachmentDescriptions[i] = attachment;

						VkAttachmentReference ref = {};
						ref.attachment = i;
						ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
						colorAttachmentRefs[i] = ref;
					}


					VkSubpassDescription subpass = {};
					subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
					subpass.colorAttachmentCount = pDesc->numRTs;
					subpass.pColorAttachments = colorAttachmentRefs;

					VkAttachmentDescription depthAttachment = {};
					VkAttachmentReference depthAttachmentRef = {};
					if (pDesc->DSFormat != FORMAT_UNKNOWN)
					{
						depthAttachment.format = _ConvertFormat(pDesc->DSFormat);
						depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
						depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
						depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
						depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
						depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
						depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						depthAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
						attachmentDescriptions[pDesc->numRTs] = depthAttachment;

						depthAttachmentRef.attachment = pDesc->numRTs;
						depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

						subpass.pDepthStencilAttachment = &depthAttachmentRef;
					}

					VkRenderPassCreateInfo renderPassInfo = {};
					renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
					renderPassInfo.attachmentCount = psoAttachmentCount;
					renderPassInfo.pAttachments = attachmentDescriptions;
					renderPassInfo.subpassCount = 1;
					renderPassInfo.pSubpasses = &subpass;

					if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &states.renderPass) != VK_SUCCESS) {
						throw std::runtime_error("failed to create render pass!");
					}
				}
				if (states.frameBuffer == VK_NULL_HANDLE)
				{
					VkFramebufferCreateInfo framebufferInfo = {};
					framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
					framebufferInfo.renderPass = states.renderPass;
					framebufferInfo.attachmentCount = psoAttachmentCount;
					framebufferInfo.pAttachments = attachments;
					framebufferInfo.width = attachmentsExtents.width;
					framebufferInfo.height = attachmentsExtents.height;
					framebufferInfo.layers = attachmentLayers;

					if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &states.frameBuffer) != VK_SUCCESS) {
						throw std::runtime_error("failed to create framebuffer!");
					}
				}

				renderPass = states.renderPass;
				frameBuffer = states.frameBuffer;
			}

			if (activeRTHash)
			{
				vkCmdEndRenderPass(commandBuffer);
			}

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = frameBuffer;
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = attachmentsExtents;
			renderPassInfo.clearValueCount = attachmentCount;
			renderPassInfo.pClearValues = clearColor;
			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			activeRTHash = requestRTHash;
			dirty = false;


			// Performing texture clear requests if needed:
			if (!clearRequests.empty())
			{
				VkClearAttachment clearInfos[9];
				UINT realClearCount = 0;
				bool remainingClearRequests = false;
				for (UINT i = 0; i < clearRequests.size(); ++i)
				{
					if (clearRequests[i].attachment == VK_NULL_HANDLE)
					{
						continue;
					}

					for (UINT j = 0; j < attachmentCount; ++j)
					{
						if (clearRequests[i].attachment == attachments[j])
						{
							if (clearRequests[i].clearFlags == 0)
							{
								clearInfos[realClearCount].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
								clearInfos[realClearCount].clearValue = clearRequests[i].clearValue;
								clearInfos[realClearCount].colorAttachment = j;

								realClearCount++;
								clearRequests[i].attachment = VK_NULL_HANDLE;
							}
							else
							{
								clearInfos[realClearCount].aspectMask = 0;
								if (clearRequests[i].clearFlags & CLEAR_DEPTH)
								{
									clearInfos[realClearCount].aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
								}
								if (clearRequests[i].clearFlags & CLEAR_STENCIL)
								{
									clearInfos[realClearCount].aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
								}
								clearInfos[realClearCount].clearValue = clearRequests[i].clearValue;
								clearInfos[realClearCount].colorAttachment = 0;

								realClearCount++;
								clearRequests[i].attachment = VK_NULL_HANDLE;
							}

							continue;
						}
					}

					remainingClearRequests = true;
				}
				if (realClearCount > 0)
				{
					VkClearRect rect = {};
					rect.baseArrayLayer = 0;
					rect.layerCount = 1;
					rect.rect.offset.x = 0;
					rect.rect.offset.y = 0;
					rect.rect.extent.width = attachmentsExtents.width;
					rect.rect.extent.height = attachmentsExtents.height;

					vkCmdClearAttachments(commandBuffer, realClearCount, clearInfos, 1, &rect);
				}

				if (!remainingClearRequests)
				{
					clearRequests.clear();
				}
			}

		}
	}



	// Engine functions
	VkCommandBuffer GraphicsDevice_Vulkan::GetDirectCommandList(GRAPHICSTHREAD threadID) { return GetFrameResources().commandBuffers[threadID]; }

	GraphicsDevice_Vulkan::GraphicsDevice_Vulkan(wiWindowRegistration::window_type window, bool fullscreen, bool debuglayer)
	{
		DEBUGDEVICE = debuglayer;
		BACKBUFFER_FORMAT = FORMAT::FORMAT_B8G8R8A8_UNORM;

		FULLSCREEN = fullscreen;

		RECT rect = RECT();
		GetClientRect(window, &rect);
		SCREENWIDTH = rect.right - rect.left;
		SCREENHEIGHT = rect.bottom - rect.top;



		// Fill out application info:
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Wicked Engine Application";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Wicked Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		// Enumerate available extensions:
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		std::vector<const char*> extensionNames;
		//for (auto& x : extensions)
		//{
		//	extensionNames.push_back(x.extensionName);
		//}
		extensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
		extensionNames.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);


		bool enableValidationLayers = debuglayer;
		
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			//throw std::runtime_error("validation layers requested, but not available!");
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
			if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
				throw std::runtime_error("failed to create instance!");
			}
		}

		// Register validation layer callback:
		if (enableValidationLayers)
		{
			VkDebugReportCallbackCreateInfoEXT createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
			createInfo.pfnCallback = debugCallback;
			if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
				throw std::runtime_error("failed to set up debug callback!");
			}
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
				throw std::runtime_error("failed to create window surface!");
			}
#else
#error WICKEDENGINE VULKAN DEVICE ERROR: PLATFORM NOT SUPPORTED
#endif // WIN32
		}


		// Enumerating and creating devices:
		{
			uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

			if (deviceCount == 0) {
				throw std::runtime_error("failed to find GPUs with Vulkan support!");
			}

			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

			for (const auto& device : devices) 
			{
				if (isDeviceSuitable(device, surface)) 
				{
					physicalDevice = device;
					break;
				}
			}

			if (physicalDevice == VK_NULL_HANDLE) {
				throw std::runtime_error("failed to find a suitable GPU!");
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

			vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

			VkPhysicalDeviceFeatures deviceFeatures = {};
			vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

			VkDeviceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

			createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			createInfo.pQueueCreateInfos = queueCreateInfos.data();

			createInfo.pEnabledFeatures = &deviceFeatures;

			createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();

			if (enableValidationLayers) {
				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();
			}
			else {
				createInfo.enabledLayerCount = 0;
			}

			if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
				throw std::runtime_error("failed to create logical device!");
			}

			vkGetDeviceQueue(device, queueIndices.graphicsFamily, 0, &graphicsQueue);
			vkGetDeviceQueue(device, queueIndices.presentFamily, 0, &presentQueue);
			vkGetDeviceQueue(device, queueIndices.copyFamily, 0, &copyQueue);
		}


		// Create default pipeline:
		{

			//
			//								##################################################################################
			//								##		The desired descriptor layout will be as such (per shader stage)		##
			//								##################################################################################
			//
			//	- We are mapping HLSL constructs to Vulkan descriptor type equivalents. The difference is that DX11 manages resource bindings by "Memory Type"
			//		but HLSL has distinctive resource types which map to them. Vulkan API has a more straight forward mapping but we are emulating the
			//		DX11 system for now...
			//
			//	- We are creating this table (descriptor set) for every shader stage. The SPIR-V shaders will have set and layout bindings compiled
			//		into them for each resource. 
			//			- The [layout set] binding will correspond to shader stage
			//				- except in compute shader because it will have only single descriptor table, special logic will handle that
			//			- The [layout location] binding will correspond to Vulkan name offset inside the set which is hard coded 
			//				(eg. see VULKAN_DESCRIPTOR_SET_OFFSET_CBV in ShaderInterop_Vulkan.h)
			//
			//	- Left hand side of this table is essentially DX12-like descriptor table layout (per stage)
			//		- DX12 maps perfectly to DX11 regarding table layout
			//	- Right hand side is corresponding Vulkan layout (per stage).
			//		- Vulkan implementation has bigger tables. 
			//			- CBV table has same amount like DX12
			//			- SRV table has 3x amount of DX12
			//			- UAV table has 3x amount of DX12
			//				- UAV counter buffer would take +1x but not used for now...
			//			- Sampler table has same amount like DX12
			//
			//	================================================================================||===============================================================
			//	|	DX11 Memory Type	|	Slot	|	HLSL name								||	Vulkan name				|	Descriptor count				|
			//	|===============================================================================||==============================================================|
			//	|	ImmediateIndexable	|	b		|	cbuffer, ConstantBuffer					||	Uniform Buffer			|	GPU_RESOURCE_HEAP_CBV_COUNT		|
			//	|-----------------------|-----------|-------------------------------------------||--------------------------|-----------------------------------|
			//	|	ShaderResourceView	|	t		|	Texture									||	Sampled Image			|	GPU_RESOURCE_HEAP_SRV_COUNT		|
			//	|						|			|	Buffer									||	Uniform Texel Buffer	|	GPU_RESOURCE_HEAP_SRV_COUNT		|
			//	|						|			|	StructuredBuffer, ByteAddressBuffer		||	Storage Buffer			|	GPU_RESOURCE_HEAP_SRV_COUNT		|
			//	|-----------------------|-----------|-------------------------------------------||--------------------------|-----------------------------------|
			//	|	UnorderedAccessView	|	u		|	RWTexture								||	Storage Image			|	GPU_RESOURCE_HEAP_UAV_COUNT		|
			//	|						|			|	RWBuffer								||	Storage Texel Buffer	|	GPU_RESOURCE_HEAP_UAV_COUNT		|
			//	|						|			|	RWStructuredBuffer, RWByteAddressBuffer	||	Storage Buffer			|	GPU_RESOURCE_HEAP_UAV_COUNT		|
			//	|-----------------------|-----------|-------------------------------------------||--------------------------|-----------------------------------|
			//	|	Sampler				|	s		|	SamplerState							||	Sampler					|	GPU_SAMPLER_HEAP_COUNT			|
			//	================================================================================||===============================================================
			//

			std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {};

			int offset = 0;

			// NOTE: we will create the layoutBinding beforehand, but only change the shader stage binding later:

			// Constant Buffers:
			assert(offset == VULKAN_DESCRIPTOR_SET_OFFSET_CBV);
			for (int j = 0; j < GPU_RESOURCE_HEAP_CBV_COUNT; ++j)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.stageFlags = 0;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				layoutBinding.binding = offset;
				layoutBinding.descriptorCount = 1;
				layoutBindings.push_back(layoutBinding);

				offset += layoutBinding.descriptorCount;
			}

			// Shader Resource Views:
			assert(offset == VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TEXTURE);
			for (int j = 0; j < GPU_RESOURCE_HEAP_SRV_COUNT; ++j)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.stageFlags = 0;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				layoutBinding.binding = offset;
				layoutBinding.descriptorCount = 1;
				layoutBindings.push_back(layoutBinding);

				offset += layoutBinding.descriptorCount;
			}

			assert(offset == VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TYPEDBUFFER);
			for (int j = 0; j < GPU_RESOURCE_HEAP_SRV_COUNT; ++j)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.stageFlags = 0;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
				layoutBinding.binding = offset;
				layoutBinding.descriptorCount = 1;
				layoutBindings.push_back(layoutBinding);

				offset += layoutBinding.descriptorCount;
			}

			assert(offset == VULKAN_DESCRIPTOR_SET_OFFSET_SRV_UNTYPEDBUFFER);
			for (int j = 0; j < GPU_RESOURCE_HEAP_SRV_COUNT; ++j)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.stageFlags = 0;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				layoutBinding.binding = offset;
				layoutBinding.descriptorCount = 1;
				layoutBindings.push_back(layoutBinding);

				offset += layoutBinding.descriptorCount;
			}


			// Unordered Access Views:
			assert(offset == VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TEXTURE);
			for (int j = 0; j < GPU_RESOURCE_HEAP_UAV_COUNT; ++j)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.stageFlags = 0;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				layoutBinding.binding = offset;
				layoutBinding.descriptorCount = 1;
				layoutBindings.push_back(layoutBinding);

				offset += layoutBinding.descriptorCount;
			}

			assert(offset == VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TYPEDBUFFER);
			for (int j = 0; j < GPU_RESOURCE_HEAP_UAV_COUNT; ++j)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.stageFlags = 0;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
				layoutBinding.binding = offset;
				layoutBinding.descriptorCount = 1;
				layoutBindings.push_back(layoutBinding);

				offset += layoutBinding.descriptorCount;
			}

			assert(offset == VULKAN_DESCRIPTOR_SET_OFFSET_UAV_UNTYPEDBUFFER);
			for (int j = 0; j < GPU_RESOURCE_HEAP_UAV_COUNT; ++j)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.stageFlags = 0;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				layoutBinding.binding = offset;
				layoutBinding.descriptorCount = 1;
				layoutBindings.push_back(layoutBinding);

				offset += layoutBinding.descriptorCount;
			}


			// Samplers:
			assert(offset == VULKAN_DESCRIPTOR_SET_OFFSET_SAMPLER);
			for (int j = 0; j < GPU_SAMPLER_HEAP_COUNT; ++j)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.stageFlags = 0;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				layoutBinding.binding = offset;
				layoutBinding.descriptorCount = 1;
				layoutBindings.push_back(layoutBinding);

				offset += layoutBinding.descriptorCount;
			}

			descriptorCount = offset;


			for (int stage = 0; stage < SHADERSTAGE_COUNT; ++stage)
			{
				VkShaderStageFlags vkstage;

				switch (stage)
				{
				case VS:
					vkstage = VK_SHADER_STAGE_VERTEX_BIT;
					break;
				case GS:
					vkstage = VK_SHADER_STAGE_GEOMETRY_BIT;
					break;
				case HS:
					vkstage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
					break;
				case DS:
					vkstage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
					break;
				case PS:
					vkstage = VK_SHADER_STAGE_FRAGMENT_BIT;
					break;
				case CS:
					vkstage = VK_SHADER_STAGE_COMPUTE_BIT;
					break;
				}

				// all stages will have the same layout, just different shader stage visibility:
				for (auto& x : layoutBindings)
				{
					x.stageFlags = vkstage;
				}

				VkDescriptorSetLayoutCreateInfo descriptorSetlayoutInfo = {};
				descriptorSetlayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				descriptorSetlayoutInfo.pBindings = layoutBindings.data();
				descriptorSetlayoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
				if (vkCreateDescriptorSetLayout(device, &descriptorSetlayoutInfo, nullptr, &defaultDescriptorSetlayouts[stage]) != VK_SUCCESS) {
					throw std::runtime_error("failed to create descriptor set layout!");
				}
			}

			// Graphics:
			{
				VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
				pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutInfo.pSetLayouts = defaultDescriptorSetlayouts;
				pipelineLayoutInfo.setLayoutCount = 5; // vs, gs, hs, ds, ps
				pipelineLayoutInfo.pushConstantRangeCount = 0;

				if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &defaultPipelineLayout_Graphics) != VK_SUCCESS) {
					throw std::runtime_error("failed to create graphics pipeline layout!");
				}
			}

			// Compute:
			{
				VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
				pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutInfo.pSetLayouts = &defaultDescriptorSetlayouts[CS];
				pipelineLayoutInfo.setLayoutCount = 1; // cs
				pipelineLayoutInfo.pushConstantRangeCount = 0;

				if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &defaultPipelineLayout_Compute) != VK_SUCCESS) {
					throw std::runtime_error("failed to create compute pipeline layout!");
				}
			}
		}


		// Set up swap chain:
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

			VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
			VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);

			swapChainExtent = { static_cast<uint32_t>(SCREENWIDTH), static_cast<uint32_t>(SCREENHEIGHT) };
			swapChainExtent.width = std::max(swapChainSupport.capabilities.minImageExtent.width, std::min(swapChainSupport.capabilities.maxImageExtent.width, swapChainExtent.width));
			swapChainExtent.height = std::max(swapChainSupport.capabilities.minImageExtent.height, std::min(swapChainSupport.capabilities.maxImageExtent.height, swapChainExtent.height));

			//uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
			//if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			//	imageCount = swapChainSupport.capabilities.maxImageCount;
			//}

			uint32_t imageCount = BACKBUFFER_COUNT;

			VkSwapchainCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.surface = surface;
			createInfo.minImageCount = imageCount;
			createInfo.imageFormat = surfaceFormat.format;
			createInfo.imageColorSpace = surfaceFormat.colorSpace;
			createInfo.imageExtent = swapChainExtent;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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
			createInfo.presentMode = presentMode;
			createInfo.clipped = VK_TRUE;
			createInfo.oldSwapchain = VK_NULL_HANDLE;

			if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
				throw std::runtime_error("failed to create swap chain!");
			}

			vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
			assert(imageCount == BACKBUFFER_COUNT);
			swapChainImages.resize(imageCount);
			vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
			swapChainImageFormat = surfaceFormat.format;

		}




		// Create default render pass:
		{
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = swapChainImageFormat;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
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

			if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &defaultRenderPass) != VK_SUCCESS) {
				throw std::runtime_error("failed to create render pass!");
			}

		}

		// Create frame resources:
		{
			int i = 0;
			for(auto& frame : frames)
			{
				// Fence:
				{
					VkFenceCreateInfo fenceInfo = {};
					fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
					//fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
					vkCreateFence(device, &fenceInfo, nullptr, &frame.frameFence);
				}

				// Create swap chain render targets:
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

					if (vkCreateImageView(device, &createInfo, nullptr, &frame.swapChainImageView) != VK_SUCCESS) {
						throw std::runtime_error("failed to create image views!");
					}

					VkImageView attachments[] = {
						frame.swapChainImageView
					};

					VkFramebufferCreateInfo framebufferInfo = {};
					framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
					framebufferInfo.renderPass = defaultRenderPass;
					framebufferInfo.attachmentCount = 1;
					framebufferInfo.pAttachments = attachments;
					framebufferInfo.width = swapChainExtent.width;
					framebufferInfo.height = swapChainExtent.height;
					framebufferInfo.layers = 1;

					if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &frame.swapChainFramebuffer) != VK_SUCCESS) {
						throw std::runtime_error("failed to create framebuffer!");
					}
				}

				// Create command buffers:
				{
					QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

					for (int threadID = 0; threadID < GRAPHICSTHREAD_COUNT; ++threadID)
					{
						VkCommandPoolCreateInfo poolInfo = {};
						poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
						poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
						poolInfo.flags = 0; // Optional

						if (vkCreateCommandPool(device, &poolInfo, nullptr, &frame.commandPools[threadID]) != VK_SUCCESS) {
							throw std::runtime_error("failed to create command pool!");
						}

						VkCommandBufferAllocateInfo commandBufferInfo = {};
						commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
						commandBufferInfo.commandBufferCount = 1;
						commandBufferInfo.commandPool = frame.commandPools[threadID];
						commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

						if (vkAllocateCommandBuffers(device, &commandBufferInfo, &frame.commandBuffers[threadID]) != VK_SUCCESS) {
							throw std::runtime_error("failed to create command buffers!");
						}

						VkCommandBufferBeginInfo beginInfo = {};
						beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
						beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
						beginInfo.pInheritanceInfo = nullptr; // Optional

						VkResult res = vkBeginCommandBuffer(frame.commandBuffers[threadID], &beginInfo);
						assert(res == VK_SUCCESS);
					}
				}


				// Create immediate resource allocators:
				for (int threadID = 0; threadID < GRAPHICSTHREAD_COUNT; ++threadID)
				{
					frame.resourceBuffer[threadID] = new FrameResources::ResourceFrameAllocator(physicalDevice, device, 4 * 1024 * 1024);
				}


				i++;
			}
		}

		// Create semaphores:
		{
			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {

				throw std::runtime_error("failed to create semaphores!");
			}
		}


		// Create resources for copy (transfer) queue:
		{
			QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface); // redundant!!


			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = queueFamilyIndices.copyFamily;
			poolInfo.flags = 0; // Optional

			if (vkCreateCommandPool(device, &poolInfo, nullptr, &copyCommandPool) != VK_SUCCESS) {
				throw std::runtime_error("failed to create command pool!");
			}

			VkCommandBufferAllocateInfo commandBufferInfo = {};
			commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferInfo.commandBufferCount = 1;
			commandBufferInfo.commandPool = copyCommandPool;
			commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

			if (vkAllocateCommandBuffers(device, &commandBufferInfo, &copyCommandBuffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to create command buffers!");
			}

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional

			VkResult res = vkBeginCommandBuffer(copyCommandBuffer, &beginInfo);
			assert(res == VK_SUCCESS);


			// Fence for copy queue:
			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			//fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			vkCreateFence(device, &fenceInfo, nullptr, &copyFence);
		}


		// Create resource upload buffers
		bufferUploader = new UploadBuffer(physicalDevice, device, queueIndices, 256 * 1024 * 1024);
		textureUploader = new UploadBuffer(physicalDevice, device, queueIndices, 256 * 1024 * 1024);


		// Create default null descriptors:
		{
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = 4;
			bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			bufferInfo.flags = 0;

			VkResult res = vkCreateBuffer(device, &bufferInfo, nullptr, &nullBuffer);
			assert(res == VK_SUCCESS);


			// Allocate resource backing memory:
			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(device, nullBuffer, &memRequirements);

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			VkDeviceMemory mem;
			if (vkAllocateMemory(device, &allocInfo, nullptr, &mem) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate buffer memory!");
			}

			res = vkBindBufferMemory(device, nullBuffer, mem, 0);
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
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
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

			VkResult res = vkCreateImage(device, &imageInfo, nullptr, &nullImage);
			assert(res == VK_SUCCESS);


			// Allocate resource backing memory:
			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(device, nullImage, &memRequirements);

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			VkDeviceMemory mem;
			if (vkAllocateMemory(device, &allocInfo, nullptr, &mem) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate image memory!");
			}

			res = vkBindImageMemory(device, nullImage, mem, 0);
			assert(res == VK_SUCCESS);


			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = nullImage;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
			
			res = vkCreateImageView(device, &viewInfo, nullptr, &nullImageView);
			assert(res == VK_SUCCESS);
		}
		{
			VkSamplerCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

			VkResult res = vkCreateSampler(device, &createInfo, nullptr, &nullSampler);
			assert(res == VK_SUCCESS);
		}


		// Preinitialize staging descriptor tables:
		for (auto& frame : frames)
		{
			for (int threadID = 0; threadID < GRAPHICSTHREAD_COUNT; ++threadID)
			{
				frame.ResourceDescriptorsGPU[threadID] = new FrameResources::DescriptorTableFrameAllocator(this, 1024);
			}
		}


		// Initiate first commands:
		for (int threadID = 0; threadID < GRAPHICSTHREAD_COUNT; ++threadID)
		{
			VkViewport viewports[6];
			for (UINT i = 0; i < ARRAYSIZE(viewports); ++i)
			{
				viewports[i].x = 0;
				viewports[i].y = 0;
				viewports[i].width = static_cast<float>(SCREENWIDTH);
				viewports[i].height = static_cast<float>(SCREENHEIGHT);
				viewports[i].minDepth = 0;
				viewports[i].maxDepth = 1;
			}
			vkCmdSetViewport(GetDirectCommandList(static_cast<GRAPHICSTHREAD>(threadID)), 0, ARRAYSIZE(viewports), viewports);

			VkRect2D scissors[8];
			for (int i = 0; i < ARRAYSIZE(scissors); ++i)
			{
				scissors[i].offset.x = 0;
				scissors[i].offset.y = 0;
				scissors[i].extent.width = 65535;
				scissors[i].extent.height = 65535;
			}
			vkCmdSetScissor(GetDirectCommandList(static_cast<GRAPHICSTHREAD>(threadID)), 0, ARRAYSIZE(scissors), scissors);

			float blendConstants[] = { 1,1,1,1 };
			vkCmdSetBlendConstants(GetDirectCommandList(static_cast<GRAPHICSTHREAD>(threadID)), blendConstants);
		}

		wiBackLog::post("Created GraphicsDevice_Vulkan");
	}
	GraphicsDevice_Vulkan::~GraphicsDevice_Vulkan()
	{
		WaitForGPU();

		SAFE_DELETE(bufferUploader);
		SAFE_DELETE(textureUploader);

		for (auto& frame : frames)
		{
			vkDestroyFence(device, frame.frameFence, nullptr);
			vkDestroyFramebuffer(device, frame.swapChainFramebuffer, nullptr);
			vkDestroyImageView(device, frame.swapChainImageView, nullptr);
			for (auto& commandPool : frame.commandPools)
			{
				vkDestroyCommandPool(device, commandPool, nullptr);
			}

			for (int threadID = 0; threadID < GRAPHICSTHREAD_COUNT; ++threadID)
			{
				SAFE_DELETE(frame.ResourceDescriptorsGPU[threadID]);
				SAFE_DELETE(frame.resourceBuffer[threadID]);
			}
		}

		vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

		for (int i = 0; i < SHADERSTAGE_COUNT; ++i)
		{
			vkDestroyDescriptorSetLayout(device, defaultDescriptorSetlayouts[i], nullptr);
		}
		vkDestroyPipelineLayout(device, defaultPipelineLayout_Graphics, nullptr);
		vkDestroyPipelineLayout(device, defaultPipelineLayout_Compute, nullptr);
		vkDestroyRenderPass(device, defaultRenderPass, nullptr);

		for (auto& x : swapChainImages)
		{
			vkDestroyImage(device, x, nullptr);
		}
		vkDestroySwapchainKHR(device, swapChain, nullptr);
		vkDestroyDevice(device, nullptr);
		DestroyDebugReportCallbackEXT(instance, callback, nullptr);
		vkDestroyInstance(instance, nullptr);
	}

	void GraphicsDevice_Vulkan::SetResolution(int width, int height)
	{
		if (width != SCREENWIDTH || height != SCREENHEIGHT)
		{
			SCREENWIDTH = width;
			SCREENHEIGHT = height;
			//swapChain->ResizeBuffers(2, width, height, _ConvertFormat(GetBackBufferFormat()), 0);
			RESOLUTIONCHANGED = true;
		}
	}

	Texture2D GraphicsDevice_Vulkan::GetBackBuffer()
	{
		return Texture2D();
	}

	HRESULT GraphicsDevice_Vulkan::CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *pBuffer)
	{
		DestroyBuffer(pBuffer);
		DestroyResource(pBuffer);
		pBuffer->type = GPUResource::BUFFER;
		pBuffer->Register(this);

		HRESULT hr = E_FAIL;

		pBuffer->desc = *pDesc;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = pBuffer->desc.ByteWidth /** BACKBUFFER_COUNT*/;
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

		bufferInfo.flags = 0;

		// Allow access from copy queue:
		bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

		uint32_t queueFamilyIndices[] = { 
			static_cast<uint32_t>(queueIndices.graphicsFamily), 
			static_cast<uint32_t>(queueIndices.copyFamily) 
		};
		bufferInfo.pQueueFamilyIndices = queueFamilyIndices;
		bufferInfo.queueFamilyIndexCount = ARRAYSIZE(queueFamilyIndices);



		VkResult res;
		res = vkCreateBuffer(device, &bufferInfo, nullptr, reinterpret_cast<VkBuffer*>(&pBuffer->resource));
		hr = res == VK_SUCCESS;
		assert(SUCCEEDED(hr));



		// Allocate resource backing memory:
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, (VkBuffer)pBuffer->resource, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT /*| VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT*/);

		if (vkAllocateMemory(device, &allocInfo, nullptr, reinterpret_cast<VkDeviceMemory*>(&pBuffer->resourceMemory)) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		res = vkBindBufferMemory(device, (VkBuffer)pBuffer->resource, (VkDeviceMemory)pBuffer->resourceMemory, 0);
		hr = res == VK_SUCCESS;
		assert(SUCCEEDED(hr));



		// Issue data copy on request:
		if (pInitialData != nullptr)
		{
			copyQueueLock.lock();
			{
				uint8_t* dest = bufferUploader->allocate(static_cast<size_t>(memRequirements.size), static_cast<size_t>(memRequirements.alignment));
				memcpy(dest, pInitialData->pSysMem, static_cast<size_t>(memRequirements.size));

				VkBufferCopy copyRegion = {};
				copyRegion.size = memRequirements.size;
				copyRegion.srcOffset = bufferUploader->calculateOffset(dest);
				copyRegion.dstOffset = 0;

				VkBufferMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				barrier.buffer = (VkBuffer)pBuffer->resource;
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				vkCmdPipelineBarrier(
					copyCommandBuffer,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0, nullptr,
					1, &barrier,
					0, nullptr
				);


				vkCmdCopyBuffer(copyCommandBuffer, bufferUploader->resource, (VkBuffer)pBuffer->resource, 1, &copyRegion);


				VkAccessFlags tmp = barrier.srcAccessMask;
				barrier.srcAccessMask = barrier.dstAccessMask;

				if (pBuffer->desc.BindFlags & BIND_CONSTANT_BUFFER)
				{
					barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
				}
				else if (pBuffer->desc.BindFlags & BIND_VERTEX_BUFFER)
				{
					barrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
				}
				else if (pBuffer->desc.BindFlags & BIND_INDEX_BUFFER)
				{
					barrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
				}
				else
				{
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				}

				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				vkCmdPipelineBarrier(
					copyCommandBuffer,
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



		if (pDesc->BindFlags & BIND_SHADER_RESOURCE && pBuffer->desc.Format != FORMAT_UNKNOWN)
		{
			VkBufferViewCreateInfo srv_desc = {};
			srv_desc.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
			srv_desc.buffer = (VkBuffer)pBuffer->resource;
			srv_desc.flags = 0;
			srv_desc.format = _ConvertFormat(pBuffer->desc.Format);
			srv_desc.offset = 0;
			srv_desc.range = pBuffer->desc.ByteWidth;

			res = vkCreateBufferView(device, &srv_desc, nullptr, reinterpret_cast<VkBufferView*>(&pBuffer->SRV));
			assert(res == VK_SUCCESS);
		}

		if (pDesc->BindFlags & BIND_UNORDERED_ACCESS && pBuffer->desc.Format != FORMAT_UNKNOWN)
		{
			VkBufferViewCreateInfo uav_desc = {};
			uav_desc.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
			uav_desc.buffer = (VkBuffer)pBuffer->resource;
			uav_desc.flags = 0;
			uav_desc.format = _ConvertFormat(pBuffer->desc.Format);
			uav_desc.offset = 0;
			uav_desc.range = pBuffer->desc.ByteWidth;

			res = vkCreateBufferView(device, &uav_desc, nullptr, reinterpret_cast<VkBufferView*>(&pBuffer->UAV));
			assert(res == VK_SUCCESS);
		}



		return hr;
	}
	HRESULT GraphicsDevice_Vulkan::CreateTexture1D(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture1D *pTexture1D)
	{
		DestroyTexture1D(pTexture1D);
		DestroyResource(pTexture1D);
		pTexture1D->type = GPUResource::TEXTURE_1D;
		pTexture1D->Register(this);

		pTexture1D->desc = *pDesc;

		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateTexture2D(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture2D *pTexture2D)
	{
		DestroyTexture2D(pTexture2D);
		DestroyResource(pTexture2D);
		pTexture2D->type = GPUResource::TEXTURE_2D;
		pTexture2D->Register(this);

		pTexture2D->desc = *pDesc;

		if (pTexture2D->desc.MipLevels == 0)
		{
			pTexture2D->desc.MipLevels = static_cast<UINT>(log2(std::max(pTexture2D->desc.Width, pTexture2D->desc.Height)));
		}


		HRESULT hr = E_FAIL;

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = pTexture2D->desc.Width;
		imageInfo.extent.height = pTexture2D->desc.Height;
		imageInfo.extent.depth = 1;
		imageInfo.format = _ConvertFormat(pTexture2D->desc.Format);
		imageInfo.arrayLayers = pTexture2D->desc.ArraySize;
		imageInfo.mipLevels = pTexture2D->desc.MipLevels;
		imageInfo.samples = static_cast<VkSampleCountFlagBits>(pTexture2D->desc.SampleDesc.Count);
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // or preinitialized?
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = 0;
		if (pTexture2D->desc.BindFlags & BIND_SHADER_RESOURCE)
		{
			imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		if (pTexture2D->desc.BindFlags & BIND_RENDER_TARGET)
		{
			imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
		if (pTexture2D->desc.BindFlags & BIND_DEPTH_STENCIL)
		{
			imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}
		imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		imageInfo.flags = 0;
		if (pTexture2D->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
		{
			imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		}
		if (pTexture2D->desc.ArraySize > 1)
		{
			imageInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
		}

		VkResult res;
		res = vkCreateImage(device, &imageInfo, nullptr, reinterpret_cast<VkImage*>(&pTexture2D->resource));
		hr = res == VK_SUCCESS;
		assert(SUCCEEDED(hr));



		// Allocate resource backing memory:
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, (VkImage)pTexture2D->resource, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, reinterpret_cast<VkDeviceMemory*>(&pTexture2D->resourceMemory)) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		res = vkBindImageMemory(device, (VkImage)pTexture2D->resource, (VkDeviceMemory)pTexture2D->resourceMemory, 0);
		hr = res == VK_SUCCESS; 
		assert(SUCCEEDED(hr));



		// Issue data copy on request:
		if (pInitialData != nullptr)
		{
			copyQueueLock.lock();
			{
				uint8_t* dest = textureUploader->allocate(static_cast<size_t>(memRequirements.size), static_cast<size_t>(memRequirements.alignment));

				std::vector<VkBufferImageCopy> copyRegions;

				size_t cpyoffset = 0;
				UINT initDataIdx = 0;
				for (UINT arrayIndex = 0; arrayIndex < pDesc->ArraySize; ++arrayIndex)
				{
					uint32_t width = pDesc->Width;
					uint32_t height = pDesc->Height;
					for (UINT mip = 0; mip < pDesc->MipLevels; ++mip)
					{
						const SubresourceData& subresourceData = pInitialData[initDataIdx++];
						size_t cpysize = subresourceData.SysMemPitch * height;
						switch (pDesc->Format)
						{
						case FORMAT_BC1_UNORM:
						case FORMAT_BC2_UNORM:
						case FORMAT_BC3_UNORM:
							cpysize /= 4;
						default:
							break;
						}
						uint8_t* cpyaddr = dest + cpyoffset;
						memcpy(cpyaddr, subresourceData.pSysMem, cpysize);
						cpyoffset += cpysize;

						VkBufferImageCopy copyRegion = {};
						copyRegion.bufferOffset = textureUploader->calculateOffset(cpyaddr);
						copyRegion.bufferRowLength = 0;
						copyRegion.bufferImageHeight = 0;

						copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						copyRegion.imageSubresource.mipLevel = mip;
						copyRegion.imageSubresource.baseArrayLayer = arrayIndex;
						copyRegion.imageSubresource.layerCount = 1;

						copyRegion.imageOffset = { 0, 0, 0 };
						copyRegion.imageExtent = {
							width,
							height,
							1
						};

						width = std::max(1u, width / 2);
						height /= std::max(1u, height / 2);

						copyRegions.push_back(copyRegion);
					}
				}

				VkImageMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = (VkImage)pTexture2D->resource;
				barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.srcQueueFamilyIndex = queueIndices.copyFamily;
				barrier.dstQueueFamilyIndex = queueIndices.copyFamily;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = pDesc->ArraySize;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = pDesc->MipLevels;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				vkCmdPipelineBarrier(
					copyCommandBuffer,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);

				vkCmdCopyBufferToImage(copyCommandBuffer, textureUploader->resource, (VkImage)pTexture2D->resource, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)copyRegions.size(), copyRegions.data());


				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				vkCmdPipelineBarrier(
					copyCommandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);

			}
			copyQueueLock.unlock();
		}



		// Issue creation of additional descriptors for the resource:

		if (pTexture2D->desc.BindFlags & BIND_RENDER_TARGET)
		{
			UINT arraySize = pTexture2D->desc.ArraySize;
			UINT sampleCount = pTexture2D->desc.SampleDesc.Count;
			bool multisampled = sampleCount > 1;

			VkImageViewCreateInfo rtv_desc = {};
			rtv_desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			rtv_desc.flags = 0;
			rtv_desc.image = (VkImage)pTexture2D->resource;
			rtv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
			rtv_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			rtv_desc.subresourceRange.baseArrayLayer = 0;
			rtv_desc.subresourceRange.layerCount = 1;
			rtv_desc.subresourceRange.baseMipLevel = 0;
			rtv_desc.subresourceRange.levelCount = 1;

			rtv_desc.format = _ConvertFormat(pTexture2D->desc.Format);


			if (pTexture2D->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
			{
				// TextureCube, TextureCubeArray...
				UINT slices = arraySize / 6;

				if (arraySize > 6)
				{
					rtv_desc.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
				}
				else
				{
					rtv_desc.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
				}

				if (pTexture2D->independentRTVCubemapFaces)
				{
					// independent faces
					for (UINT i = 0; i < arraySize; ++i)
					{
						rtv_desc.subresourceRange.baseArrayLayer = i;
						rtv_desc.subresourceRange.layerCount = 1;

						pTexture2D->additionalRTVs.push_back(VK_NULL_HANDLE);
						res = vkCreateImageView(device, &rtv_desc, nullptr, reinterpret_cast<VkImageView*>(&pTexture2D->additionalRTVs.back()));
						assert(res == VK_SUCCESS);
					}
				}
				else if (pTexture2D->independentRTVArraySlices)
				{
					// independent cubemaps
					for (UINT i = 0; i < slices; ++i)
					{
						rtv_desc.subresourceRange.baseArrayLayer = i * 6;
						rtv_desc.subresourceRange.layerCount = 6;

						pTexture2D->additionalRTVs.push_back(VK_NULL_HANDLE);
						res = vkCreateImageView(device, &rtv_desc, nullptr, reinterpret_cast<VkImageView*>(&pTexture2D->additionalRTVs.back()));
						assert(res == VK_SUCCESS);
					}
				}

				{
					// Create full-resource RTVs:
					rtv_desc.subresourceRange.baseArrayLayer = 0;
					rtv_desc.subresourceRange.layerCount = pTexture2D->desc.ArraySize;
					rtv_desc.subresourceRange.baseMipLevel = 0;
					rtv_desc.subresourceRange.levelCount = 1; // RTV so only 1 MIP!

					res = vkCreateImageView(device, &rtv_desc, nullptr, reinterpret_cast<VkImageView*>(&pTexture2D->RTV));
					hr = res == VK_SUCCESS;
					assert(SUCCEEDED(hr));
				}
			}
			else
			{
				if (arraySize > 1 && pTexture2D->independentRTVArraySlices)
				{
					if (multisampled)
					{
						rtv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY; // MSArray?
					}
					else
					{
						rtv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
					}
					UINT slices = arraySize;

					// independent slices
					for (UINT i = 0; i < slices; ++i)
					{
						rtv_desc.subresourceRange.baseArrayLayer = i;
						rtv_desc.subresourceRange.layerCount = 1;

						pTexture2D->additionalRTVs.push_back(VK_NULL_HANDLE);
						res = vkCreateImageView(device, &rtv_desc, nullptr, reinterpret_cast<VkImageView*>(&pTexture2D->additionalRTVs.back()));
						assert(res == VK_SUCCESS);
					}
				}

				{
					// Create full-resource RTV:

					if (arraySize > 1)
					{
						if (multisampled)
						{
							rtv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY; // MSArray?
						}
						else
						{
							rtv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
						}
					}
					else
					{
						if (multisampled)
						{
							rtv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D; // MSAA?
						}
						else
						{
							rtv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
						}
					}

					rtv_desc.subresourceRange.baseArrayLayer = 0;
					rtv_desc.subresourceRange.layerCount = pTexture2D->desc.ArraySize;
					rtv_desc.subresourceRange.baseMipLevel = 0;
					rtv_desc.subresourceRange.levelCount = 1; // RTV so only 1 MIP!

					res = vkCreateImageView(device, &rtv_desc, nullptr, reinterpret_cast<VkImageView*>(&pTexture2D->RTV));
					hr = res == VK_SUCCESS;
					assert(SUCCEEDED(hr));
				}

			}
		}


		if (pTexture2D->desc.BindFlags & BIND_DEPTH_STENCIL)
		{
			UINT arraySize = pTexture2D->desc.ArraySize;
			UINT sampleCount = pTexture2D->desc.SampleDesc.Count;
			bool multisampled = sampleCount > 1;

			VkImageViewCreateInfo dsv_desc = {};
			dsv_desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			dsv_desc.flags = 0;
			dsv_desc.image = (VkImage)pTexture2D->resource;
			dsv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
			dsv_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			dsv_desc.format = _ConvertFormat(pTexture2D->desc.Format);


			if (arraySize > 1)
			{
				if (pTexture2D->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
				{
					if (arraySize > 6)
					{
						dsv_desc.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
					}
					else
					{
						dsv_desc.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
					}
				}
				else
				{
					if (multisampled)
					{
						dsv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY; // MSArray?
					}
					else
					{
						dsv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
					}
				}

			}
			else
			{
				if (multisampled)
				{
					dsv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D; // MSAA?
				}
				else
				{
					dsv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
				}
			}

			// Create full-resource DSV:
			dsv_desc.subresourceRange.baseArrayLayer = 0;
			dsv_desc.subresourceRange.layerCount = pTexture2D->desc.ArraySize;
			dsv_desc.subresourceRange.baseMipLevel = 0;
			dsv_desc.subresourceRange.levelCount = 1; // DSV so only 1 MIP!

			res = vkCreateImageView(device, &dsv_desc, nullptr, reinterpret_cast<VkImageView*>(&pTexture2D->DSV));
			hr = res == VK_SUCCESS;
			assert(SUCCEEDED(hr));
		}


		if (pTexture2D->desc.BindFlags & BIND_SHADER_RESOURCE)
		{
			UINT arraySize = pTexture2D->desc.ArraySize;
			UINT sampleCount = pTexture2D->desc.SampleDesc.Count;
			bool multisampled = sampleCount > 1;

			VkImageViewCreateInfo srv_desc = {};
			srv_desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			srv_desc.flags = 0;
			srv_desc.image = (VkImage)pTexture2D->resource;
			srv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
			srv_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			if (pTexture2D->desc.BindFlags & BIND_DEPTH_STENCIL)
			{
				srv_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			}

			srv_desc.subresourceRange.baseArrayLayer = 0;
			srv_desc.subresourceRange.layerCount = 1;
			srv_desc.subresourceRange.baseMipLevel = 0;
			srv_desc.subresourceRange.levelCount = 1;

			srv_desc.format = _ConvertFormat(pTexture2D->desc.Format);


			if (arraySize > 1)
			{
				if (pTexture2D->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
				{
					if (arraySize > 6)
					{
						srv_desc.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
					}
					else
					{
						srv_desc.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
					}
				}
				else
				{
					if (multisampled)
					{
						srv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY; // MSArray?
					}
					else
					{
						srv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
					}
				}

				if (pTexture2D->independentSRVArraySlices)
				{
					if (pTexture2D->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
					{
						UINT slices = arraySize / 6;

						// independent cubemaps
						for (UINT i = 0; i < slices; ++i)
						{
							srv_desc.subresourceRange.baseArrayLayer = i * 6;
							srv_desc.subresourceRange.layerCount = 6;

							pTexture2D->additionalSRVs.push_back(VK_NULL_HANDLE);
							res = vkCreateImageView(device, &srv_desc, nullptr, reinterpret_cast<VkImageView*>(&pTexture2D->additionalSRVs.back()));
							assert(res == VK_SUCCESS);
						}
					}
					else
					{
						UINT slices = arraySize;

						// independent slices
						for (UINT i = 0; i < slices; ++i)
						{
							srv_desc.subresourceRange.baseArrayLayer = i;
							srv_desc.subresourceRange.layerCount = 1;

							pTexture2D->additionalSRVs.push_back(VK_NULL_HANDLE);
							res = vkCreateImageView(device, &srv_desc, nullptr, reinterpret_cast<VkImageView*>(&pTexture2D->additionalSRVs.back()));
							assert(res == VK_SUCCESS);
						}
					}
				}
			}
			else
			{
				if (multisampled)
				{
					srv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D; // MSAA?
				}
				else
				{
					srv_desc.viewType = VK_IMAGE_VIEW_TYPE_2D;

					if (pTexture2D->independentSRVMIPs)
					{
						// Create subresource SRVs:
						UINT miplevels = pTexture2D->desc.MipLevels;
						for (UINT i = 0; i < miplevels; ++i)
						{
							srv_desc.subresourceRange.baseMipLevel = i;
							srv_desc.subresourceRange.levelCount = 1;

							pTexture2D->additionalSRVs.push_back(VK_NULL_HANDLE);
							res = vkCreateImageView(device, &srv_desc, nullptr, reinterpret_cast<VkImageView*>(&pTexture2D->additionalSRVs.back()));
							assert(res == VK_SUCCESS);
						}
					}
				}
			}

			// Create full-resource SRV:
			srv_desc.subresourceRange.baseArrayLayer = 0;
			srv_desc.subresourceRange.layerCount = pTexture2D->desc.ArraySize;
			srv_desc.subresourceRange.baseMipLevel = 0;
			srv_desc.subresourceRange.levelCount = pTexture2D->desc.MipLevels;

			res = vkCreateImageView(device, &srv_desc, nullptr, reinterpret_cast<VkImageView*>(&pTexture2D->SRV));
			hr = res == VK_SUCCESS;
			assert(SUCCEEDED(hr));
		}

		if (pTexture2D->desc.BindFlags & BIND_UNORDERED_ACCESS)
		{
		}


		return hr;
	}
	HRESULT GraphicsDevice_Vulkan::CreateTexture3D(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture3D *pTexture3D)
	{
		DestroyTexture3D(pTexture3D);
		DestroyResource(pTexture3D);
		pTexture3D->type = GPUResource::TEXTURE_3D;
		pTexture3D->Register(this);

		pTexture3D->desc = *pDesc;

		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements, const ShaderByteCode* shaderCode, VertexLayout *pInputLayout)
	{
		DestroyInputLayout(pInputLayout);
		pInputLayout->Register(this);

		pInputLayout->desc.reserve((size_t)NumElements);
		for (UINT i = 0; i < NumElements; ++i)
		{
			pInputLayout->desc.push_back(pInputElementDescs[i]);
		}

		return S_OK;
	}
	HRESULT GraphicsDevice_Vulkan::CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, VertexShader *pVertexShader)
	{
		DestroyVertexShader(pVertexShader);
		pVertexShader->Register(this);

		pVertexShader->code.data = new BYTE[BytecodeLength];
		memcpy(pVertexShader->code.data, pShaderBytecode, BytecodeLength);
		pVertexShader->code.size = BytecodeLength;

		return (pVertexShader->code.data != nullptr && pVertexShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_Vulkan::CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, PixelShader *pPixelShader)
	{
		DestroyPixelShader(pPixelShader);
		pPixelShader->Register(this);

		pPixelShader->code.data = new BYTE[BytecodeLength];
		memcpy(pPixelShader->code.data, pShaderBytecode, BytecodeLength);
		pPixelShader->code.size = BytecodeLength;

		return (pPixelShader->code.data != nullptr && pPixelShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_Vulkan::CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, GeometryShader *pGeometryShader)
	{
		DestroyGeometryShader(pGeometryShader);
		pGeometryShader->Register(this);

		pGeometryShader->code.data = new BYTE[BytecodeLength];
		memcpy(pGeometryShader->code.data, pShaderBytecode, BytecodeLength);
		pGeometryShader->code.size = BytecodeLength;

		return (pGeometryShader->code.data != nullptr && pGeometryShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_Vulkan::CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, HullShader *pHullShader)
	{
		DestroyHullShader(pHullShader);
		pHullShader->Register(this);

		pHullShader->code.data = new BYTE[BytecodeLength];
		memcpy(pHullShader->code.data, pShaderBytecode, BytecodeLength);
		pHullShader->code.size = BytecodeLength;

		return (pHullShader->code.data != nullptr && pHullShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_Vulkan::CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, DomainShader *pDomainShader)
	{
		DestroyDomainShader(pDomainShader);
		pDomainShader->Register(this);

		pDomainShader->code.data = new BYTE[BytecodeLength];
		memcpy(pDomainShader->code.data, pShaderBytecode, BytecodeLength);
		pDomainShader->code.size = BytecodeLength;

		return (pDomainShader->code.data != nullptr && pDomainShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_Vulkan::CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ComputeShader *pComputeShader)
	{
		DestroyComputeShader(pComputeShader);
		pComputeShader->Register(this);

		pComputeShader->code.data = new BYTE[BytecodeLength];
		memcpy(pComputeShader->code.data, pShaderBytecode, BytecodeLength);
		pComputeShader->code.size = BytecodeLength;

		return (pComputeShader->code.data != nullptr && pComputeShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_Vulkan::CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState)
	{
		DestroyBlendState(pBlendState);
		pBlendState->Register(this);

		pBlendState->desc = *pBlendStateDesc;
		return S_OK;
	}
	HRESULT GraphicsDevice_Vulkan::CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState)
	{
		DestroyDepthStencilState(pDepthStencilState);
		pDepthStencilState->Register(this);

		pDepthStencilState->desc = *pDepthStencilStateDesc;
		return S_OK;
	}
	HRESULT GraphicsDevice_Vulkan::CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState)
	{
		DestroyRasterizerState(pRasterizerState);
		pRasterizerState->Register(this);

		pRasterizerState->desc = *pRasterizerStateDesc;
		return S_OK;
	}
	HRESULT GraphicsDevice_Vulkan::CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState)
	{
		DestroySamplerState(pSamplerState);
		pSamplerState->Register(this);

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

		if (vkCreateSampler(device, &createInfo, nullptr, reinterpret_cast<VkSampler*>(&pSamplerState->resource)) != VK_SUCCESS) {
			throw std::runtime_error("failed to create sampler!");
		}

		return S_OK;
	}
	HRESULT GraphicsDevice_Vulkan::CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery)
	{
		// TODO!
		//DestroyQuery(pQuery);
		//pQuery->Register(this);

		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateGraphicsPSO(const GraphicsPSODesc* pDesc, GraphicsPSO* pso)
	{
		DestroyGraphicsPSO(pso);
		pso->Register(this);

		pso->desc = *pDesc;

		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference> colorAttachmentRefs;

		attachments.reserve(pDesc->numRTs);
		colorAttachmentRefs.reserve(pDesc->numRTs);

		// This will be a dummy render pass used for PSO validation:
		VkRenderPass renderPass = VK_NULL_HANDLE;
		{
			uint32_t psoAttachmentCount = pDesc->numRTs + (pDesc->DSFormat == FORMAT_UNKNOWN ? 0 : 1);

			VkAttachmentDescription attachmentDescriptions[9];
			VkAttachmentReference colorAttachmentRefs[9];

			for (UINT i = 0; i < pDesc->numRTs; ++i)
			{
				VkAttachmentDescription attachment = {};
				attachment.format = _ConvertFormat(pDesc->RTFormats[i]);
				attachment.samples = VK_SAMPLE_COUNT_1_BIT;
				attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				attachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
				attachmentDescriptions[i] = attachment;

				VkAttachmentReference ref = {};
				ref.attachment = i;
				ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorAttachmentRefs[i] = ref;
			}


			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = pDesc->numRTs;
			subpass.pColorAttachments = colorAttachmentRefs;

			VkAttachmentDescription depthAttachment = {};
			VkAttachmentReference depthAttachmentRef = {};
			if (pDesc->DSFormat != FORMAT_UNKNOWN)
			{
				depthAttachment.format = _ConvertFormat(pDesc->DSFormat);
				depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
				depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
				depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				depthAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
				attachmentDescriptions[pDesc->numRTs] = depthAttachment;

				depthAttachmentRef.attachment = pDesc->numRTs;
				depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

				subpass.pDepthStencilAttachment = &depthAttachmentRef;
			}

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = psoAttachmentCount;
			renderPassInfo.pAttachments = attachmentDescriptions;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;

			if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
				throw std::runtime_error("failed to create render pass!");
			}
		}



		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = defaultPipelineLayout_Graphics;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;


		// Shaders:

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

		if (pDesc->vs != nullptr)
		{
			VkShaderModuleCreateInfo moduleInfo = {}; 
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleInfo.codeSize = pDesc->vs->code.size;
			moduleInfo.pCode = reinterpret_cast<const uint32_t*>(pDesc->vs->code.data);
			VkShaderModule shaderModule;
			if (vkCreateShaderModule(device, &moduleInfo, nullptr, &shaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module!");
			}

			VkPipelineShaderStageCreateInfo stageInfo = {}; 
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			stageInfo.module = shaderModule;
			stageInfo.pName = "main";

			shaderStages.push_back(stageInfo);
		}

		if (pDesc->hs != nullptr)
		{
			VkShaderModuleCreateInfo moduleInfo = {};
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleInfo.codeSize = pDesc->hs->code.size;
			moduleInfo.pCode = reinterpret_cast<const uint32_t*>(pDesc->hs->code.data);
			VkShaderModule shaderModule;
			if (vkCreateShaderModule(device, &moduleInfo, nullptr, &shaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module!");
			}

			VkPipelineShaderStageCreateInfo stageInfo = {};
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			stageInfo.module = shaderModule;
			stageInfo.pName = "main";

			shaderStages.push_back(stageInfo);
		}

		if (pDesc->ds != nullptr)
		{
			VkShaderModuleCreateInfo moduleInfo = {};
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleInfo.codeSize = pDesc->ds->code.size;
			moduleInfo.pCode = reinterpret_cast<const uint32_t*>(pDesc->ds->code.data);
			VkShaderModule shaderModule;
			if (vkCreateShaderModule(device, &moduleInfo, nullptr, &shaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module!");
			}

			VkPipelineShaderStageCreateInfo stageInfo = {};
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			stageInfo.module = shaderModule;
			stageInfo.pName = "main";

			shaderStages.push_back(stageInfo);
		}

		if (pDesc->gs != nullptr)
		{
			VkShaderModuleCreateInfo moduleInfo = {};
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleInfo.codeSize = pDesc->gs->code.size;
			moduleInfo.pCode = reinterpret_cast<const uint32_t*>(pDesc->gs->code.data);
			VkShaderModule shaderModule;
			if (vkCreateShaderModule(device, &moduleInfo, nullptr, &shaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module!");
			}

			VkPipelineShaderStageCreateInfo stageInfo = {};
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			stageInfo.module = shaderModule;
			stageInfo.pName = "main";

			shaderStages.push_back(stageInfo);
		}

		if (pDesc->ps != nullptr)
		{
			VkShaderModuleCreateInfo moduleInfo = {};
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleInfo.codeSize = pDesc->ps->code.size;
			moduleInfo.pCode = reinterpret_cast<const uint32_t*>(pDesc->ps->code.data);
			VkShaderModule shaderModule;
			if (vkCreateShaderModule(device, &moduleInfo, nullptr, &shaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module!");
			}

			VkPipelineShaderStageCreateInfo stageInfo = {};
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			stageInfo.module = shaderModule;
			stageInfo.pName = "main";

			shaderStages.push_back(stageInfo);
		}

		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineInfo.pStages = shaderStages.data();


		// Fixed function states:

		// Input layout:
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		std::vector<VkVertexInputBindingDescription> bindings;
		std::vector<VkVertexInputAttributeDescription> attributes;
		if (pDesc->il != nullptr)
		{
			uint32_t lastBinding = 0xFFFFFFFF;
			for (auto& x : pDesc->il->desc)
			{
				VkVertexInputBindingDescription bind = {};
				bind.binding = x.InputSlot;
				bind.inputRate = x.InputSlotClass == INPUT_PER_VERTEX_DATA ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
				bind.stride = x.AlignedByteOffset;
				if (bind.stride == VertexLayoutDesc::APPEND_ALIGNED_ELEMENT)
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
			for (auto& x : pDesc->il->desc)
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
				if (attr.offset == VertexLayoutDesc::APPEND_ALIGNED_ELEMENT)
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
		switch (pDesc->pt)
		{
		case POINTLIST:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			break;
		case LINELIST:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
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
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		if (pDesc->rs != nullptr)
		{
			const RasterizerStateDesc& desc = pDesc->rs->desc;

			rasterizer.depthClampEnable = desc.DepthClipEnable;

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
			rasterizer.depthBiasEnable = desc.DepthBias != 0;
			rasterizer.depthBiasConstantFactor = static_cast<float>(desc.DepthBias);
			rasterizer.depthBiasClamp = desc.DepthBiasClamp;
			rasterizer.depthBiasSlopeFactor = desc.SlopeScaledDepthBias;

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
		if (pDesc->dss != nullptr)
		{
			depthstencil.depthTestEnable = pDesc->dss->desc.DepthEnable ? 1 : 0;
			depthstencil.depthWriteEnable = pDesc->dss->desc.DepthWriteMask != DEPTH_WRITE_MASK_ZERO;
			depthstencil.depthCompareOp = _ConvertComparisonFunc(pDesc->dss->desc.DepthFunc);

			// TODO stencil!
			depthstencil.stencilTestEnable = /*pDesc->dss->desc.StencilEnable ? 1 : 0*/ VK_FALSE;
		}

		pipelineInfo.pDepthStencilState = &depthstencil;


		// MSAA:
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		pipelineInfo.pMultisampleState = &multisampling;


		// Blending:
		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(pDesc->numRTs);
		for (size_t i = 0; i < colorBlendAttachments.size(); ++i)
		{
			RenderTargetBlendStateDesc desc = pDesc->bs != nullptr ? pDesc->bs->desc.RenderTarget[i] : RenderTargetBlendStateDesc();

			colorBlendAttachments[i].blendEnable = desc.BlendEnable ? VK_TRUE : VK_FALSE;

			colorBlendAttachments[i].colorWriteMask = 0;
			if (desc.RenderTargetWriteMask & COLOR_WRITE_ENABLE_RED)
			{
				colorBlendAttachments[i].colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
			}
			if (desc.RenderTargetWriteMask & COLOR_WRITE_ENABLE_GREEN)
			{
				colorBlendAttachments[i].colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
			}
			if (desc.RenderTargetWriteMask & COLOR_WRITE_ENABLE_BLUE)
			{
				colorBlendAttachments[i].colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
			}
			if (desc.RenderTargetWriteMask & COLOR_WRITE_ENABLE_ALPHA)
			{
				colorBlendAttachments[i].colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
			}

			colorBlendAttachments[i].srcColorBlendFactor = _ConvertBlend(desc.SrcBlend);
			colorBlendAttachments[i].dstColorBlendFactor = _ConvertBlend(desc.DestBlend);
			colorBlendAttachments[i].colorBlendOp = _ConvertBlendOp(desc.BlendOp);
			colorBlendAttachments[i].srcAlphaBlendFactor = _ConvertBlend(desc.SrcBlendAlpha);
			colorBlendAttachments[i].dstAlphaBlendFactor = _ConvertBlend(desc.DestBlendAlpha);
			colorBlendAttachments[i].alphaBlendOp = _ConvertBlendOp(desc.BlendOpAlpha);
		}

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
		colorBlending.pAttachments = colorBlendAttachments.data();
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
		dynamicState.dynamicStateCount = ARRAYSIZE(dynamicStates);
		dynamicState.pDynamicStates = dynamicStates;

		pipelineInfo.pDynamicState = &dynamicState;


		VkResult res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, reinterpret_cast<VkPipeline*>(&pso->pipeline));
		HRESULT hr = res == VK_SUCCESS ? S_OK : E_FAIL;
		assert(SUCCEEDED(hr));


		// Dummy render pass no longer needed:
		vkDestroyRenderPass(device, renderPass, nullptr);

		return hr;
	}
	HRESULT GraphicsDevice_Vulkan::CreateComputePSO(const ComputePSODesc* pDesc, ComputePSO* pso)
	{
		DestroyComputePSO(pso);
		pso->Register(this);

		pso->desc = *pDesc;

		VkComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = defaultPipelineLayout_Compute;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;



		VkShaderModuleCreateInfo moduleInfo = {};
		moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		VkPipelineShaderStageCreateInfo stageInfo = {};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;

		VkShaderModule shaderModule = {};

		if (pDesc->cs != nullptr)
		{
			moduleInfo.codeSize = pDesc->cs->code.size;
			moduleInfo.pCode = reinterpret_cast<const uint32_t*>(pDesc->cs->code.data);
			if (vkCreateShaderModule(device, &moduleInfo, nullptr, &shaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module!");
			}

			stageInfo.module = shaderModule;
			stageInfo.pName = "main";

			pipelineInfo.stage = stageInfo;
		}


		VkResult res = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, reinterpret_cast<VkPipeline*>(&pso->pipeline));
		HRESULT hr = res == VK_SUCCESS ? S_OK : E_FAIL;
		assert(SUCCEEDED(hr));

		return hr;
	}


	void GraphicsDevice_Vulkan::DestroyResource(GPUResource* pResource)
	{
		vkFreeMemory(device, (VkDeviceMemory)pResource->resourceMemory, nullptr);
		pResource->resourceMemory = WI_NULL_HANDLE;
	}
	void GraphicsDevice_Vulkan::DestroyBuffer(GPUBuffer *pBuffer)
	{
		vkDestroyBuffer(device, (VkBuffer)pBuffer->resource, nullptr);
		pBuffer->resource = WI_NULL_HANDLE;

		vkDestroyBufferView(device, (VkBufferView)pBuffer->SRV, nullptr);
		pBuffer->SRV = WI_NULL_HANDLE;
		for (auto& x : pBuffer->additionalSRVs)
		{
			vkDestroyBufferView(device, (VkBufferView)x, nullptr);
		}
		pBuffer->additionalSRVs.clear();

		vkDestroyBufferView(device, (VkBufferView)pBuffer->UAV, nullptr);
		pBuffer->UAV = WI_NULL_HANDLE;
		for (auto& x : pBuffer->additionalUAVs)
		{
			vkDestroyBufferView(device, (VkBufferView)x, nullptr);
		}
		pBuffer->additionalUAVs.clear();
	}
	void GraphicsDevice_Vulkan::DestroyTexture1D(Texture1D *pTexture1D)
	{
		vkDestroyImage(device, (VkImage)pTexture1D->resource, nullptr);
		pTexture1D->resource = WI_NULL_HANDLE;

		vkDestroyImageView(device, (VkImageView)pTexture1D->RTV, nullptr);
		pTexture1D->RTV = WI_NULL_HANDLE;
		for (auto& x : pTexture1D->additionalRTVs)
		{
			vkDestroyImageView(device, (VkImageView)x, nullptr);
		}
		pTexture1D->additionalRTVs.clear();

		vkDestroyImageView(device, (VkImageView)pTexture1D->SRV, nullptr);
		pTexture1D->SRV = WI_NULL_HANDLE;
		for (auto& x : pTexture1D->additionalSRVs)
		{
			vkDestroyImageView(device, (VkImageView)x, nullptr);
		}
		pTexture1D->additionalSRVs.clear();

		vkDestroyImageView(device, (VkImageView)pTexture1D->UAV, nullptr);
		pTexture1D->UAV = WI_NULL_HANDLE;
		for (auto& x : pTexture1D->additionalUAVs)
		{
			vkDestroyImageView(device, (VkImageView)x, nullptr);
		}
		pTexture1D->additionalUAVs.clear();
	}
	void GraphicsDevice_Vulkan::DestroyTexture2D(Texture2D *pTexture2D)
	{
		vkDestroyImage(device, (VkImage)pTexture2D->resource, nullptr);
		pTexture2D->resource = WI_NULL_HANDLE;

		vkDestroyImageView(device, (VkImageView)pTexture2D->RTV, nullptr);
		pTexture2D->RTV = WI_NULL_HANDLE;
		for (auto& x : pTexture2D->additionalRTVs)
		{
			vkDestroyImageView(device, (VkImageView)x, nullptr);
		}
		pTexture2D->additionalRTVs.clear();

		vkDestroyImageView(device, (VkImageView)pTexture2D->DSV, nullptr);
		pTexture2D->DSV = WI_NULL_HANDLE;
		for (auto& x : pTexture2D->additionalDSVs)
		{
			vkDestroyImageView(device, (VkImageView)x, nullptr);
		}
		pTexture2D->additionalDSVs.clear();

		vkDestroyImageView(device, (VkImageView)pTexture2D->SRV, nullptr);
		pTexture2D->SRV = WI_NULL_HANDLE;
		for (auto& x : pTexture2D->additionalSRVs)
		{
			vkDestroyImageView(device, (VkImageView)x, nullptr);
		}
		pTexture2D->additionalSRVs.clear();

		vkDestroyImageView(device, (VkImageView)pTexture2D->UAV, nullptr);
		pTexture2D->UAV = WI_NULL_HANDLE;
		for (auto& x : pTexture2D->additionalUAVs)
		{
			vkDestroyImageView(device, (VkImageView)x, nullptr);
		}
		pTexture2D->additionalUAVs.clear();
	}
	void GraphicsDevice_Vulkan::DestroyTexture3D(Texture3D *pTexture3D)
	{
		vkDestroyImage(device, (VkImage)pTexture3D->resource, nullptr);
		pTexture3D->resource = WI_NULL_HANDLE;

		vkDestroyImageView(device, (VkImageView)pTexture3D->RTV, nullptr);
		pTexture3D->RTV = WI_NULL_HANDLE;
		for (auto& x : pTexture3D->additionalRTVs)
		{
			vkDestroyImageView(device, (VkImageView)x, nullptr);
		}
		pTexture3D->additionalRTVs.clear();

		vkDestroyImageView(device, (VkImageView)pTexture3D->SRV, nullptr);
		pTexture3D->SRV = WI_NULL_HANDLE;
		for (auto& x : pTexture3D->additionalSRVs)
		{
			vkDestroyImageView(device, (VkImageView)x, nullptr);
		}
		pTexture3D->additionalSRVs.clear();

		vkDestroyImageView(device, (VkImageView)pTexture3D->UAV, nullptr);
		pTexture3D->UAV = WI_NULL_HANDLE;
		for (auto& x : pTexture3D->additionalUAVs)
		{
			vkDestroyImageView(device, (VkImageView)x, nullptr);
		}
		pTexture3D->additionalUAVs.clear();
	}
	void GraphicsDevice_Vulkan::DestroyInputLayout(VertexLayout *pInputLayout)
	{

	}
	void GraphicsDevice_Vulkan::DestroyVertexShader(VertexShader *pVertexShader)
	{

	}
	void GraphicsDevice_Vulkan::DestroyPixelShader(PixelShader *pPixelShader)
	{

	}
	void GraphicsDevice_Vulkan::DestroyGeometryShader(GeometryShader *pGeometryShader)
	{

	}
	void GraphicsDevice_Vulkan::DestroyHullShader(HullShader *pHullShader)
	{

	}
	void GraphicsDevice_Vulkan::DestroyDomainShader(DomainShader *pDomainShader)
	{

	}
	void GraphicsDevice_Vulkan::DestroyComputeShader(ComputeShader *pComputeShader)
	{

	}
	void GraphicsDevice_Vulkan::DestroyBlendState(BlendState *pBlendState)
	{

	}
	void GraphicsDevice_Vulkan::DestroyDepthStencilState(DepthStencilState *pDepthStencilState)
	{

	}
	void GraphicsDevice_Vulkan::DestroyRasterizerState(RasterizerState *pRasterizerState)
	{

	}
	void GraphicsDevice_Vulkan::DestroySamplerState(Sampler *pSamplerState)
	{
		vkDestroySampler(device, (VkSampler)pSamplerState->resource, nullptr);
	}
	void GraphicsDevice_Vulkan::DestroyQuery(GPUQuery *pQuery)
	{

	}
	void GraphicsDevice_Vulkan::DestroyGraphicsPSO(GraphicsPSO* pso)
	{
		vkDestroyPipeline(device, (VkPipeline)pso->pipeline, nullptr);
		pso->pipeline = WI_NULL_HANDLE;
	}
	void GraphicsDevice_Vulkan::DestroyComputePSO(ComputePSO* pso)
	{
		vkDestroyPipeline(device, (VkPipeline)pso->pipeline, nullptr);
		pso->pipeline = WI_NULL_HANDLE;
	}


	void GraphicsDevice_Vulkan::SetName(GPUResource* pResource, const std::string& name)
	{

	}


	void GraphicsDevice_Vulkan::PresentBegin()
	{
		// Sync up copy queue:
		copyQueueLock.lock();
		{
			if (vkEndCommandBuffer(copyCommandBuffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to record copy command buffer!");
			}

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &copyCommandBuffer;

			if (vkQueueSubmit(copyQueue, 1, &submitInfo, copyFence) != VK_SUCCESS) {
				throw std::runtime_error("failed to submit copy command buffer!");
			}

			VkResult res;

			//vkQueueWaitIdle(copyQueue);

			res = vkWaitForFences(device, 1, &copyFence, true, 0xFFFFFFFFFFFFFFFF);
			assert(res == VK_SUCCESS);

			res = vkResetFences(device, 1, &copyFence);
			assert(res == VK_SUCCESS);


			res = vkResetCommandPool(device, copyCommandPool, 0);
			assert(res == VK_SUCCESS);

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional

			res = vkBeginCommandBuffer(copyCommandBuffer, &beginInfo);
			assert(res == VK_SUCCESS);

			bufferUploader->clear();
			textureUploader->clear();
		}
		copyQueueLock.unlock();



		renderPass[GRAPHICSTHREAD_IMMEDIATE].disable(GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE));

		//VkClearValue clearColor = { (FRAMECOUNT % 256) / 255.0f, 0.0f, 0.0f, 1.0f };
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		//VkRenderPassBeginInfo renderPassInfo = {};
		//renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		//renderPassInfo.renderPass = defaultRenderPass;
		//renderPassInfo.framebuffer = GetFrameResources().swapChainFramebuffer;
		//renderPassInfo.renderArea.offset = { 0, 0 };
		//renderPassInfo.renderArea.extent = swapChainExtent;
		//renderPassInfo.clearValueCount = 1;
		//renderPassInfo.pClearValues = &clearColor;

		// Begin presentation render pass...
		//vkCmdBeginRenderPass(GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		
		renderPass[GRAPHICSTHREAD_IMMEDIATE].dirty = true;
		renderPass[GRAPHICSTHREAD_IMMEDIATE].attachmentCount = 1;
		renderPass[GRAPHICSTHREAD_IMMEDIATE].attachments[0] = GetFrameResources().swapChainImageView;
		renderPass[GRAPHICSTHREAD_IMMEDIATE].attachmentsExtents = swapChainExtent;
		renderPass[GRAPHICSTHREAD_IMMEDIATE].clearColor[0] = clearColor;
		renderPass[GRAPHICSTHREAD_IMMEDIATE].overrideRenderPass = defaultRenderPass;
		renderPass[GRAPHICSTHREAD_IMMEDIATE].overrideFramebuffer = GetFrameResources().swapChainFramebuffer;

		renderPass[GRAPHICSTHREAD_IMMEDIATE].validate(device, GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE));


		VkClearAttachment clearInfo = {};
		clearInfo.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		clearInfo.clearValue = clearColor;
		clearInfo.colorAttachment = 0;

		VkClearRect rect = {};
		rect.baseArrayLayer = 0;
		rect.layerCount = 1;
		rect.rect.offset.x = 0;
		rect.rect.offset.y = 0;
		rect.rect.extent.width = SCREENWIDTH;
		rect.rect.extent.height = SCREENHEIGHT;

		vkCmdClearAttachments(GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE), 1, &clearInfo, 1, &rect);


	}
	void GraphicsDevice_Vulkan::PresentEnd()
	{
		VkResult res;

		uint64_t currentframe = GetFrameCount() % BACKBUFFER_COUNT;
		uint32_t imageIndex;
		vkAcquireNextImageKHR(device, swapChain, 0xFFFFFFFFFFFFFFFF, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		assert(imageIndex == currentframe);



		// ...end presentation render pass
		renderPass[GRAPHICSTHREAD_IMMEDIATE].disable(GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE));




		//VkImageMemoryBarrier barrier = {};
		//barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		//barrier.image = swapChainImages[imageIndex];
		//barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		//barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		//barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		//barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		//barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//barrier.subresourceRange.baseArrayLayer = 0;
		//barrier.subresourceRange.layerCount = 1;
		//barrier.subresourceRange.baseMipLevel = 0;
		//barrier.subresourceRange.levelCount = 1;
		//vkCmdPipelineBarrier(
		//	GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE),
		//	VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		//	VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		//	VK_DEPENDENCY_BY_REGION_BIT,
		//	0, nullptr,
		//	0, nullptr,
		//	1, &barrier
		//);



		if (vkEndCommandBuffer(GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE)) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}


		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = GetFrameResources().commandBuffers;

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, GetFrameResources().frameFence) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}


		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional

		vkQueuePresentKHR(presentQueue, &presentInfo);

		//vkQueueWaitIdle(presentQueue);


		// This acts as a barrier, following this we will be using the next frame's resources when calling GetFrameResources()!
		FRAMECOUNT++;


		// Initiate stalling CPU when GPU is behind by more frames than would fit in the backbuffers:
		if (FRAMECOUNT >= BACKBUFFER_COUNT && vkGetFenceStatus(device, GetFrameResources().frameFence) == VK_SUCCESS)
		{
			res = vkWaitForFences(device, 1, &GetFrameResources().frameFence, true, 0xFFFFFFFFFFFFFFFF);
			assert(res == VK_SUCCESS);

			res = vkResetFences(device, 1, &GetFrameResources().frameFence);
			assert(res == VK_SUCCESS);
		}
		
		for (int threadID = 0; threadID < GRAPHICSTHREAD_IMMEDIATE + 1; ++threadID) // todo: all command lists
		{
			res = vkResetCommandPool(device, GetFrameResources().commandPools[threadID], 0);
			assert(res == VK_SUCCESS);

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional

			res = vkBeginCommandBuffer(GetFrameResources().commandBuffers[threadID], &beginInfo);
			assert(res == VK_SUCCESS);

			VkViewport viewports[6];
			for (UINT i = 0; i < ARRAYSIZE(viewports); ++i)
			{
				viewports[i].x = 0;
				viewports[i].y = 0;
				viewports[i].width = static_cast<float>(SCREENWIDTH);
				viewports[i].height = static_cast<float>(SCREENHEIGHT);
				viewports[i].minDepth = 0;
				viewports[i].maxDepth = 1;
			}
			vkCmdSetViewport(GetDirectCommandList(static_cast<GRAPHICSTHREAD>(threadID)), 0, ARRAYSIZE(viewports), viewports);

			VkRect2D scissors[8];
			for (int i = 0; i < ARRAYSIZE(scissors); ++i)
			{
				scissors[i].offset.x = 0;
				scissors[i].offset.y = 0;
				scissors[i].extent.width = 65535;
				scissors[i].extent.height = 65535;
			}
			vkCmdSetScissor(GetDirectCommandList(static_cast<GRAPHICSTHREAD>(threadID)), 0, ARRAYSIZE(scissors), scissors);

			float blendConstants[] = { 1,1,1,1 };
			vkCmdSetBlendConstants(GetDirectCommandList(static_cast<GRAPHICSTHREAD>(threadID)), blendConstants);


			// reset descriptor allocators:
			GetFrameResources().ResourceDescriptorsGPU[threadID]->reset();

			// reset immediate resource allocators:
			GetFrameResources().resourceBuffer[threadID]->clear();

			renderPass[threadID].reset();
		}

		RESOLUTIONCHANGED = false;
	}

	void GraphicsDevice_Vulkan::CreateCommandLists()
	{

	}
	void GraphicsDevice_Vulkan::ExecuteCommandLists()
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = GRAPHICSTHREAD_COUNT - 1;
		submitInfo.pCommandBuffers = &GetFrameResources().commandBuffers[GRAPHICSTHREAD_IMMEDIATE + 1];

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}
	}
	void GraphicsDevice_Vulkan::FinishCommandList(GRAPHICSTHREAD threadID)
	{
		if (threadID == GRAPHICSTHREAD_IMMEDIATE)
			return;
		if (vkEndCommandBuffer(GetDirectCommandList(threadID)) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	void GraphicsDevice_Vulkan::WaitForGPU()
	{
		vkQueueWaitIdle(graphicsQueue);
	}


	void GraphicsDevice_Vulkan::BindScissorRects(UINT numRects, const Rect* rects, GRAPHICSTHREAD threadID) {
		assert(rects != nullptr);
		assert(numRects <= 8);
		VkRect2D scissors[8];
		for(UINT i = 0; i < numRects; ++i) {
			scissors[i].extent.width = abs(rects[i].right - rects[i].left);
			scissors[i].extent.height = abs(rects[i].top - rects[i].bottom);
			scissors[i].offset.x = std::max(0l, rects[i].left);
			scissors[i].offset.y = std::max(0l, rects[i].top);
		}
		vkCmdSetScissor(GetDirectCommandList(threadID), 0, numRects, scissors);
	}
	void GraphicsDevice_Vulkan::BindViewports(UINT NumViewports, const ViewPort *pViewports, GRAPHICSTHREAD threadID)
	{
		assert(NumViewports <= 6);
		VkViewport viewports[6];
		for (UINT i = 0; i < NumViewports; ++i)
		{
			viewports[i].x = pViewports[i].TopLeftX;
			viewports[i].y = pViewports[i].TopLeftY;
			viewports[i].width = pViewports[i].Width;
			viewports[i].height = pViewports[i].Height;
			viewports[i].minDepth = pViewports[i].MinDepth;
			viewports[i].maxDepth = pViewports[i].MaxDepth;
		}
		vkCmdSetViewport(GetDirectCommandList(threadID), 0, NumViewports, viewports);
	}
	void GraphicsDevice_Vulkan::BindRenderTargets(UINT NumViews, const Texture2D* const *ppRenderTargets, const Texture2D* depthStencilTexture, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		assert(NumViews <= 8);
		for (UINT i = 0; i < NumViews; ++i)
		{
			renderPass[threadID].attachments[i] = (VkImageView)ppRenderTargets[i]->RTV;

			renderPass[threadID].attachmentsExtents.width = ppRenderTargets[i]->desc.Width;
			renderPass[threadID].attachmentsExtents.height = ppRenderTargets[i]->desc.Height;
			renderPass[threadID].attachmentLayers = ppRenderTargets[i]->desc.ArraySize;
		}
		renderPass[threadID].attachmentCount = NumViews;

		if (depthStencilTexture != nullptr)
		{
			renderPass[threadID].attachments[renderPass[threadID].attachmentCount] = (VkImageView)depthStencilTexture->DSV;
			renderPass[threadID].attachmentCount++;

			renderPass[threadID].attachmentsExtents.width = depthStencilTexture->desc.Width;
			renderPass[threadID].attachmentsExtents.height = depthStencilTexture->desc.Height;
			renderPass[threadID].attachmentLayers = depthStencilTexture->desc.ArraySize;
		}

		renderPass[threadID].dirty = true;

	}
	void GraphicsDevice_Vulkan::ClearRenderTarget(const Texture* pTexture, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID, int arrayIndex)
	{
		RenderPassManager::ClearRequest clear;

		clear.attachment = (VkImageView)pTexture->RTV;
		clear.clearValue = { ColorRGBA[0], ColorRGBA[1], ColorRGBA[2], ColorRGBA[3] };

		renderPass[threadID].clearRequests.push_back(clear);
	}
	void GraphicsDevice_Vulkan::ClearDepthStencil(const Texture2D* pTexture, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		RenderPassManager::ClearRequest clear;

		clear.attachment = (VkImageView)pTexture->DSV;
		clear.clearValue.depthStencil.depth = Depth;
		clear.clearValue.depthStencil.stencil = Stencil;
		clear.clearFlags = ClearFlags;

		renderPass[threadID].clearRequests.push_back(clear);
	}
	void GraphicsDevice_Vulkan::BindResource(SHADERSTAGE stage, const GPUResource* resource, UINT slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		assert(slot < GPU_RESOURCE_HEAP_SRV_COUNT);

		if (resource != nullptr && resource->resource != VK_NULL_HANDLE)
		{
			if (arrayIndex < 0)
			{
				if (resource->IsTexture() && resource->SRV != VK_NULL_HANDLE)
				{
					// Texture:

					uint32_t binding = VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TEXTURE + slot;

					if (GetFrameResources().ResourceDescriptorsGPU[threadID]->boundDescriptors[stage][binding] == resource->SRV)
					{
						return;
					}

					VkDescriptorImageInfo imageInfo = {};
					imageInfo.imageView = (VkImageView)resource->SRV;
					imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

					VkWriteDescriptorSet descriptorWrite = {};
					descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrite.dstSet = GetFrameResources().ResourceDescriptorsGPU[threadID]->descriptorSet_CPU[stage];
					descriptorWrite.dstBinding = binding;
					descriptorWrite.dstArrayElement = 0;
					descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					descriptorWrite.descriptorCount = 1;
					descriptorWrite.pBufferInfo = nullptr;
					descriptorWrite.pImageInfo = &imageInfo;
					descriptorWrite.pTexelBufferView = nullptr;

					vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
					GetFrameResources().ResourceDescriptorsGPU[threadID]->dirty[stage] = true;
					GetFrameResources().ResourceDescriptorsGPU[threadID]->boundDescriptors[stage][binding] = resource->SRV;
				}
				else
				{
					// Buffer:
					const GPUBuffer* buffer = (const GPUBuffer*)resource;

					if (buffer->desc.Format == FORMAT_UNKNOWN)
					{
						// structured buffer, raw buffer:

						uint32_t binding = VULKAN_DESCRIPTOR_SET_OFFSET_SRV_UNTYPEDBUFFER + slot;

						if (GetFrameResources().ResourceDescriptorsGPU[threadID]->boundDescriptors[stage][binding] == buffer->resource)
						{
							return;
						}

						VkDescriptorBufferInfo bufferInfo = {};
						bufferInfo.buffer = (VkBuffer)buffer->resource;
						bufferInfo.offset = 0;
						bufferInfo.range = buffer->desc.ByteWidth;

						VkWriteDescriptorSet descriptorWrite = {};
						descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						descriptorWrite.dstSet = GetFrameResources().ResourceDescriptorsGPU[threadID]->descriptorSet_CPU[stage];
						descriptorWrite.dstBinding = binding;
						descriptorWrite.dstArrayElement = 0;
						descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						descriptorWrite.descriptorCount = 1;
						descriptorWrite.pBufferInfo = &bufferInfo;
						descriptorWrite.pImageInfo = nullptr;
						descriptorWrite.pTexelBufferView = nullptr;

						vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
						GetFrameResources().ResourceDescriptorsGPU[threadID]->dirty[stage] = true;
						GetFrameResources().ResourceDescriptorsGPU[threadID]->boundDescriptors[stage][binding] = buffer->resource;

					}
					else if (resource->SRV != VK_NULL_HANDLE)
					{
						// typed buffer:

						uint32_t binding = VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TYPEDBUFFER + slot;

						if (GetFrameResources().ResourceDescriptorsGPU[threadID]->boundDescriptors[stage][binding] == resource->SRV)
						{
							return;
						}

						VkWriteDescriptorSet descriptorWrite = {};
						descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						descriptorWrite.dstSet = GetFrameResources().ResourceDescriptorsGPU[threadID]->descriptorSet_CPU[stage];
						descriptorWrite.dstBinding = binding;
						descriptorWrite.dstArrayElement = 0;
						descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
						descriptorWrite.descriptorCount = 1;
						descriptorWrite.pBufferInfo = nullptr;
						descriptorWrite.pImageInfo = nullptr;
						descriptorWrite.pTexelBufferView = reinterpret_cast<const VkBufferView*>(&resource->SRV);

						vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
						GetFrameResources().ResourceDescriptorsGPU[threadID]->dirty[stage] = true;
						GetFrameResources().ResourceDescriptorsGPU[threadID]->boundDescriptors[stage][binding] = resource->SRV;
					}

				}

			}
			else
			{
				assert(resource->additionalSRVs.size() > static_cast<size_t>(arrayIndex) && "Invalid arrayIndex!");

			}
		}
	}
	void GraphicsDevice_Vulkan::BindResources(SHADERSTAGE stage, const GPUResource *const* resources, UINT slot, UINT count, GRAPHICSTHREAD threadID)
	{
		if (resources != nullptr)
		{
			for (UINT i = 0; i < count; ++i)
			{
				BindResource(stage, resources[i], slot + i, threadID, -1);
			}
		}
	}
	void GraphicsDevice_Vulkan::BindUAV(SHADERSTAGE stage, const GPUResource* resource, UINT slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		assert(slot < GPU_RESOURCE_HEAP_UAV_COUNT);

		if (resource != nullptr && resource->resource != VK_NULL_HANDLE)
		{
			if (arrayIndex < 0)
			{
				if (resource->IsTexture() && resource->UAV != VK_NULL_HANDLE)
				{
					// Texture:
					uint32_t binding = VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TEXTURE + slot;

					if (GetFrameResources().ResourceDescriptorsGPU[threadID]->boundDescriptors[stage][binding] == resource->UAV)
					{
						return;
					}

					VkDescriptorImageInfo imageInfo = {};
					imageInfo.imageView = (VkImageView)resource->UAV;
					imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

					VkWriteDescriptorSet descriptorWrite = {};
					descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrite.dstSet = GetFrameResources().ResourceDescriptorsGPU[threadID]->descriptorSet_CPU[stage];
					descriptorWrite.dstBinding = binding;
					descriptorWrite.dstArrayElement = 0;
					descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					descriptorWrite.descriptorCount = 1;
					descriptorWrite.pBufferInfo = nullptr;
					descriptorWrite.pImageInfo = &imageInfo;
					descriptorWrite.pTexelBufferView = nullptr;

					vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
					GetFrameResources().ResourceDescriptorsGPU[threadID]->dirty[stage] = true;
					GetFrameResources().ResourceDescriptorsGPU[threadID]->boundDescriptors[stage][binding] = resource->UAV;
				}
				else
				{
					// Buffer:
					const GPUBuffer* buffer = (const GPUBuffer*)resource;

					if (buffer->desc.Format == FORMAT_UNKNOWN)
					{
						// structured buffer, raw buffer:

						uint32_t binding = VULKAN_DESCRIPTOR_SET_OFFSET_UAV_UNTYPEDBUFFER + slot;

						if (GetFrameResources().ResourceDescriptorsGPU[threadID]->boundDescriptors[stage][binding] == buffer->resource)
						{
							return;
						}

						VkDescriptorBufferInfo bufferInfo = {};
						bufferInfo.buffer = (VkBuffer)buffer->resource;
						bufferInfo.offset = 0;
						bufferInfo.range = buffer->desc.ByteWidth;

						VkWriteDescriptorSet descriptorWrite = {};
						descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						descriptorWrite.dstSet = GetFrameResources().ResourceDescriptorsGPU[threadID]->descriptorSet_CPU[stage];
						descriptorWrite.dstBinding = binding;
						descriptorWrite.dstArrayElement = 0;
						descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						descriptorWrite.descriptorCount = 1;
						descriptorWrite.pBufferInfo = &bufferInfo;
						descriptorWrite.pImageInfo = nullptr;
						descriptorWrite.pTexelBufferView = nullptr;

						vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
						GetFrameResources().ResourceDescriptorsGPU[threadID]->dirty[stage] = true;
						GetFrameResources().ResourceDescriptorsGPU[threadID]->boundDescriptors[stage][binding] = buffer->resource;

					}
					else if (resource->UAV != VK_NULL_HANDLE)
					{
						// typed buffer:

						uint32_t binding = VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TYPEDBUFFER + slot;

						if (GetFrameResources().ResourceDescriptorsGPU[threadID]->boundDescriptors[stage][binding] == resource->UAV)
						{
							return;
						}

						VkWriteDescriptorSet descriptorWrite = {};
						descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						descriptorWrite.dstSet = GetFrameResources().ResourceDescriptorsGPU[threadID]->descriptorSet_CPU[stage];
						descriptorWrite.dstBinding = binding;
						descriptorWrite.dstArrayElement = 0;
						descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
						descriptorWrite.descriptorCount = 1;
						descriptorWrite.pBufferInfo = nullptr;
						descriptorWrite.pImageInfo = nullptr;
						descriptorWrite.pTexelBufferView = reinterpret_cast<const VkBufferView*>(&resource->UAV);

						vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
						GetFrameResources().ResourceDescriptorsGPU[threadID]->dirty[stage] = true;
						GetFrameResources().ResourceDescriptorsGPU[threadID]->boundDescriptors[stage][binding] = resource->UAV;

					}

				}

			}
			else
			{
				//assert(resource->additionalUAVs.size() > static_cast<size_t>(arrayIndex) && "Invalid arrayIndex!");

			}
		}
	}
	void GraphicsDevice_Vulkan::BindUAVs(SHADERSTAGE stage, const GPUResource *const* resources, UINT slot, UINT count, GRAPHICSTHREAD threadID)
	{
		if (resources != nullptr)
		{
			for (UINT i = 0; i < count; ++i)
			{
				BindUAV(stage, resources[i], slot + i, threadID, -1);
			}
		}
	}
	void GraphicsDevice_Vulkan::UnbindResources(UINT slot, UINT num, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::UnbindUAVs(UINT slot, UINT num, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindSampler(SHADERSTAGE stage, const Sampler* sampler, UINT slot, GRAPHICSTHREAD threadID)
	{
		assert(slot < GPU_SAMPLER_HEAP_COUNT);

		if (sampler != nullptr && sampler->resource != VK_NULL_HANDLE)
		{
			uint32_t binding = VULKAN_DESCRIPTOR_SET_OFFSET_SAMPLER + slot;

			if (GetFrameResources().ResourceDescriptorsGPU[threadID]->boundDescriptors[stage][binding] == sampler->resource)
			{
				return;
			}

			VkDescriptorImageInfo imageInfo = {};
			imageInfo.sampler = (VkSampler)sampler->resource;
			imageInfo.imageView = VK_NULL_HANDLE;

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = GetFrameResources().ResourceDescriptorsGPU[threadID]->descriptorSet_CPU[stage];
			descriptorWrite.dstBinding = binding;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = nullptr;
			descriptorWrite.pImageInfo = &imageInfo;
			descriptorWrite.pTexelBufferView = nullptr;

			vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
			GetFrameResources().ResourceDescriptorsGPU[threadID]->dirty[stage] = true;
			GetFrameResources().ResourceDescriptorsGPU[threadID]->boundDescriptors[stage][binding] = sampler->resource;
		}
	}
	void GraphicsDevice_Vulkan::BindConstantBuffer(SHADERSTAGE stage, const GPUBuffer* buffer, UINT slot, GRAPHICSTHREAD threadID)
	{
		assert(slot < GPU_RESOURCE_HEAP_CBV_COUNT);

		if (buffer != nullptr && buffer->resource != VK_NULL_HANDLE)
		{
			uint32_t binding = VULKAN_DESCRIPTOR_SET_OFFSET_CBV + slot;

			if (GetFrameResources().ResourceDescriptorsGPU[threadID]->boundDescriptors[stage][binding] == buffer->resource)
			{
				return;
			}

			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = (VkBuffer)buffer->resource;
			bufferInfo.offset = 0;
			bufferInfo.range = buffer->desc.ByteWidth;

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = GetFrameResources().ResourceDescriptorsGPU[threadID]->descriptorSet_CPU[stage];
			descriptorWrite.dstBinding = binding;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr;
			descriptorWrite.pTexelBufferView = nullptr;

			vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
			GetFrameResources().ResourceDescriptorsGPU[threadID]->dirty[stage] = true;
			GetFrameResources().ResourceDescriptorsGPU[threadID]->boundDescriptors[stage][binding] = buffer->resource;
		}
	}
	void GraphicsDevice_Vulkan::BindVertexBuffers(const GPUBuffer *const* vertexBuffers, UINT slot, UINT count, const UINT* strides, const UINT* offsets, GRAPHICSTHREAD threadID)
	{
		VkDeviceSize voffsets[8] = {};
		VkBuffer vbuffers[8] = {};
		assert(count <= 8);
		for (UINT i = 0; i < count; ++i)
		{
			vbuffers[i] = (vertexBuffers[i] == nullptr ? nullBuffer : (VkBuffer)vertexBuffers[i]->resource);
			if (offsets != nullptr)
			{
				voffsets[i] = offsets[i];
			}
		}

		vkCmdBindVertexBuffers(GetDirectCommandList(threadID), static_cast<uint32_t>(slot), static_cast<uint32_t>(count), vbuffers, voffsets);
	}
	void GraphicsDevice_Vulkan::BindIndexBuffer(const GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, UINT offset, GRAPHICSTHREAD threadID)
	{
		if (indexBuffer != nullptr)
		{
			vkCmdBindIndexBuffer(GetDirectCommandList(threadID), (VkBuffer)indexBuffer->resource, offset, format == INDEXFORMAT_16BIT ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
		}
	}
	void GraphicsDevice_Vulkan::BindStencilRef(UINT value, GRAPHICSTHREAD threadID)
	{
		vkCmdSetStencilReference(GetDirectCommandList(threadID), VK_STENCIL_FRONT_AND_BACK, value);
	}
	void GraphicsDevice_Vulkan::BindBlendFactor(float r, float g, float b, float a, GRAPHICSTHREAD threadID)
	{
		float blendConstants[] = { r, g, b, a };
		vkCmdSetBlendConstants(GetDirectCommandList(threadID), blendConstants);
	}
	void GraphicsDevice_Vulkan::BindGraphicsPSO(const GraphicsPSO* pso, GRAPHICSTHREAD threadID)
	{
		vkCmdBindPipeline(GetDirectCommandList(threadID), VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)pso->pipeline);
		renderPass[threadID].pDesc = &pso->desc;
	}
	void GraphicsDevice_Vulkan::BindComputePSO(const ComputePSO* pso, GRAPHICSTHREAD threadID)
	{
		vkCmdBindPipeline(GetDirectCommandList(threadID), VK_PIPELINE_BIND_POINT_COMPUTE, (VkPipeline)pso->pipeline);
	}
	void GraphicsDevice_Vulkan::Draw(UINT vertexCount, UINT startVertexLocation, GRAPHICSTHREAD threadID)
	{
		renderPass[threadID].validate(device, GetDirectCommandList(threadID));
		GetFrameResources().ResourceDescriptorsGPU[threadID]->validate(GetDirectCommandList(threadID));
		vkCmdDraw(GetDirectCommandList(threadID), static_cast<uint32_t>(vertexCount), 1, startVertexLocation, 0);
	}
	void GraphicsDevice_Vulkan::DrawIndexed(UINT indexCount, UINT startIndexLocation, UINT baseVertexLocation, GRAPHICSTHREAD threadID)
	{
		renderPass[threadID].validate(device, GetDirectCommandList(threadID));
		GetFrameResources().ResourceDescriptorsGPU[threadID]->validate(GetDirectCommandList(threadID));
		vkCmdDrawIndexed(GetDirectCommandList(threadID), static_cast<uint32_t>(indexCount), 1, startIndexLocation, baseVertexLocation, 0);
	}
	void GraphicsDevice_Vulkan::DrawInstanced(UINT vertexCount, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID)
	{
		renderPass[threadID].validate(device, GetDirectCommandList(threadID));
		GetFrameResources().ResourceDescriptorsGPU[threadID]->validate(GetDirectCommandList(threadID));
		vkCmdDraw(GetDirectCommandList(threadID), static_cast<uint32_t>(vertexCount), static_cast<uint32_t>(instanceCount), startVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_Vulkan::DrawIndexedInstanced(UINT indexCount, UINT instanceCount, UINT startIndexLocation, UINT baseVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID)
	{
		renderPass[threadID].validate(device, GetDirectCommandList(threadID));
		GetFrameResources().ResourceDescriptorsGPU[threadID]->validate(GetDirectCommandList(threadID));
		vkCmdDrawIndexed(GetDirectCommandList(threadID), static_cast<uint32_t>(indexCount), static_cast<uint32_t>(instanceCount), startIndexLocation, baseVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_Vulkan::DrawInstancedIndirect(const GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::DrawIndexedInstancedIndirect(const GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ, GRAPHICSTHREAD threadID)
	{
		renderPass[threadID].disable(GetDirectCommandList(threadID));

		GetFrameResources().ResourceDescriptorsGPU[threadID]->validate(GetDirectCommandList(threadID));
		vkCmdDispatch(GetDirectCommandList(threadID), threadGroupCountX, threadGroupCountY, threadGroupCountZ);

		VkMemoryBarrier barrier;
		barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		barrier.pNext = nullptr;
		barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		vkCmdPipelineBarrier(GetDirectCommandList(threadID), 
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
			0, 
			1, &barrier, 
			0, nullptr, 
			0, nullptr);
	}
	void GraphicsDevice_Vulkan::DispatchIndirect(const GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::CopyTexture2D(const Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID)
	{
		VkImageCopy copy;
		copy.extent.width = pDst->desc.Width;
		copy.extent.height = pDst->desc.Height;
		copy.extent.depth = 1;

		copy.srcOffset.x = 0;
		copy.srcOffset.y = 0;
		copy.srcOffset.z = 0;

		copy.dstOffset.x = 0;
		copy.dstOffset.y = 0;
		copy.dstOffset.z = 0;

		copy.srcSubresource.aspectMask = pSrc->desc.BindFlags & BIND_DEPTH_STENCIL ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		copy.srcSubresource.baseArrayLayer = 0;
		copy.srcSubresource.layerCount = 1;
		copy.srcSubresource.mipLevel = 0;

		copy.dstSubresource.aspectMask = pDst->desc.BindFlags & BIND_DEPTH_STENCIL ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		copy.dstSubresource.baseArrayLayer = 0;
		copy.dstSubresource.layerCount = 1;
		copy.dstSubresource.mipLevel = 0;

		vkCmdCopyImage(GetDirectCommandList(threadID),
			(VkImage)pSrc->resource, VK_IMAGE_LAYOUT_GENERAL,
			(VkImage)pDst->resource, VK_IMAGE_LAYOUT_GENERAL,
			1, &copy);
	}
	void GraphicsDevice_Vulkan::CopyTexture2D_Region(const Texture2D* pDst, UINT dstMip, UINT dstX, UINT dstY, const Texture2D* pSrc, UINT srcMip, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::MSAAResolve(const Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::UpdateBuffer(const GPUBuffer* buffer, const void* data, GRAPHICSTHREAD threadID, int dataSize)
	{
		// This will fully update the buffer on the GPU timeline
		//	But on the CPU side we need to keep the in flight data versioned, and we use the temporary buffer for that
		//	This is like a USAGE_DEFAULT buffer updater on DX11
		// TODO: USAGE_DYNAMIC would not use GPU copies, but then it would need to handle assigning descriptor pointer dynamically. 
		//	However, now descriptors are created ahead of time in CreateBuffer

		assert(buffer->desc.Usage != USAGE_IMMUTABLE && "Cannot update IMMUTABLE GPUBuffer!");
		assert((int)buffer->desc.ByteWidth >= dataSize || dataSize < 0 && "Data size is too big!");

		if (dataSize == 0)
		{
			return;
		}

		dataSize = std::min((int)buffer->desc.ByteWidth, dataSize);
		dataSize = (dataSize >= 0 ? dataSize : buffer->desc.ByteWidth);



		renderPass[threadID].disable(GetDirectCommandList(threadID));




		VkPipelineStageFlags stages = 0;

		VkBufferMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.buffer = (VkBuffer)buffer->resource;
		barrier.srcAccessMask = 0;
		if (buffer->desc.BindFlags & BIND_CONSTANT_BUFFER)
		{
			barrier.srcAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
			stages = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}
		else if (buffer->desc.BindFlags & BIND_VERTEX_BUFFER)
		{
			barrier.srcAccessMask = VK_ACCESS_INDEX_READ_BIT;
			stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		}
		else if (buffer->desc.BindFlags & BIND_INDEX_BUFFER)
		{
			barrier.srcAccessMask = VK_ACCESS_INDEX_READ_BIT;
			stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		}
		else
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			stages = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		vkCmdPipelineBarrier(
			GetDirectCommandList(threadID),
			stages,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_DEPENDENCY_BY_REGION_BIT,
			0, nullptr,
			1, &barrier,
			0, nullptr
		);


		// issue data copy:
		uint8_t* dest = GetFrameResources().resourceBuffer[threadID]->allocate(dataSize, 256);
		memcpy(dest, data, dataSize);

		VkBufferCopy copyRegion = {};
		copyRegion.size = dataSize;
		copyRegion.srcOffset = GetFrameResources().resourceBuffer[threadID]->calculateOffset(dest);
		copyRegion.dstOffset = 0;

		vkCmdCopyBuffer(GetDirectCommandList(threadID), (VkBuffer)GetFrameResources().resourceBuffer[threadID]->buffer.resource, 
			(VkBuffer)buffer->resource, 1, &copyRegion);






		VkAccessFlags tmp = barrier.srcAccessMask;
		barrier.srcAccessMask = barrier.dstAccessMask;
		barrier.dstAccessMask = tmp;

		vkCmdPipelineBarrier(
			GetDirectCommandList(threadID),
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			stages,
			VK_DEPENDENCY_BY_REGION_BIT,
			0, nullptr,
			1, &barrier,
			0, nullptr
		);


	}
	bool GraphicsDevice_Vulkan::DownloadResource(const GPUResource* resourceToDownload, const GPUResource* resourceDest, void* dataDest, GRAPHICSTHREAD threadID)
	{
		return false;
	}

	void GraphicsDevice_Vulkan::QueryBegin(const GPUQuery *query, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::QueryEnd(const GPUQuery *query, GRAPHICSTHREAD threadID)
	{
	}
	bool GraphicsDevice_Vulkan::QueryRead(const GPUQuery* query, GPUQueryResult* result)
	{
		return true;
	}

	void GraphicsDevice_Vulkan::UAVBarrier(const GPUResource *const* uavs, UINT NumBarriers, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::TransitionBarrier(const GPUResource *const* resources, UINT NumBarriers, RESOURCE_STATES stateBefore, RESOURCE_STATES stateAfter, GRAPHICSTHREAD threadID)
	{
		renderPass[threadID].disable(GetDirectCommandList(threadID));

		//if (stateBefore == RESOURCE_STATE_UNORDERED_ACCESS && stateAfter == RESOURCE_STATE_GENERIC_READ)
		//{
		//	VkBufferMemoryBarrier barrier = {};
		//	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		//	barrier.pNext = nullptr;
		//	barrier.buffer = static_cast<VkBuffer>(resources[0]->resource);
		//	barrier.size = ((GPUBuffer*)resources[0])->desc.ByteWidth;
		//	barrier.offset = 0;
		//	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		//	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		//	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		//	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		//	vkCmdPipelineBarrier(GetDirectCommandList(threadID),
		//		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		//		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		//		0,
		//		0, nullptr,
		//		1, &barrier,
		//		0, nullptr);
		//}
		//else if (stateBefore == RESOURCE_STATE_UNORDERED_ACCESS && stateAfter == RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
		//{
		//	VkBufferMemoryBarrier barrier = {};
		//	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		//	barrier.pNext = nullptr;
		//	barrier.buffer = static_cast<VkBuffer>(resources[0]->resource);
		//	barrier.size = ((GPUBuffer*)resources[0])->desc.ByteWidth;
		//	barrier.offset = 0;
		//	barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		//	barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		//	vkCmdPipelineBarrier(GetDirectCommandList(threadID),
		//		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		//		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		//		0,
		//		0, nullptr,
		//		1, &barrier,
		//		0, nullptr);
		//}



		//for (UINT i = 0; i < NumBarriers; ++i)
		//{

		//	if (stateBefore == RESOURCE_STATE_RENDER_TARGET)
		//	{
		//		Texture* tex = dynamic_cast<Texture*>(resources[i]);

		//		if (tex != nullptr)
		//		{
		//			VkImageMemoryBarrier barrier = {};
		//			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		//			barrier.image = static_cast<VkImage>(tex->resource);
		//			barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		//			barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		//			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		//			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		//			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//			barrier.subresourceRange.baseArrayLayer = 0;
		//			barrier.subresourceRange.layerCount = 1;
		//			barrier.subresourceRange.baseMipLevel = 0;
		//			barrier.subresourceRange.levelCount = 1;
		//			vkCmdPipelineBarrier(
		//				GetDirectCommandList(threadID),
		//				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		//				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		//				VK_DEPENDENCY_BY_REGION_BIT,
		//				0, nullptr,
		//				0, nullptr,
		//				1, &barrier
		//			);
		//		}
		//	}

		//}

	}

	GraphicsDevice::GPUAllocation GraphicsDevice_Vulkan::AllocateGPU(size_t dataSize, GRAPHICSTHREAD threadID)
	{
		// This case allocates a CPU write access and GPU read access memory from the temporary buffer
		// The application can write into this, but better to not read from it

		FrameResources::ResourceFrameAllocator& allocator = *GetFrameResources().resourceBuffer[threadID];
		GPUAllocation result;

		if (dataSize == 0)
		{
			return result;
		}

		uint8_t* dest = allocator.allocate(dataSize, 256);

		assert(dest != nullptr); // todo: this needs to be handled as well

		result.buffer = &allocator.buffer;
		result.offset = (UINT)allocator.calculateOffset(dest);
		result.data = (void*)dest;
		return result;
	}

	void GraphicsDevice_Vulkan::EventBegin(const std::string& name, GRAPHICSTHREAD threadID)
	{
		//PFN_vkCmdDebugMarkerBeginEXT pfnCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");

		//VkDebugMarkerMarkerInfoEXT info = {};
		//info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		//info.pNext = nullptr;
		//info.color[0] = 1;
		//info.color[1] = 0;
		//info.color[2] = 1;
		//info.color[3] = 1;
		//info.pMarkerName = name.c_str();

		//pfnCmdDebugMarkerBeginEXT(GetDirectCommandList(threadID), &info);

		////vkCmdDebugMarkerBeginEXT(GetDirectCommandList(threadID), &info);
	}
	void GraphicsDevice_Vulkan::EventEnd(GRAPHICSTHREAD threadID)
	{
		//PFN_vkCmdDebugMarkerEndEXT pfnCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
		//pfnCmdDebugMarkerEndEXT(GetDirectCommandList(threadID));

		////vkCmdDebugMarkerEndEXT(GetDirectCommandList(threadID));
	}
	void GraphicsDevice_Vulkan::SetMarker(const std::string& name, GRAPHICSTHREAD threadID)
	{
		//PFN_vkCmdDebugMarkerInsertEXT pfnCmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");

		//VkDebugMarkerMarkerInfoEXT info = {};
		//info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		//info.pNext = nullptr;
		//info.color[0] = 1;
		//info.color[1] = 1;
		//info.color[2] = 0;
		//info.color[3] = 1;
		//info.pMarkerName = name.c_str();

		//pfnCmdDebugMarkerInsertEXT(GetDirectCommandList(threadID), &info);

		////vkCmdDebugMarkerInsertEXT(GetDirectCommandList(threadID), &info);
	}

}


#endif // WICKEDENGINE_BUILD_VULKAN
