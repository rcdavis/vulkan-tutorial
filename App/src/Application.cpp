#include "Application.h"

#include "Utils/Log.h"
#include "Utils/VkUtils.h"

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
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		LOG_ERROR("Failed to initialize SDL: {}", SDL_GetError());
		return false;
	}

	if (!SDL_Vulkan_LoadLibrary(nullptr)) {
		LOG_ERROR("Failed to load Vulkan library: {}", SDL_GetError());
		return false;
	}
	mVulkanLoaded = true;

	if (!CreateVulkanInstance(mVulkanContext, GetRequiredExtensions())) {
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

	if (!CreateDevice(mVulkanContext)) {
		LOG_ERROR("Failed to create device!");
		return false;
	}

	if (!SDL_Vulkan_GetPresentationSupport(mVulkanContext.instance, mVulkanContext.physicalDevice, mVulkanContext.graphicsQueueFamily)) {
		LOG_ERROR("Selected queue family does not support presentation!: {}", SDL_GetError());
		return false;
	}

	if (!SDL_CreateWindowAndRenderer("Vulkan Tutorial", WIDTH, HEIGHT, SDL_WINDOW_VULKAN, &mWindow, &mRenderer)) {
		LOG_ERROR("Failed to create SDL window and/or renderer: {}", SDL_GetError());
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

	DestroyVulkanContext(mVulkanContext);

	if (mRenderer) {
		SDL_DestroyRenderer(mRenderer);
		mRenderer = nullptr;
	}

	if (mWindow) {
		SDL_DestroyWindow(mWindow);
		mWindow = nullptr;
	}

	if (mVulkanLoaded) {
		SDL_Vulkan_UnloadLibrary();
		mVulkanLoaded = false;
	}

	SDL_Quit();
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
	SDL_SetRenderDrawColor(mRenderer, 255, 0, 255, 255);
	SDL_RenderClear(mRenderer);

	SDL_RenderPresent(mRenderer);
}

std::vector<const char*> Application::GetRequiredExtensions() {
	uint32_t count = 0;
	auto extensions = SDL_Vulkan_GetInstanceExtensions(&count);

	std::vector<const char*> result(extensions, extensions + count);

	if constexpr (VulkanContext::EnableValidationLayers)
		result.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return result;
}
