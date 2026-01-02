
#include "vulkan/vulkan.h"

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

	// Vulkan creation methods
	void CreateInstance();

private:
	VkInstance mInstance;
	GLFWwindow* mWindow;
};
