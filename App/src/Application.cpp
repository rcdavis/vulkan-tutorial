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
		LOG_ERROR("Failed to load SDL Vulkan library");
		return false;
	}

	if (volkInitialize() != VK_SUCCESS) {
		LOG_ERROR("Failed to initialize volk!");
		return false;
	}

	if (!InitVulkanInstance()) {
		LOG_ERROR("Failed to initialize Vulkan instance!");
		return false;
	}

	if constexpr (EnableValidationLayers) {
		constexpr auto debugCreateInfo = VkUtils::CreateDebugMessengerCreateInfo();
		if (vkCreateDebugUtilsMessengerEXT(mInstance, &debugCreateInfo, nullptr, &mDebugMessenger) != VK_SUCCESS) {
			LOG_ERROR("Failed to create debug utils messenger");
			return false;
		}
	}

	if (!InitPhysicalDevice()) {
		LOG_ERROR("Failed to initialize physical device!");
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

	if constexpr (EnableValidationLayers) {
		if (mDebugMessenger != VK_NULL_HANDLE) {
			vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
			mDebugMessenger = VK_NULL_HANDLE;
		}
	}

	if (mInstance != VK_NULL_HANDLE) {
		vkDestroyInstance(mInstance, nullptr);
		mInstance = VK_NULL_HANDLE;
	}

	if (mRenderer) {
		SDL_DestroyRenderer(mRenderer);
		mRenderer = nullptr;
	}

	if (mWindow) {
		SDL_DestroyWindow(mWindow);
		mWindow = nullptr;
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

bool Application::InitVulkanInstance() {
	constexpr VkApplicationInfo appInfo {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "Vulkan Tutorial",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_3
	};

	auto instanceExtensions = GetRequiredExtensions();
	if (std::empty(instanceExtensions)) {
		LOG_ERROR("Failed to get SDL Vulkan Instance extensions: {}", SDL_GetError());
		return false;
	}

	VkInstanceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = (uint32_t)std::size(instanceExtensions),
		.ppEnabledExtensionNames = std::data(instanceExtensions)
	};

	constexpr auto debugCreateInfo = VkUtils::CreateDebugMessengerCreateInfo();
	if constexpr (EnableValidationLayers) {
		if (!CheckValidationLayerSupport()) {
			LOG_ERROR("Validation layers unavailable");
			return false;
		}

		createInfo.pNext = &debugCreateInfo;
		createInfo.enabledLayerCount = (uint32_t)std::size(ValidationLayers);
		createInfo.ppEnabledLayerNames = std::data(ValidationLayers);
	}

	if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS) {
		LOG_ERROR("Failed to create Vulkan instance!");
		return false;
	}

	volkLoadInstance(mInstance);
	return true;
}

bool Application::InitPhysicalDevice() {
	mPhysicalDevice = VkUtils::GetSuitablePhysicalDevice(mInstance);
	if (mPhysicalDevice == VK_NULL_HANDLE) {
		LOG_ERROR("Failed to find suitable physical device!");
		return false;
	}

	return true;
}

std::vector<const char*> Application::GetRequiredExtensions() {
	uint32_t count = 0;
	auto extensions = SDL_Vulkan_GetInstanceExtensions(&count);

	std::vector<const char*> result(extensions, extensions + count);

	if constexpr (EnableValidationLayers)
		result.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return result;
}

bool Application::CheckValidationLayerSupport() {
	const auto availableLayers = VkUtils::GetInstanceLayerProperties();
	for (const char* layerName : ValidationLayers) {
		bool layerFound = false;
		for (const auto& layerProps : availableLayers) {
			if (strcmp(layerName, layerProps.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
			return false;
	}

	return true;
}
