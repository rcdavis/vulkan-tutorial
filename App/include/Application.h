#pragma once

#include "volk.h"

#include "SDL3/SDL.h"

class Application {
public:
	constexpr static int WIDTH = 800;
	constexpr static int HEIGHT = 600;

public:
	Application() = default;
	~Application();

	void Run();

private:
	void Init();
	void Shutdown();

	void MainLoop();
	void Render();

private:
	SDL_Window* mWindow = nullptr;
	SDL_Renderer* mRenderer = nullptr;
	bool mIsRunning = false;
};
