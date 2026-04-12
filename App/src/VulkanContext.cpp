#include "VulkanContext.h"

#include "Utils/VkUtils.h"
#include "Utils/Log.h"
#include "Platform.h"

static bool VulkanContext_CreateInstance(VulkanContext& context, Platform& platform);

static bool VulkanContext_CreateDevice(VulkanContext& context);

static bool VulkanContext_CreateSwapchain(VulkanContext& context, Platform& platform);

static bool CheckValidationLayerSupport() {
	const auto availableLayers = VkUtils::GetInstanceLayerProperties();
	for (const char* layerName : VulkanContext::ValidationLayers) {
		bool layerFound = false;
		for (const auto& layerProps : availableLayers) {
			if (strcmp(layerName, layerProps.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
			return false;
	}

	return true;
}

bool VulkanContext_Init(VulkanContext& context, Platform& platform) {
	if (!VulkanContext_CreateInstance(context, platform)) {
		LOG_ERROR("Failed to create Vulkan instance!");
		return false;
	}

	if constexpr (VulkanContext::EnableValidationLayers) {
		constexpr auto debugCreateInfo = VkUtils::CreateDebugMessengerCreateInfo();
		if (vkCreateDebugUtilsMessengerEXT(context.instance, &debugCreateInfo, nullptr, &context.debugMessenger) != VK_SUCCESS) {
			LOG_ERROR("Failed to create debug utils messenger");
			return false;
		}
	}

	if (!VulkanContext_CreateDevice(context)) {
		LOG_ERROR("Failed to create device!");
		return false;
	}

	if (!Platform_CheckPresentationSupport(platform, context.instance, context.physicalDevice, context.graphicsQueueFamily)) {
		LOG_ERROR("Selected queue family does not support presentation!");
		return false;
	}

	if (!VulkanContext_CreateSwapchain(context, platform)) {
		LOG_ERROR("Failed to create swapchain!");
		return false;
	}

	return true;
}

void VulkanContext_Destroy(VulkanContext& context) {
	for (VkImageView imageView : context.swapchainImageViews) {
		vkDestroyImageView(context.device, imageView, nullptr);
	}
	context.swapchainImageViews.clear();
	context.swapchainImages.clear();

	if (context.swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(context.device, context.swapchain, nullptr);
		context.swapchain = VK_NULL_HANDLE;
	}

	if (context.surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
		context.surface = VK_NULL_HANDLE;
	}

	if (context.mAllocator != VK_NULL_HANDLE) {
		vmaDestroyAllocator(context.mAllocator);
		context.mAllocator = VK_NULL_HANDLE;
	}

	if (context.device != VK_NULL_HANDLE) {
		vkDestroyDevice(context.device, nullptr);
		context.device = VK_NULL_HANDLE;
	}

	if constexpr (VulkanContext::EnableValidationLayers) {
		if (context.debugMessenger != VK_NULL_HANDLE) {
			vkDestroyDebugUtilsMessengerEXT(context.instance, context.debugMessenger, nullptr);
			context.debugMessenger = VK_NULL_HANDLE;
		}
	}

	if (context.instance != VK_NULL_HANDLE) {
		vkDestroyInstance(context.instance, nullptr);
		context.instance = VK_NULL_HANDLE;
	}

	context.physicalDevice = VK_NULL_HANDLE;
	context.graphicsQueue = VK_NULL_HANDLE;
	context.graphicsQueueFamily = -1;
}

static bool VulkanContext_CreateInstance(VulkanContext& context, Platform& platform) {
	if (volkInitialize() != VK_SUCCESS) {
		LOG_ERROR("Failed to initialize volk!");
		return false;
	}

	const auto instanceExtensions = Platform_GetRequiredExtensions(platform);

	constexpr VkApplicationInfo appInfo {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "Vulkan Tutorial",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_3
	};

	VkInstanceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = (uint32_t)std::size(instanceExtensions),
		.ppEnabledExtensionNames = std::data(instanceExtensions)
	};

	constexpr auto debugCreateInfo = VkUtils::CreateDebugMessengerCreateInfo();
	if constexpr (VulkanContext::EnableValidationLayers) {
		if (!CheckValidationLayerSupport()) {
			LOG_ERROR("Validation layers unavailable");
			return false;
		}

		createInfo.pNext = &debugCreateInfo;
		createInfo.enabledLayerCount = (uint32_t)std::size(VulkanContext::ValidationLayers);
		createInfo.ppEnabledLayerNames = std::data(VulkanContext::ValidationLayers);
	}

	if (vkCreateInstance(&createInfo, nullptr, &context.instance) != VK_SUCCESS) {
		LOG_ERROR("Failed to create Vulkan instance!");
		return false;
	}

	volkLoadInstance(context.instance);

	return true;
}

static bool VulkanContext_CreateDevice(VulkanContext& context) {
	const auto devices = VkUtils::GetPhysicalDevices(context.instance);
	for (const auto& device : devices) {
		VkPhysicalDeviceProperties2 deviceProps {};
		deviceProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

		vkGetPhysicalDeviceProperties2(device, &deviceProps);

		if (deviceProps.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			LOG_INFO("Selected physical device: {} (discrete GPU)", deviceProps.properties.deviceName);
			context.physicalDevice = device;
			break;
		}

		if (deviceProps.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
			LOG_INFO("Selected physical device: {} (integrated GPU)", deviceProps.properties.deviceName);
			context.physicalDevice = device;
			break;
		}
	}

	if (context.physicalDevice == VK_NULL_HANDLE) {
		LOG_ERROR("Failed to find suitable physical device!");
		return false;
	}

	const auto queueFamilies = VkUtils::GetQueueFamilyProperties(context.physicalDevice);
	for (uint32_t i = 0; i < std::size(queueFamilies); i++) {
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			context.graphicsQueueFamily = i;
			break;
		}
	}

	if (context.graphicsQueueFamily == VulkanContext::InvalidQueueFamily) {
		LOG_ERROR("Failed to find suitable queue family with graphics support!");
		return false;
	}

	constexpr float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = context.graphicsQueueFamily;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceVulkan12Features deviceFeatures12 {};
	deviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	deviceFeatures12.descriptorIndexing = VK_TRUE;
	deviceFeatures12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	deviceFeatures12.descriptorBindingVariableDescriptorCount = VK_TRUE;
	deviceFeatures12.runtimeDescriptorArray = VK_TRUE;
	deviceFeatures12.bufferDeviceAddress = VK_TRUE;

	VkPhysicalDeviceVulkan13Features deviceFeatures13 {};
	deviceFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	deviceFeatures13.pNext = &deviceFeatures12;
	deviceFeatures13.synchronization2 = VK_TRUE;
	deviceFeatures13.dynamicRendering = VK_TRUE;

	VkPhysicalDeviceFeatures enabledFeatures {};
	enabledFeatures.samplerAnisotropy = VK_TRUE;

	constexpr std::array deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkDeviceCreateInfo createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = &deviceFeatures13;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.enabledExtensionCount = (uint32_t)std::size(deviceExtensions);
	createInfo.ppEnabledExtensionNames = std::data(deviceExtensions);
	createInfo.pEnabledFeatures = &enabledFeatures;

	if (vkCreateDevice(context.physicalDevice, &createInfo, nullptr, &context.device) != VK_SUCCESS) {
		LOG_ERROR("Failed to create Vulkan device!");
		return false;
	}

	vkGetDeviceQueue(context.device, context.graphicsQueueFamily, 0, &context.graphicsQueue);

	const VmaVulkanFunctions vmaVulkanFunctions {
		.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
		.vkGetDeviceProcAddr = vkGetDeviceProcAddr,
		.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
		.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
		.vkAllocateMemory = vkAllocateMemory,
		.vkFreeMemory = vkFreeMemory,
		.vkMapMemory = vkMapMemory,
		.vkUnmapMemory = vkUnmapMemory,
		.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
		.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
		.vkBindBufferMemory = vkBindBufferMemory,
		.vkBindImageMemory = vkBindImageMemory,
		.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
		.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
		.vkCreateBuffer = vkCreateBuffer,
		.vkDestroyBuffer = vkDestroyBuffer,
		.vkCreateImage = vkCreateImage,
		.vkDestroyImage = vkDestroyImage
	};

	VmaAllocatorCreateInfo allocatorCreateInfo {};
	allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
	allocatorCreateInfo.physicalDevice = context.physicalDevice;
	allocatorCreateInfo.device = context.device;
	allocatorCreateInfo.instance = context.instance;
	allocatorCreateInfo.pVulkanFunctions = &vmaVulkanFunctions;

	if (vmaCreateAllocator(&allocatorCreateInfo, &context.mAllocator) != VK_SUCCESS) {
		LOG_ERROR("Failed to create VMA allocator!");
		return false;
	}

	return true;
}

static bool VulkanContext_CreateSwapchain(VulkanContext& context, Platform& platform) {
	context.surface = Platform_CreateVulkanSurface(platform, context.instance);
	if (context.surface == VK_NULL_HANDLE) {
		LOG_ERROR("Failed to create Vulkan surface!");
		return false;
	}

	VkSurfaceCapabilitiesKHR surfaceCapabilities {};
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physicalDevice, context.surface, &surfaceCapabilities) != VK_SUCCESS) {
		LOG_ERROR("Failed to get surface capabilities!");
		return false;
	}

	context.swapchainExtent = surfaceCapabilities.currentExtent;
	if (context.swapchainExtent.width == 0xFFFFFFFF) {
		context.swapchainExtent = {
			.width = std::clamp(platform.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
			.height = std::clamp(platform.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
		};
	}

	constexpr VkFormat desiredFormat = VK_FORMAT_B8G8R8A8_SRGB;
	const VkSwapchainCreateInfoKHR createInfo {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = context.surface,
		.minImageCount = surfaceCapabilities.minImageCount,
		.imageFormat = desiredFormat,
		.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		.imageExtent = context.swapchainExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = surfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR
	};

	if (vkCreateSwapchainKHR(context.device, &createInfo, nullptr, &context.swapchain) != VK_SUCCESS) {
		LOG_ERROR("Failed to create swapchain!");
		return false;
	}

	uint32_t imageCount = 0;
	if (vkGetSwapchainImagesKHR(context.device, context.swapchain, &imageCount, nullptr) != VK_SUCCESS) {
		LOG_ERROR("Failed to get swapchain images!");
		return false;
	}
	context.swapchainImages.resize(imageCount);
	if (vkGetSwapchainImagesKHR(context.device, context.swapchain, &imageCount, context.swapchainImages.data()) != VK_SUCCESS) {
		LOG_ERROR("Failed to get swapchain image views!");
		return false;
	}
	context.swapchainImageViews.resize(imageCount);

	for (int i = 0; i < imageCount; i++) {
		const VkImageViewCreateInfo viewCreateInfo {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = context.swapchainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = desiredFormat,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		if (vkCreateImageView(context.device, &viewCreateInfo, nullptr, &context.swapchainImageViews[i]) != VK_SUCCESS) {
			LOG_ERROR("Failed to create swapchain image views!");
			return false;
		}
	}

	return true;
}
