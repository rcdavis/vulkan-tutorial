#include "Application.h"

#include "Utils/Log.h"

Application::~Application() {
	Shutdown();
}

void Application::Run() {
	Init();

	LOG_INFO("Application is running!");

	Shutdown();
}

void Application::Init() {
	if (volkInitialize() != VK_SUCCESS) {
		LOG_ERROR("Failed to initialize volk!");
	}
}

void Application::Shutdown() {

}
