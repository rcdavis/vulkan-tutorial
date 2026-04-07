#pragma once

#include "volk.h"

#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"

#include <vector>
#include <array>

class Application {
public:
	constexpr static int WIDTH = 800;
	constexpr static int HEIGHT = 600;

public:
	Application() = default;
	~Application();

	void Run();

private:
	bool Init();
	void Shutdown();

	void MainLoop();
	void Render();

	bool InitVulkanInstance();
	bool InitDevice();

	std::vector<const char*> GetRequiredExtensions();

	bool CheckValidationLayerSupport();

private:
#ifdef NDEBUG
	constexpr static bool EnableValidationLayers = false;
#else
	constexpr static bool EnableValidationLayers = true;
#endif

	constexpr static std::array ValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

private:
	SDL_Window* mWindow = nullptr;
	SDL_Renderer* mRenderer = nullptr;

	VkInstance mInstance = VK_NULL_HANDLE;
	VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
	VkDevice mDevice = VK_NULL_HANDLE;
	VkQueue mGraphicsQueue = VK_NULL_HANDLE;

	VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;

	bool mIsRunning = false;
};
