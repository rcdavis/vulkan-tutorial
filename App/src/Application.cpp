#include "Application.h"

#include "SDL3/SDL_camera.h"
#include "Utils/Log.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include <cstdint>
#include <utility>
#include <vulkan/vulkan_core.h>

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
	for (size_t i = 0; i < std::size(mObjRotations); i++) {
		auto instancePos = glm::vec3((float)(i - 1) * 3.0f, 0.0f, 0.0f);
		mShaderData.model[i] = glm::translate(glm::mat4(1.0f), instancePos) * glm::mat4_cast(glm::quat(mObjRotations[i]));
	}
	memcpy(mVulkanContext.shaderDataBuffers[mVulkanContext.currentFrame].allocationInfo.pMappedData, &mShaderData, sizeof(ShaderData));

	VkCommandBuffer commandBuffer = mVulkanContext.commandBuffers[mVulkanContext.currentFrame];
	vkResetCommandBuffer(commandBuffer, 0);

	constexpr VkCommandBufferBeginInfo cbBeginInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	if (vkBeginCommandBuffer(commandBuffer, &cbBeginInfo) != VK_SUCCESS) {
		LOG_ERROR("Failed to begin recording command buffer!");
		return;
	}

	const std::array<VkImageMemoryBarrier2, 2> outputBarriers {
		VkImageMemoryBarrier2 {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_2_NONE,
			.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.image = mVulkanContext.swapchainImages[mVulkanContext.imageIndex],
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1
			}
		},
		VkImageMemoryBarrier2 {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
			.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.image = mVulkanContext.depthImage,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
				.levelCount = 1,
				.layerCount = 1
			}
		}
	};

	const VkDependencyInfo dependencyInfo {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = (uint32_t)std::size(outputBarriers),
		.pImageMemoryBarriers = std::data(outputBarriers),
	};

	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}
