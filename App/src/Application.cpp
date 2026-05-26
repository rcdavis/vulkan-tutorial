#include "Application.h"

#include "Utils/Log.h"

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

	if (!VulkanContext_Init(mVulkanContext, mPlatform)) {
		LOG_ERROR("Failed to initialize Vulkan context!");
		return false;
	}

	mIsRunning = true;

	LOG_INFO("Application initialized successfully!");
	return true;
}

void Application::Shutdown() {
	LOG_INFO("Shutting down application...");

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
