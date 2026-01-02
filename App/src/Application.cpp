#include "Application.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "Utils/Log.h"

static constexpr uint32_t WindowWidth = 800;
static constexpr uint32_t WindowHeight = 600;

Application::Application() :
	mInstance(VK_NULL_HANDLE),
	mWindow(nullptr)
{}

Application::~Application() {
	Cleanup();
}

void Application::Run() {
	InitWindow();
	InitVulkan();
	MainLoop();
}

void Application::InitWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	mWindow = glfwCreateWindow(WindowWidth, WindowHeight, "Vulkan", nullptr, nullptr);
}

void Application::InitVulkan() {
	CreateInstance();
}

void Application::MainLoop() {
	while (!glfwWindowShouldClose(mWindow)) {
		glfwPollEvents();

		glfwSwapBuffers(mWindow);
	}
}

void Application::Cleanup() {
	if (mInstance != VK_NULL_HANDLE) {
		vkDestroyInstance(mInstance, nullptr);
		mInstance = VK_NULL_HANDLE;
	}

	glfwTerminate();
	mWindow = nullptr;
}

void Application::CreateInstance() {
	constexpr VkApplicationInfo appInfo {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_4
	};

	uint32_t glfwExtensionCount = 0;
	const char** const glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	const VkInstanceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = glfwExtensionCount,
		.ppEnabledExtensionNames = glfwExtensions
	};

	if (const VkResult result = vkCreateInstance(&createInfo, nullptr, &mInstance); result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan Instance");
	}
}
