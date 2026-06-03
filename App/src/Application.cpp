#include "Application.h"

#include "SDL3/SDL_camera.h"
#include "Utils/Log.h"

#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/matrix_transform.hpp"

Application::~Application() {
	Shutdown();
}

void Application::Run() {
	if (!Init()) {
		return;
	}

	LOG_INFO("Application is running!");
	MainLoop();

	//Shutdown();
}

bool Application::Init() {
	if (!Platform_Init(mPlatform, "Vulkan Tutorial", WIDTH, HEIGHT)) {
		LOG_ERROR("Failed to initialize platform!");
		return false;
	}

	if (!VulkanContext_Init(mVulkanContext, mPlatform)) {
		LOG_ERROR("Failed to initialize Vulkan context!");
		return false;
	}

	mIsRunning = true;

	LOG_INFO("Application initialized successfully!");
	return true;
}

void Application::Shutdown() {
	LOG_INFO("Shutting down application...");

	VulkanContext_Destroy(mVulkanContext);
	Platform_Destroy(mPlatform);
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
	if (vkWaitForFences(mVulkanContext.device, 1, &mVulkanContext.inFlightFences[mVulkanContext.currentFrame], VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
		LOG_ERROR("Failed to wait for in-flight fence!");
		return;
	}

	if (vkResetFences(mVulkanContext.device, 1, &mVulkanContext.inFlightFences[mVulkanContext.currentFrame]) != VK_SUCCESS) {
		LOG_ERROR("Failed to reset in-flight fence!");
		return;
	}

	VkResult result = vkAcquireNextImageKHR(mVulkanContext.device, mVulkanContext.swapchain, UINT64_MAX, mVulkanContext.imageAvailableSemaphores[mVulkanContext.currentFrame], VK_NULL_HANDLE, &mVulkanContext.imageIndex);
	if (result != VK_SUCCESS) {
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			mUpdateSwapchain = true;
			return;
		}

		LOG_ERROR("Failed to acquire next image!");
		return;
	}

	mShaderData.proj = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 32.0f);
	mShaderData.view = glm::translate(glm::mat4(1.0f), mCamPos);
}
