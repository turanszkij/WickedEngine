#define _CRT_SECURE_NO_WARNINGS
#include "wiGraphicsDevice_Vulkan.h"
#include "wiHelper.h"
#include "ResourceMapping.h"

#include <sstream>

using namespace std;



#ifdef WICKEDENGINE_BUILD_VULKAN
#pragma comment(lib,"vulkan-1.lib")

bool in_callback = false;
#define ERR_EXIT(err_msg, err_class)                                             \
    do {                                                                         \
        if (!this->suppress_popups) MessageBoxA(NULL, err_msg, err_class, MB_OK); \
        exit(1);                                                                 \
    } while (0)
void DbgMsg(char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	printf(fmt, va);
	fflush(stdout);
	va_end(va);
}
VKAPI_ATTR VkBool32 VKAPI_CALL BreakCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
	size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg,
	void *pUserData) {
#ifndef WIN32
	raise(SIGTRAP);
#else
	DebugBreak();
#endif

	return false;
}

static int validation_error = 0;
VKAPI_ATTR VkBool32 VKAPI_CALL dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location,
	int32_t msgCode, const char *pLayerPrefix, const char *pMsg, void *pUserData) {
	// clang-format off
	char *message = (char *)malloc(strlen(pMsg) + 100);

	assert(message);

	if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
		sprintf(message, "INFORMATION: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		validation_error = 1;
	}
	else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		sprintf(message, "WARNING: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		validation_error = 1;
	}
	else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
		sprintf(message, "PERFORMANCE WARNING: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		validation_error = 1;
	}
	else if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		sprintf(message, "ERROR: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		validation_error = 1;
	}
	else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
		sprintf(message, "DEBUG: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		validation_error = 1;
	}
	else {
		sprintf(message, "INFORMATION: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		validation_error = 1;
	}

#ifdef _WIN32

	in_callback = true;
	struct demo *demo = (struct demo*) pUserData;
	MessageBoxA(NULL, message, "Alert", MB_OK);
	in_callback = false;

#elif defined(ANDROID)

	if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
		__android_log_print(ANDROID_LOG_INFO, APP_SHORT_NAME, "%s", message);
	}
	else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		__android_log_print(ANDROID_LOG_WARN, APP_SHORT_NAME, "%s", message);
	}
	else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
		__android_log_print(ANDROID_LOG_WARN, APP_SHORT_NAME, "%s", message);
	}
	else if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		__android_log_print(ANDROID_LOG_ERROR, APP_SHORT_NAME, "%s", message);
	}
	else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
		__android_log_print(ANDROID_LOG_DEBUG, APP_SHORT_NAME, "%s", message);
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, APP_SHORT_NAME, "%s", message);
	}

#else

	printf("%s\n", message);
	fflush(stdout);

#endif

	free(message);

	// clang-format on

	/*
	* false indicates that layer should not bail-out of an
	* API call that had validation failures. This may mean that the
	* app dies inside the driver due to invalid parameter(s).
	* That's what would happen without validation layers, so we'll
	* keep that behavior here.
	*/
	return false;
}


static VkBool32 demo_check_layers(uint32_t check_count, char **check_names,
	uint32_t layer_count,
	VkLayerProperties *layers) {
	for (uint32_t i = 0; i < check_count; i++) {
		VkBool32 found = 0;
		for (uint32_t j = 0; j < layer_count; j++) {
			if (!strcmp(check_names[i], layers[j].layerName)) {
				found = 1;
				break;
			}
		}
		if (!found) {
			fprintf(stderr, "Cannot find layer: %s\n", check_names[i]);
			return 0;
		}
	}
	return 1;
}

namespace wiGraphicsTypes
{
	// Engine functions
	GraphicsDevice_Vulkan::GraphicsDevice_Vulkan(wiWindowRegistration::window_type window, bool fullscreen) : GraphicsDevice()
	{
		FULLSCREEN = fullscreen;

		RECT rect = RECT();
		GetClientRect(window, &rect);
		SCREENWIDTH = rect.right - rect.left;
		SCREENHEIGHT = rect.bottom - rect.top;

		VkResult err;
		uint32_t instance_extension_count = 0;
		uint32_t instance_layer_count = 0;
		uint32_t validation_layer_count = 0;
		char **instance_validation_layers = NULL;

		this->enabled_extension_count = 0;
		this->enabled_layer_count = 0;
		this->validate = true;

		char *instance_validation_layers_alt1[] = {
			"VK_LAYER_LUNARG_standard_validation"
		};

		char *instance_validation_layers_alt2[] = {
			"VK_LAYER_GOOGLE_threading",      "VK_LAYER_LUNARG_parameter_validation",
			"VK_LAYER_LUNARG_object_tracker", "VK_LAYER_LUNARG_core_validation",
			"VK_LAYER_GOOGLE_unique_objects"
		};

		/* Look for validation layers */
		VkBool32 validation_found = 0;
		if (validate) {

			err = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
			assert(!err);

			instance_validation_layers = instance_validation_layers_alt1;
			if (instance_layer_count > 0) {
				VkLayerProperties *instance_layers =
					(VkLayerProperties*)malloc(sizeof(VkLayerProperties) * instance_layer_count);
				err = vkEnumerateInstanceLayerProperties(&instance_layer_count,
					instance_layers);
				assert(!err);


				validation_found = demo_check_layers(
					ARRAYSIZE(instance_validation_layers_alt1),
					instance_validation_layers, instance_layer_count,
					instance_layers);
				if (validation_found) {
					this->enabled_layer_count = ARRAYSIZE(instance_validation_layers_alt1);
					this->enabled_layers[0] = "VK_LAYER_LUNARG_standard_validation";
					validation_layer_count = 1;
				}
				else {
					// use alternative set of validation layers
					instance_validation_layers = instance_validation_layers_alt2;
					this->enabled_layer_count = ARRAYSIZE(instance_validation_layers_alt2);
					validation_found = demo_check_layers(
						ARRAYSIZE(instance_validation_layers_alt2),
						instance_validation_layers, instance_layer_count,
						instance_layers);
					validation_layer_count =
						ARRAYSIZE(instance_validation_layers_alt2);
					for (uint32_t i = 0; i < validation_layer_count; i++) {
						this->enabled_layers[i] = instance_validation_layers[i];
					}
				}
				free(instance_layers);
			}

			if (!validation_found) {
				ERR_EXIT("vkEnumerateInstanceLayerProperties failed to find "
					"required validation layer.\n\n"
					"Please look at the Getting Started guide for additional "
					"information.\n",
					"vkCreateInstance Failure");
			}
		}

		/* Look for instance extensions */
		VkBool32 surfaceExtFound = 0;
		VkBool32 platformSurfaceExtFound = 0;
		memset(this->extension_names, 0, sizeof(this->extension_names));

		err = vkEnumerateInstanceExtensionProperties(
			NULL, &instance_extension_count, NULL);
		assert(!err);

		if (instance_extension_count > 0) {
			VkExtensionProperties *instance_extensions =
				(VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * instance_extension_count);
			err = vkEnumerateInstanceExtensionProperties(
				NULL, &instance_extension_count, instance_extensions);
			assert(!err);
			for (uint32_t i = 0; i < instance_extension_count; i++) {
				if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME,
					instance_extensions[i].extensionName)) {
					surfaceExtFound = 1;
					this->extension_names[this->enabled_extension_count++] =
						VK_KHR_SURFACE_EXTENSION_NAME;
				}
#if defined(VK_USE_PLATFORM_WIN32_KHR)
				if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
					instance_extensions[i].extensionName)) {
					platformSurfaceExtFound = 1;
					this->extension_names[this->enabled_extension_count++] =
						VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
				}
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
				if (!strcmp(VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
					instance_extensions[i].extensionName)) {
					platformSurfaceExtFound = 1;
					this->extension_names[this->enabled_extension_count++] =
						VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
				}
#elif defined(VK_USE_PLATFORM_XCB_KHR)
				if (!strcmp(VK_KHR_XCB_SURFACE_EXTENSION_NAME,
					instance_extensions[i].extensionName)) {
					platformSurfaceExtFound = 1;
					this->extension_names[this->enabled_extension_count++] =
						VK_KHR_XCB_SURFACE_EXTENSION_NAME;
				}
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
				if (!strcmp(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
					instance_extensions[i].extensionName)) {
					platformSurfaceExtFound = 1;
					this->extension_names[this->enabled_extension_count++] =
						VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
				}
#elif defined(VK_USE_PLATFORM_MIR_KHR)
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
				if (!strcmp(VK_KHR_DISPLAY_EXTENSION_NAME,
					instance_extensions[i].extensionName)) {
					platformSurfaceExtFound = 1;
					this->extension_names[this->enabled_extension_count++] =
						VK_KHR_DISPLAY_EXTENSION_NAME;
				}
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
				if (!strcmp(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
					instance_extensions[i].extensionName)) {
					platformSurfaceExtFound = 1;
					this->extension_names[this->enabled_extension_count++] =
						VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
				}
#elif defined(VK_USE_PLATFORM_IOS_MVK)
				if (!strcmp(VK_MVK_IOS_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName)) {
					platformSurfaceExtFound = 1;
					this->extension_names[this->enabled_extension_count++] = VK_MVK_IOS_SURFACE_EXTENSION_NAME;
				}
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
				if (!strcmp(VK_MVK_MACOS_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName)) {
					platformSurfaceExtFound = 1;
					this->extension_names[this->enabled_extension_count++] = VK_MVK_MACOS_SURFACE_EXTENSION_NAME;
				}
#endif
				if (!strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
					instance_extensions[i].extensionName)) {
					if (this->validate) {
						this->extension_names[this->enabled_extension_count++] =
							VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
					}
				}
				assert(this->enabled_extension_count < 64);
			}

			free(instance_extensions);
		}

		if (!surfaceExtFound) {
			ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
				"the " VK_KHR_SURFACE_EXTENSION_NAME
				" extension.\n\nDo you have a compatible "
				"Vulkan installable client driver (ICD) installed?\nPlease "
				"look at the Getting Started guide for additional "
				"information.\n",
				"vkCreateInstance Failure");
		}
		if (!platformSurfaceExtFound) {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
			ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
				"the " VK_KHR_WIN32_SURFACE_EXTENSION_NAME
				" extension.\n\nDo you have a compatible "
				"Vulkan installable client driver (ICD) installed?\nPlease "
				"look at the Getting Started guide for additional "
				"information.\n",
				"vkCreateInstance Failure");
#elif defined(VK_USE_PLATFORM_IOS_MVK)
			ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find the "
				VK_MVK_IOS_SURFACE_EXTENSION_NAME" extension.\n\nDo you have a compatible "
				"Vulkan installable client driver (ICD) installed?\nPlease "
				"look at the Getting Started guide for additional "
				"information.\n",
				"vkCreateInstance Failure");
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
			ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find the "
				VK_MVK_MACOS_SURFACE_EXTENSION_NAME" extension.\n\nDo you have a compatible "
				"Vulkan installable client driver (ICD) installed?\nPlease "
				"look at the Getting Started guide for additional "
				"information.\n",
				"vkCreateInstance Failure");
#elif defined(VK_USE_PLATFORM_XCB_KHR)
			ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
				"the " VK_KHR_XCB_SURFACE_EXTENSION_NAME
				" extension.\n\nDo you have a compatible "
				"Vulkan installable client driver (ICD) installed?\nPlease "
				"look at the Getting Started guide for additional "
				"information.\n",
				"vkCreateInstance Failure");
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
			ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
				"the " VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
				" extension.\n\nDo you have a compatible "
				"Vulkan installable client driver (ICD) installed?\nPlease "
				"look at the Getting Started guide for additional "
				"information.\n",
				"vkCreateInstance Failure");
#elif defined(VK_USE_PLATFORM_MIR_KHR)
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
			ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
				"the " VK_KHR_DISPLAY_EXTENSION_NAME
				" extension.\n\nDo you have a compatible "
				"Vulkan installable client driver (ICD) installed?\nPlease "
				"look at the Getting Started guide for additional "
				"information.\n",
				"vkCreateInstance Failure");
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
			ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
				"the " VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
				" extension.\n\nDo you have a compatible "
				"Vulkan installable client driver (ICD) installed?\nPlease "
				"look at the Getting Started guide for additional "
				"information.\n",
				"vkCreateInstance Failure");
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
			ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
				"the " VK_KHR_XLIB_SURFACE_EXTENSION_NAME
				" extension.\n\nDo you have a compatible "
				"Vulkan installable client driver (ICD) installed?\nPlease "
				"look at the Getting Started guide for additional "
				"information.\n",
				"vkCreateInstance Failure");
#endif
		}
		VkApplicationInfo app;
		app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app.pNext = NULL;
		app.pApplicationName = "WickedEngine";
		app.applicationVersion = 0;
		app.pEngineName = "Wickedengine";
		app.engineVersion = 0;
		app.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo inst_info;
		inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		inst_info.pNext = NULL;
		inst_info.pApplicationInfo = &app;
		inst_info.enabledLayerCount = this->enabled_layer_count;
		inst_info.ppEnabledLayerNames = (const char *const *)instance_validation_layers;
		inst_info.enabledExtensionCount = this->enabled_extension_count;
		inst_info.ppEnabledExtensionNames = (const char *const *)this->extension_names;

		/*
		* This is info for a temp callback to use during CreateInstance.
		* After the instance is created, we use the instance-based
		* function to register the final callback.
		*/
		VkDebugReportCallbackCreateInfoEXT dbgCreateInfoTemp;
		VkValidationFlagsEXT val_flags;
		if (this->validate) {
			dbgCreateInfoTemp.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
			dbgCreateInfoTemp.pNext = NULL;
			dbgCreateInfoTemp.pfnCallback = this->use_break ? BreakCallback : dbgFunc;
			dbgCreateInfoTemp.pUserData = this;
			dbgCreateInfoTemp.flags =
				VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
			if (this->validate_checks_disabled) {
				val_flags.sType = VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT;
				val_flags.pNext = NULL;
				val_flags.disabledValidationCheckCount = 1;
				VkValidationCheckEXT disabled_check = VK_VALIDATION_CHECK_ALL_EXT;
				val_flags.pDisabledValidationChecks = &disabled_check;
				dbgCreateInfoTemp.pNext = (void*)&val_flags;
			}
			inst_info.pNext = &dbgCreateInfoTemp;
		}

		uint32_t gpu_count;

		err = vkCreateInstance(&inst_info, NULL, &this->inst);
		if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
			ERR_EXIT("Cannot find a compatible Vulkan installable client driver "
				"(ICD).\n\nPlease look at the Getting Started guide for "
				"additional information.\n",
				"vkCreateInstance Failure");
		}
		else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
			ERR_EXIT("Cannot find a specified extension library"
				".\nMake sure your layers path is set appropriately.\n",
				"vkCreateInstance Failure");
		}
		else if (err) {
			ERR_EXIT("vkCreateInstance failed.\n\nDo you have a compatible Vulkan "
				"installable client driver (ICD) installed?\nPlease look at "
				"the Getting Started guide for additional information.\n",
				"vkCreateInstance Failure");
		}

		/* Make initial call to query gpu_count, then second call for gpu info*/
		err = vkEnumeratePhysicalDevices(this->inst, &gpu_count, NULL);
		assert(!err && gpu_count > 0);

		if (gpu_count > 0) {
			VkPhysicalDevice *physical_devices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * gpu_count);
			err = vkEnumeratePhysicalDevices(this->inst, &gpu_count, physical_devices);
			assert(!err);
			/* For cube demo we just grab the first physical device */
			this->gpu = physical_devices[0];
			free(physical_devices);
		}
		else {
			ERR_EXIT("vkEnumeratePhysicalDevices reported zero accessible devices.\n\n"
				"Do you have a compatible Vulkan installable client driver (ICD) "
				"installed?\nPlease look at the Getting Started guide for "
				"additional information.\n",
				"vkEnumeratePhysicalDevices Failure");
		}

		/* Look for device extensions */
		uint32_t device_extension_count = 0;
		VkBool32 swapchainExtFound = 0;
		this->enabled_extension_count = 0;
		memset(this->extension_names, 0, sizeof(this->extension_names));

		err = vkEnumerateDeviceExtensionProperties(this->gpu, NULL,
			&device_extension_count, NULL);
		assert(!err);

		if (device_extension_count > 0) {
			VkExtensionProperties *device_extensions =
				(VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * device_extension_count);
			err = vkEnumerateDeviceExtensionProperties(
				this->gpu, NULL, &device_extension_count, device_extensions);
			assert(!err);

			for (uint32_t i = 0; i < device_extension_count; i++) {
				if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME,
					device_extensions[i].extensionName)) {
					swapchainExtFound = 1;
					this->extension_names[this->enabled_extension_count++] =
						VK_KHR_SWAPCHAIN_EXTENSION_NAME;
				}
				assert(this->enabled_extension_count < 64);
			}

			if (this->VK_KHR_incremental_present_enabled) {
				// Even though the user "enabled" the extension via the command
				// line, we must make sure that it's enumerated for use with the
				// device.  Therefore, disable it here, and re-enable it again if
				// enumerated.
				this->VK_KHR_incremental_present_enabled = false;
				for (uint32_t i = 0; i < device_extension_count; i++) {
					if (!strcmp(VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME,
						device_extensions[i].extensionName)) {
						this->extension_names[this->enabled_extension_count++] =
							VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME;
						this->VK_KHR_incremental_present_enabled = true;
						DbgMsg("VK_KHR_incremental_present extension enabled\n");
					}
					assert(this->enabled_extension_count < 64);
				}
				if (!this->VK_KHR_incremental_present_enabled) {
					DbgMsg("VK_KHR_incremental_present extension NOT AVAILABLE\n");
				}
			}

			if (this->VK_GOOGLE_display_timing_enabled) {
				// Even though the user "enabled" the extension via the command
				// line, we must make sure that it's enumerated for use with the
				// device.  Therefore, disable it here, and re-enable it again if
				// enumerated.
				this->VK_GOOGLE_display_timing_enabled = false;
				for (uint32_t i = 0; i < device_extension_count; i++) {
					if (!strcmp(VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME,
						device_extensions[i].extensionName)) {
						this->extension_names[this->enabled_extension_count++] =
							VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME;
						this->VK_GOOGLE_display_timing_enabled = true;
						DbgMsg("VK_GOOGLE_display_timing extension enabled\n");
					}
					assert(this->enabled_extension_count < 64);
				}
				if (!this->VK_GOOGLE_display_timing_enabled) {
					DbgMsg("VK_GOOGLE_display_timing extension NOT AVAILABLE\n");
				}
			}

			free(device_extensions);
		}

		if (!swapchainExtFound) {
			ERR_EXIT("vkEnumerateDeviceExtensionProperties failed to find "
				"the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
				" extension.\n\nDo you have a compatible "
				"Vulkan installable client driver (ICD) installed?\nPlease "
				"look at the Getting Started guide for additional "
				"information.\n",
				"vkCreateInstance Failure");
		}

		if (this->validate) {
			this->CreateDebugReportCallback =
				(PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
					this->inst, "vkCreateDebugReportCallbackEXT");
			this->DestroyDebugReportCallback =
				(PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
					this->inst, "vkDestroyDebugReportCallbackEXT");
			if (!this->CreateDebugReportCallback) {
				ERR_EXIT(
					"GetProcAddr: Unable to find vkCreateDebugReportCallbackEXT\n",
					"vkGetProcAddr Failure");
			}
			if (!this->DestroyDebugReportCallback) {
				ERR_EXIT(
					"GetProcAddr: Unable to find vkDestroyDebugReportCallbackEXT\n",
					"vkGetProcAddr Failure");
			}
			this->DebugReportMessage =
				(PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(
					this->inst, "vkDebugReportMessageEXT");
			if (!this->DebugReportMessage) {
				ERR_EXIT("GetProcAddr: Unable to find vkDebugReportMessageEXT\n",
					"vkGetProcAddr Failure");
			}

			VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
			PFN_vkDebugReportCallbackEXT callback;
			callback = this->use_break ? BreakCallback : dbgFunc;
			dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
			dbgCreateInfo.pNext = NULL;
			dbgCreateInfo.pfnCallback = callback;
			dbgCreateInfo.pUserData = this;
			dbgCreateInfo.flags =
				VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
			err = this->CreateDebugReportCallback(this->inst, &dbgCreateInfo, NULL,
				&this->msg_callback);
			switch (err) {
			case VK_SUCCESS:
				break;
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				ERR_EXIT("CreateDebugReportCallback: out of host memory\n",
					"CreateDebugReportCallback Failure");
				break;
			default:
				ERR_EXIT("CreateDebugReportCallback: unknown failure\n",
					"CreateDebugReportCallback Failure");
				break;
			}
		}
		vkGetPhysicalDeviceProperties(this->gpu, &this->gpu_props);

		/* Call with NULL data to get count */
		vkGetPhysicalDeviceQueueFamilyProperties(this->gpu,
			&this->queue_family_count, NULL);
		assert(this->queue_family_count >= 1);

		this->queue_props = (VkQueueFamilyProperties *)malloc(
			this->queue_family_count * sizeof(VkQueueFamilyProperties));
		vkGetPhysicalDeviceQueueFamilyProperties(
			this->gpu, &this->queue_family_count, this->queue_props);

		// Query fine-grained feature support for this device.
		//  If app has specific feature requirements it should check supported
		//  features based on this query
		VkPhysicalDeviceFeatures physDevFeatures;
		vkGetPhysicalDeviceFeatures(this->gpu, &physDevFeatures);

		//GET_INSTANCE_PROC_ADDR(this->inst, GetPhysicalDeviceSurfaceSupportKHR);
		//GET_INSTANCE_PROC_ADDR(this->inst, GetPhysicalDeviceSurfaceCapabilitiesKHR);
		//GET_INSTANCE_PROC_ADDR(this->inst, GetPhysicalDeviceSurfaceFormatsKHR);
		//GET_INSTANCE_PROC_ADDR(this->inst, GetPhysicalDeviceSurfacePresentModesKHR);
		//GET_INSTANCE_PROC_ADDR(this->inst, GetSwapchainImagesKHR);




	}
	GraphicsDevice_Vulkan::~GraphicsDevice_Vulkan()
	{
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

	HRESULT GraphicsDevice_Vulkan::CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *ppBuffer)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateTexture1D(const Texture1DDesc* pDesc, const SubresourceData *pInitialData, Texture1D **ppTexture1D)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateTexture2D(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, Texture2D **ppTexture2D)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateTexture3D(const Texture3DDesc* pDesc, const SubresourceData *pInitialData, Texture3D **ppTexture3D)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements,
		const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, VertexLayout *pInputLayout)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, VertexShader *pVertexShader)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, PixelShader *pPixelShader)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, GeometryShader *pGeometryShader)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, HullShader *pHullShader)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, DomainShader *pDomainShader)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ComputeShader *pComputeShader)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateGraphicsPSO(const GraphicsPSODesc* pDesc, GraphicsPSO* pso)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::CreateComputePSO(const ComputePSODesc* pDesc, ComputePSO* pso)
	{
		return E_FAIL;
	}


	void GraphicsDevice_Vulkan::PresentBegin()
	{
	}
	void GraphicsDevice_Vulkan::PresentEnd()
	{
	}

	void GraphicsDevice_Vulkan::ExecuteDeferredContexts()
	{
	}
	void GraphicsDevice_Vulkan::FinishCommandList(GRAPHICSTHREAD threadID)
	{
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
		vkCmdSetViewport(commandBuffers[threadID], 0, NumViewports, viewports);
	}
	void GraphicsDevice_Vulkan::BindRenderTargetsUAVs(UINT NumViews, Texture* const *ppRenderTargets, Texture2D* depthStencilTexture, GPUResource* const *ppUAVs, int slotUAV, int countUAV,
		GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_Vulkan::BindRenderTargets(UINT NumViews, Texture* const *ppRenderTargets, Texture2D* depthStencilTexture, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_Vulkan::ClearRenderTarget(Texture* pTexture, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_Vulkan::ClearDepthStencil(Texture2D* pTexture, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_Vulkan::BindResource(SHADERSTAGE stage, GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_Vulkan::BindResources(SHADERSTAGE stage, GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindUnorderedAccessResource(SHADERSTAGE stage, GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_Vulkan::BindUnorderedAccessResources(SHADERSTAGE stage, GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindUnorderedAccessResourceCS(GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_Vulkan::BindUnorderedAccessResourcesCS(GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::UnBindResources(int slot, int num, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::UnBindUnorderedAccessResources(int slot, int num, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindSampler(SHADERSTAGE stage, Sampler* sampler, int slot, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindConstantBuffer(SHADERSTAGE stage, GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindVertexBuffers(GPUBuffer* const *vertexBuffers, int slot, int count, const UINT* strides, const UINT* offsets, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindIndexBuffer(GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, UINT offset, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindPrimitiveTopology(PRIMITIVETOPOLOGY type, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindStencilRef(UINT value, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindBlendFactor(XMFLOAT4 value, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindGraphicsPSO(GraphicsPSO* pso, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::BindComputePSO(ComputePSO* pso, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::Draw(int vertexCount, UINT startVertexLocation, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::DrawIndexed(int indexCount, UINT startIndexLocation, UINT baseVertexLocation, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::DrawInstanced(int vertexCount, int instanceCount, UINT startVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::DrawIndexedInstanced(int indexCount, int instanceCount, UINT startIndexLocation, UINT baseVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::DrawInstancedIndirect(GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::DrawIndexedInstancedIndirect(GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::DispatchIndirect(GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::GenerateMips(Texture* texture, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_Vulkan::CopyTexture2D(Texture2D* pDst, Texture2D* pSrc, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::CopyTexture2D_Region(Texture2D* pDst, UINT dstMip, UINT dstX, UINT dstY, Texture2D* pSrc, UINT srcMip, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::MSAAResolve(Texture2D* pDst, Texture2D* pSrc, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::UpdateBuffer(GPUBuffer* buffer, const void* data, GRAPHICSTHREAD threadID, int dataSize)
	{
	}
	void* GraphicsDevice_Vulkan::AllocateFromRingBuffer(GPURingBuffer* buffer, size_t dataSize, UINT& offsetIntoBuffer, GRAPHICSTHREAD threadID)
	{
		offsetIntoBuffer = 0;
		return nullptr;
	}
	void GraphicsDevice_Vulkan::InvalidateBufferAccess(GPUBuffer* buffer, GRAPHICSTHREAD threadID)
	{
	}
	bool GraphicsDevice_Vulkan::DownloadBuffer(GPUBuffer* bufferToDownload, GPUBuffer* bufferDest, void* dataDest, GRAPHICSTHREAD threadID)
	{
		return false;
	}
	void GraphicsDevice_Vulkan::SetScissorRects(UINT numRects, const Rect* rects, GRAPHICSTHREAD threadID)
	{
		assert(rects != nullptr);
		assert(numRects <= 8);
		VkRect2D scissors[8];
		for (UINT i = 0; i < numRects; ++i)
		{
			scissors[i].extent.width = abs(rects[i].right - rects[i].left);
			scissors[i].extent.height = abs(rects[i].bottom - rects[i].top);
			scissors[i].offset.x = rects[i].left;
			scissors[i].offset.y = rects[i].bottom;
		}
		vkCmdSetScissor(commandBuffers[threadID], 0, numRects, scissors);
	}

	void GraphicsDevice_Vulkan::QueryBegin(GPUQuery *query, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::QueryEnd(GPUQuery *query, GRAPHICSTHREAD threadID)
	{
	}
	bool GraphicsDevice_Vulkan::QueryRead(GPUQuery *query, GRAPHICSTHREAD threadID)
	{
		return true;
	}

	void GraphicsDevice_Vulkan::UAVBarrier(GPUResource *const* uavs, UINT NumBarriers, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::TransitionBarrier(GPUResource *const* resources, UINT NumBarriers, RESOURCE_STATES stateBefore, RESOURCE_STATES stateAfter, GRAPHICSTHREAD threadID)
	{
	}


	HRESULT GraphicsDevice_Vulkan::CreateTextureFromFile(const std::string& fileName, Texture2D **ppTexture, bool mipMaps, GRAPHICSTHREAD threadID)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::SaveTexturePNG(const std::string& fileName, Texture2D *pTexture, GRAPHICSTHREAD threadID)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_Vulkan::SaveTextureDDS(const std::string& fileName, Texture *pTexture, GRAPHICSTHREAD threadID)
	{
		return E_FAIL;
	}

	void GraphicsDevice_Vulkan::EventBegin(const std::string& name, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::EventEnd(GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_Vulkan::SetMarker(const std::string& name, GRAPHICSTHREAD threadID)
	{
	}

}


#endif // WICKEDENGINE_BUILD_VULKAN
