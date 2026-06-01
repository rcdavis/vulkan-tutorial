#include "Platform.h"

#include "Utils/Log.h"
#include "SDL3/SDL_vulkan.h"
#include "VulkanContext.h"

bool Platform_Init(Platform& platform, const char* title, uint32_t width, uint32_t height) {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		LOG_ERROR("Failed to initialize SDL: {}", SDL_GetError());
		return false;
	}

	platform.window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN);
	if (!platform.window) {
		LOG_ERROR("Failed to create SDL window");
		return false;
	}

#ifdef PLATFORM_LINUX
	LOG_INFO("Running on Linux platform");
#endif

	platform.width = width;
	platform.height = height;

	return true;
}

void Platform_Destroy(Platform& platform) {
	if (platform.window) {
		SDL_DestroyWindow(platform.window);
		platform.window = nullptr;
	}

	platform.width = 0;
	platform.height = 0;
}

VkSurfaceKHR Platform_CreateVulkanSurface(Platform& platform, VkInstance instance) {
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	if (!SDL_Vulkan_CreateSurface(platform.window, instance, nullptr, &surface)) {
		LOG_ERROR("Failed to create Vulkan surface: {}", SDL_GetError());
		return VK_NULL_HANDLE;
	}

	return surface;
}

std::vector<const char*> Platform_GetRequiredExtensions(Platform& platform) {
	uint32_t count = 0;
	auto extensions = SDL_Vulkan_GetInstanceExtensions(&count);

	std::vector<const char*> result(extensions, extensions + count);

	if constexpr (VulkanContext::EnableValidationLayers) {
		result.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return result;
}

bool Platform_CheckPresentationSupport(Platform& platform,
	VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex
) {
	if (!SDL_Vulkan_GetPresentationSupport(instance, physicalDevice, queueFamilyIndex)) {
		LOG_ERROR("Selected queue family \'{}\' does not support presentation!: {}",
			queueFamilyIndex, SDL_GetError());
		return false;
	}

	return true;
}
