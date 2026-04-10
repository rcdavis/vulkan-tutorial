#include "Application.h"

#include "Utils/Log.h"
#include "Utils/VkUtils.h"

#include "SDL3/SDL_vulkan.h"

Application::~Application() {
	Shutdown();
}

void Application::Run() {
	if (!Init()) {
		return;
	}

	LOG_INFO("Application is running!");
	MainLoop();

	//Shutdown();
}

bool Application::Init() {
	if (!Platform_Init(mPlatform, "Vulkan Tutorial", WIDTH, HEIGHT)) {
		LOG_ERROR("Failed to initialize platform!");
		return false;
	}

	if (!VulkanContext_CreateInstance(mVulkanContext, mPlatform)) {
		LOG_ERROR("Failed to create Vulkan instance!");
		return false;
	}

	if constexpr (VulkanContext::EnableValidationLayers) {
		constexpr auto debugCreateInfo = VkUtils::CreateDebugMessengerCreateInfo();
		if (vkCreateDebugUtilsMessengerEXT(mVulkanContext.instance, &debugCreateInfo, nullptr, &mDebugMessenger) != VK_SUCCESS) {
			LOG_ERROR("Failed to create debug utils messenger");
			return false;
		}
	}

	if (!VulkanContext_CreateDevice(mVulkanContext)) {
		LOG_ERROR("Failed to create device!");
		return false;
	}

	if (!SDL_Vulkan_GetPresentationSupport(mVulkanContext.instance, mVulkanContext.physicalDevice, mVulkanContext.graphicsQueueFamily)) {
		LOG_ERROR("Selected queue family does not support presentation!: {}", SDL_GetError());
		return false;
	}

	mVulkanContext.surface = Platform_CreateVulkanSurface(mPlatform, mVulkanContext.instance);
	if (mVulkanContext.surface == VK_NULL_HANDLE) {
		LOG_ERROR("Failed to create Vulkan surface!");
		return false;
	}

	mIsRunning = true;

	LOG_INFO("Application initialized successfully!");
	return true;
}

void Application::Shutdown() {
	LOG_INFO("Shutting down application...");

	if constexpr (VulkanContext::EnableValidationLayers) {
		if (mDebugMessenger != VK_NULL_HANDLE) {
			vkDestroyDebugUtilsMessengerEXT(mVulkanContext.instance, mDebugMessenger, nullptr);
			mDebugMessenger = VK_NULL_HANDLE;
		}
	}

	VulkanContext_Destroy(mVulkanContext);
	Platform_Destroy(mPlatform);
}

void Application::MainLoop() {
	SDL_Event event;
	SDL_zero(event);

	while (mIsRunning) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				mIsRunning = false;
			}
		}

		Render();
	}
}

void Application::Render() {

}
