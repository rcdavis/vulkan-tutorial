#include "Platform.h"

#include "Utils/Log.h"
#include "SDL3/SDL_vulkan.h"
#include "VulkanContext.h"

bool Platform_Init(Platform& platform) {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		LOG_ERROR("Failed to initialize SDL: {}", SDL_GetError());
		return false;
	}

	if (!SDL_Vulkan_LoadLibrary(nullptr)) {
		LOG_ERROR("Failed to load Vulkan library: {}", SDL_GetError());
		return false;
	}
	platform.vulkanLoaded = true;

	return true;
}

void Platform_Destroy(Platform& platform) {
	if (platform.window) {
		SDL_DestroyWindow(platform.window);
		platform.window = nullptr;
	}

	if (platform.vulkanLoaded) {
		SDL_Vulkan_UnloadLibrary();
		platform.vulkanLoaded = false;
	}

	platform.width = 0;
	platform.height = 0;
}

bool Platform_CreateWindow(Platform& platform, const char* title, uint32_t width, uint32_t height) {
	if (platform.window) {
		LOG_ERROR("Window already created!");
		return false;
	}

	platform.window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN);
	if (!platform.window) {
		LOG_ERROR("Failed to create SDL window");
		return false;
	}

	platform.width = width;
	platform.height = height;

	return true;
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
