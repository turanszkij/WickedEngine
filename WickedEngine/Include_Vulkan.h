#ifndef _INCLUDE_VULKAN_H_
#define _INCLUDE_VULKAN_H_

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif // WIN32

#ifdef WICKEDENGINE_BUILD_VULKAN
#include <vulkan/vulkan.h>
#endif // WICKEDENGINE_BUILD_VULKAN

#endif // _INCLUDE_VULKAN_H_
