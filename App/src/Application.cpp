#include "Application.h"

#include "Utils/Log.h"

Application::~Application() {
	Shutdown();
}

void Application::Run() {
	Init();

	LOG_INFO("Application is running!");
	MainLoop();

	//Shutdown();
}

void Application::Init() {
	if (volkInitialize() != VK_SUCCESS) {
		LOG_ERROR("Failed to initialize volk!");
	}

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		LOG_ERROR("Failed to initialize SDL: {}", SDL_GetError());
	}

	mWindow = SDL_CreateWindow("Vulkan Tutorial", 800, 600, SDL_WINDOW_VULKAN);
	if (!mWindow) {
		LOG_ERROR("Failed to create SDL window: {}", SDL_GetError());
	}

	mIsRunning = true;
}

void Application::Shutdown() {
	LOG_INFO("Shutting down application...");

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
	}
}
