#pragma once

#include "volk.h"

#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"

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

private:
	SDL_Window* mWindow = nullptr;
	SDL_Renderer* mRenderer = nullptr;

	VkInstance mInstance = VK_NULL_HANDLE;

	bool mIsRunning = false;
};
