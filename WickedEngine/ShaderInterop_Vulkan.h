#ifndef WI_SHADERINTEROP_VULKAN_H
#define WI_SHADERINTEROP_VULKAN_H

#include "wiGraphicsDevice_SharedInternals.h"

// These shifts are made so that Vulkan resource bindings slots don't interfere with each other across shader stages:
//	These are compiled into shaders and GraphicsDevice_Vulkan, so rebuilding those will be necessary after modifications
#define VULKAN_BINDING_SHIFT_S 0
#define VULKAN_BINDING_SHIFT_B (VULKAN_BINDING_SHIFT_S + GPU_SAMPLER_HEAP_COUNT)
#define VULKAN_BINDING_SHIFT_U (VULKAN_BINDING_SHIFT_B + GPU_RESOURCE_HEAP_CBV_COUNT)
#define VULKAN_BINDING_SHIFT_T (VULKAN_BINDING_SHIFT_U + GPU_RESOURCE_HEAP_UAV_COUNT)
#define VULKAN_BINDING_SHIFT_STAGE(stage) ((stage) * (VULKAN_BINDING_SHIFT_T + GPU_RESOURCE_HEAP_SRV_COUNT))

#endif // WI_SHADERINTEROP_VULKAN_H

