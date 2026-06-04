#include "Application.h"

#include "Utils/Log.h"

#include "glm/gtc/quaternion.hpp"
#include <cstdint>
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
				.layerCount = 1,
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
				.layerCount = 1,
			}
		}
	};

	const VkDependencyInfo dependencyInfo {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = (uint32_t)std::size(outputBarriers),
		.pImageMemoryBarriers = std::data(outputBarriers),
	};

	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

	const VkRenderingAttachmentInfo colorAttachmentInfo {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = mVulkanContext.swapchainImageViews[mVulkanContext.imageIndex],
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue {
			.color = { { 1.0f, 0.0f, 1.0f, 1.0f } }
		}
	};

	const VkRenderingAttachmentInfo depthAttachmentInfo {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = mVulkanContext.depthImageView,
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.clearValue {
			.depthStencil = { 1.0f, 0 }
		}
	};

	// Maybe swap extent with platform window size?
	const VkRenderingInfo renderingInfo {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea {
			.extent = mVulkanContext.swapchainExtent
		},
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentInfo,
		.pDepthAttachment = &depthAttachmentInfo,
	};

	vkCmdBeginRendering(commandBuffer, &renderingInfo);

	const VkViewport viewport {
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)mVulkanContext.swapchainExtent.width,
		.height = (float)mVulkanContext.swapchainExtent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	const VkRect2D scissor {
		.offset = { 0, 0 },
		.extent = mVulkanContext.swapchainExtent,
	};
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mVulkanContext.pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mVulkanContext.pipelineLayout, 0, 1, &mVulkanContext.textureDescriptorSet, 0, nullptr);

	constexpr VkDeviceSize vOffset = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mVulkanContext.vertexBuffer, &vOffset);
	vkCmdBindIndexBuffer(commandBuffer, mVulkanContext.vertexBuffer, mVulkanContext.vertexBufferSize, VK_INDEX_TYPE_UINT16);

	vkCmdPushConstants(commandBuffer, mVulkanContext.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &mVulkanContext.shaderDataBuffers[mVulkanContext.currentFrame].bufferDeviceAddress);

	vkCmdDrawIndexed(commandBuffer, mVulkanContext.indexCount, 3, 0, 0, 0);
	vkCmdEndRendering(commandBuffer);

	const VkImageMemoryBarrier2 presentBarrier {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_2_NONE,
		.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.image = mVulkanContext.swapchainImages[mVulkanContext.imageIndex],
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		}
	};

	const VkDependencyInfo barrierPresetDependencyInfo {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &presentBarrier,
	};

	vkCmdPipelineBarrier2(commandBuffer, &barrierPresetDependencyInfo);

	vkEndCommandBuffer(commandBuffer);
}
