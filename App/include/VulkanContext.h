#pragma once

#include "volk.h"
#include "vk_mem_alloc.h"

#include <array>
#include <vector>

struct Platform;

struct VulkanContext {
#ifdef NDEBUG
	constexpr static bool EnableValidationLayers = false;
#else
	constexpr static bool EnableValidationLayers = true;
#endif

	constexpr static std::array ValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	constexpr static uint32_t InvalidQueueFamily = -1;

	VmaAllocator allocator = VK_NULL_HANDLE;

	VkInstance instance = VK_NULL_HANDLE;

	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

	VkSurfaceKHR surface = VK_NULL_HANDLE;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;

	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkExtent2D swapchainExtent {};

	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

	VkImage depthImage = VK_NULL_HANDLE;
	VkImageView depthImageView = VK_NULL_HANDLE;
	VmaAllocation depthImageAllocation = VK_NULL_HANDLE;

	VkQueue graphicsQueue = VK_NULL_HANDLE;
	uint32_t graphicsQueueFamily = InvalidQueueFamily;
};

bool VulkanContext_Init(VulkanContext& context, Platform& platform);

void VulkanContext_Destroy(VulkanContext& context);
