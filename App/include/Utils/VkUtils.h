#pragma once

#include "volk.h"

#include <vector>

namespace VkUtils {
	std::vector<VkLayerProperties> GetInstanceLayerProperties();

	std::vector<VkPhysicalDevice> GetPhysicalDevices(VkInstance instance);

	VkPhysicalDevice GetSuitablePhysicalDevice(VkInstance instance);

	std::vector<VkQueueFamilyProperties> GetQueueFamilyProperties(VkPhysicalDevice device);

	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* userData);

	constexpr VkDebugUtilsMessengerCreateInfoEXT CreateDebugMessengerCreateInfo() {
		constexpr VkDebugUtilsMessageSeverityFlagsEXT severityFlags =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		constexpr VkDebugUtilsMessageTypeFlagsEXT messageTypes =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		constexpr VkDebugUtilsMessengerCreateInfoEXT createInfo {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.pNext = nullptr,
			.flags = 0,
			.messageSeverity = severityFlags,
			.messageType = messageTypes,
			.pfnUserCallback = DebugCallback,
			.pUserData = nullptr
		};

		return createInfo;
	}
}
