
#include "vulkan/vulkan_raii.hpp"

struct GLFWwindow;

class Application {
public:
	Application();
	~Application();

	void Run();

private:
	void InitWindow();
	void InitVulkan();
	void MainLoop();
	void Cleanup();

private:
	GLFWwindow* mWindow;
};
