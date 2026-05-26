#include "Utils/VkUtils.h"

#include "Utils/Log.h"

namespace VkUtils {
	std::vector<VkLayerProperties> GetInstanceLayerProperties() {
		uint32_t count = 0;
		vkEnumerateInstanceLayerProperties(&count, nullptr);

		std::vector<VkLayerProperties> layers(count);
		vkEnumerateInstanceLayerProperties(&count, std::data(layers));

		return layers;
	}

	std::vector<VkPhysicalDevice> GetPhysicalDevices(VkInstance instance) {
		uint32_t count = 0;
		vkEnumeratePhysicalDevices(instance, &count, nullptr);

		if (count == 0)
			return {};

		std::vector<VkPhysicalDevice> devices(count);
		vkEnumeratePhysicalDevices(instance, &count, std::data(devices));

		return devices;
	}

	std::vector<VkQueueFamilyProperties> GetQueueFamilyProperties(VkPhysicalDevice device) {
		uint32_t count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

		std::vector<VkQueueFamilyProperties> properties(count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, std::data(properties));

		return properties;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* userData
	) {
		switch (severity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			LOG_INFO("Vulkan validation: {}", callbackData->pMessage);
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			LOG_WARN("Vulkan validation: {}", callbackData->pMessage);
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			LOG_ERROR("Vulkan validation: {}", callbackData->pMessage);
			break;

		default:
			LOG_TRACE("Vulkan validation: {}", callbackData->pMessage);
			break;
		}

		return VK_FALSE;
	}
}
