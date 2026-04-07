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
