
#include "volk.h"
#include "vulkan/vulkan.h"

#include <vector>

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

	bool CheckValidationLayerSupport();
	std::vector<const char*> GetRequiredExtensions();

	void SetupDebugMessenger();

private:
	VkInstance mInstance;

	VkDebugUtilsMessengerEXT mDebugMessenger;
	GLFWwindow* mWindow;
};
