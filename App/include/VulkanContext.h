#pragma once

#include "volk.h"

#include <vector>
#include <array>

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

	VkInstance instance = VK_NULL_HANDLE;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;

	VkQueue graphicsQueue = VK_NULL_HANDLE;
	uint32_t graphicsQueueFamily = InvalidQueueFamily;
};

bool CreateVulkanInstance(VulkanContext& context, const std::vector<const char*>& instanceExtensions);

bool CreateDevice(VulkanContext& context);

void DestroyVulkanContext(VulkanContext& context);
