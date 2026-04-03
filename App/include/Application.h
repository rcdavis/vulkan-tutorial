#pragma once

#include "volk.h"

#include "SDL3/SDL.h"

class Application {
public:
	Application() = default;
	~Application();

	void Run();

private:
	void Init();
	void Shutdown();

	void MainLoop();

private:
	SDL_Window* mWindow = nullptr;
	bool mIsRunning = false;
};
