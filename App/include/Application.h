#pragma once

#include "volk.h"
#include "Platform.h"
#include "VulkanContext.h"

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

private:
	Platform mPlatform {};
	VulkanContext mVulkanContext {};

	VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;

	bool mIsRunning = false;
};
