#include "VulkanContext.h"

#include "Utils/VkUtils.h"
#include "Utils/Log.h"
#include "Platform.h"
#include "Mesh.h"

static bool VulkanContext_CreateInstance(VulkanContext& context, Platform& platform);

static bool VulkanContext_CreateDevice(VulkanContext& context);

static bool VulkanContext_CreateSwapchain(VulkanContext& context, Platform& platform);

static bool VulkanContext_CreateMeshBuffers(VulkanContext& context, Platform& platform);


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

	if (!VulkanContext_CreateMeshBuffers(context, platform)) {
		LOG_ERROR("Failed to create mesh buffers!");
		return false;
	}

	return true;
}

void VulkanContext_Destroy(VulkanContext& context) {
	if (context.vertexBuffer != VK_NULL_HANDLE) {
		vmaDestroyBuffer(context.allocator, context.vertexBuffer, context.vertexBufferAllocation);
		context.vertexBuffer = VK_NULL_HANDLE;
		context.vertexBufferAllocation = VK_NULL_HANDLE;
	}

	if (context.depthImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(context.device, context.depthImageView, nullptr);
		context.depthImageView = VK_NULL_HANDLE;
	}

	if (context.depthImage != VK_NULL_HANDLE) {
		vmaDestroyImage(context.allocator, context.depthImage, context.depthImageAllocation);
		context.depthImage = VK_NULL_HANDLE;
		context.depthImageAllocation = VK_NULL_HANDLE;
	}

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

	if (context.allocator != VK_NULL_HANDLE) {
		vmaDestroyAllocator(context.allocator);
		context.allocator = VK_NULL_HANDLE;
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
		.pApplicationName = "Vulkan Tutorial",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_3,
	};

	VkInstanceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = (uint32_t)std::size(instanceExtensions),
		.ppEnabledExtensionNames = std::data(instanceExtensions),
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
		VkPhysicalDeviceProperties2 deviceProps {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
		};

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
	VkDeviceQueueCreateInfo queueCreateInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = context.graphicsQueueFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority,
	};

	VkPhysicalDeviceVulkan12Features deviceFeatures12 {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.descriptorIndexing = VK_TRUE,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
		.bufferDeviceAddress = VK_TRUE,
	};

	VkPhysicalDeviceVulkan13Features deviceFeatures13 {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &deviceFeatures12,
		.synchronization2 = VK_TRUE,
		.dynamicRendering = VK_TRUE
	};

	VkPhysicalDeviceFeatures enabledFeatures {
		.samplerAnisotropy = VK_TRUE
	};

	constexpr std::array deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	const VkDeviceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &deviceFeatures13,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledExtensionCount = (uint32_t)std::size(deviceExtensions),
		.ppEnabledExtensionNames = std::data(deviceExtensions),
		.pEnabledFeatures = &enabledFeatures
	};

	if (vkCreateDevice(context.physicalDevice, &createInfo, nullptr, &context.device) != VK_SUCCESS) {
		LOG_ERROR("Failed to create Vulkan device!");
		return false;
	}

	vkGetDeviceQueue(context.device, context.graphicsQueueFamily, 0, &context.graphicsQueue);

	volkLoadDevice(context.device);

	VmaVulkanFunctions vmaVulkanFunctions {
		.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
		.vkGetDeviceProcAddr = vkGetDeviceProcAddr,
	};

	const VmaAllocatorCreateInfo allocatorCreateInfo {
		.physicalDevice = context.physicalDevice,
		.device = context.device,
		.pVulkanFunctions = &vmaVulkanFunctions,
		.instance = context.instance,
		.vulkanApiVersion = VK_API_VERSION_1_3,
	};

	if (vmaCreateAllocator(&allocatorCreateInfo, &context.allocator) != VK_SUCCESS) {
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
		LOG_ERROR("Failed to get swapchain image count!");
		return false;
	}
	context.swapchainImages.resize(imageCount);
	if (vkGetSwapchainImagesKHR(context.device, context.swapchain, &imageCount, context.swapchainImages.data()) != VK_SUCCESS) {
		LOG_ERROR("Failed to get swapchain image!");
		return false;
	}
	context.swapchainImageViews.resize(imageCount);

	for (int i = 0; i < imageCount; i++) {
		const VkImageViewCreateInfo viewCreateInfo {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = context.swapchainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = desiredFormat,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1
			}
		};

		if (vkCreateImageView(context.device, &viewCreateInfo, nullptr, &context.swapchainImageViews[i]) != VK_SUCCESS) {
			LOG_ERROR("Failed to create swapchain image views!");
			return false;
		}
	}

	constexpr std::array<VkFormat, 2> depthFormatList = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT
	};
	VkFormat depthFormat = VK_FORMAT_UNDEFINED;
	for (VkFormat format : depthFormatList) {
		VkFormatProperties2 props {
			 .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,
		};
		vkGetPhysicalDeviceFormatProperties2(context.physicalDevice, format, &props);
		if (props.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			depthFormat = format;
			break;
		}
	}

	if (depthFormat == VK_FORMAT_UNDEFINED) {
		LOG_ERROR("Failed to find a depth format!");
		return false;
	}

	VkImageCreateInfo depthImageCI {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = depthFormat,
		.extent = VkExtent3D { .width = context.swapchainExtent.width, .height = context.swapchainExtent.height, .depth = 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	VmaAllocationCreateInfo depthAllocCI {
		.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO,
	};

	if (vmaCreateImage(context.allocator, &depthImageCI, &depthAllocCI, &context.depthImage, &context.depthImageAllocation, nullptr) != VK_SUCCESS) {
		LOG_ERROR("Failed to create depth texture!");
		return false;
	}

	VkImageViewCreateInfo depthViewCI {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = context.depthImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = depthFormat,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.levelCount = 1,
			.layerCount = 1
		}
	};

	if (vkCreateImageView(context.device, &depthViewCI, nullptr, &context.depthImageView) != VK_SUCCESS) {
		LOG_ERROR("Failed to create depth image view!");
		return false;
	}

	return true;
}

static bool VulkanContext_CreateMeshBuffers(VulkanContext& context, Platform& platform) {
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
	if (!Mesh_LoadFromOBJ("res/meshes/suzanne.obj", vertices, indices)) {
		LOG_ERROR("Failed to load mesh!");
		return false;
	}

	const VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
	const VkDeviceSize indexBufferSize = sizeof(uint16_t) * indices.size();

	VkBufferCreateInfo vertexBufferCI {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = vertexBufferSize + indexBufferSize,
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	};

	constexpr VmaAllocationCreateFlags vertexBufferAllocFlags =
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
		VMA_ALLOCATION_CREATE_MAPPED_BIT;

	constexpr VmaAllocationCreateInfo vertexBufferAllocCI {
		.flags = vertexBufferAllocFlags,
		.usage = VMA_MEMORY_USAGE_AUTO,
	};

	VmaAllocationInfo vertexBufferAllocInfo {};
	if (vmaCreateBuffer(context.allocator, &vertexBufferCI, &vertexBufferAllocCI,
		&context.vertexBuffer, &context.vertexBufferAllocation, &vertexBufferAllocInfo) != VK_SUCCESS
	) {
		LOG_ERROR("Failed to create vertex buffer!");
		return false;
	}

	memcpy(vertexBufferAllocInfo.pMappedData, std::data(vertices), vertexBufferSize);
	memcpy((uint8_t*)vertexBufferAllocInfo.pMappedData + vertexBufferSize, std::data(indices), indexBufferSize);

	return true;
}
