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

	if (!SDL_CreateWindowAndRenderer("Vulkan Tutorial", WIDTH, HEIGHT, SDL_WINDOW_VULKAN, &mWindow, &mRenderer)) {
		LOG_ERROR("Failed to create SDL window and/or renderer: {}", SDL_GetError());
	}

	mIsRunning = true;

	LOG_INFO("Application initialized successfully!");
}

void Application::Shutdown() {
	LOG_INFO("Shutting down application...");

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
