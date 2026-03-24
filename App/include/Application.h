#pragma once

class Application {
public:
	Application() = default;
	~Application();

	void Run();

private:
	void Init();
	void Shutdown();
};
