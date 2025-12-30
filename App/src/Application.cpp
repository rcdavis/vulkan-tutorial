#include "Application.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

static constexpr uint32_t WindowWidth = 800;
static constexpr uint32_t WindowHeight = 600;

Application::Application() :
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

}

void Application::MainLoop() {
	while (!glfwWindowShouldClose(mWindow)) {
		glfwPollEvents();

		glfwSwapBuffers(mWindow);
	}
}

void Application::Cleanup() {
	glfwTerminate();
	mWindow = nullptr;
}
