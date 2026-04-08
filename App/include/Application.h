#pragma once

#include "volk.h"
#include "VulkanContext.h"

#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"

#include <vector>
#include <array>

class Application {
public:
	constexpr static int WIDTH = 1280;
	constexpr static int HEIGHT = 720;

public:
	Application() = default;
	~Application();

	void Run();

private:
	bool Init();
	void Shutdown();

	void MainLoop();
	void Render();

	std::vector<const char*> GetRequiredExtensions();

private:
	VulkanContext mVulkanContext {};

	SDL_Window* mWindow = nullptr;
	SDL_Renderer* mRenderer = nullptr;

	VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;

	bool mIsRunning = false;
	bool mVulkanLoaded = false;
};
