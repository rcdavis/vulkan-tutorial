#include "VulkanContext.h"

#include "Utils/VkUtils.h"
#include "Utils/Log.h"
#include "Platform.h"
#include "Mesh.h"

#include "ktx.h"
#include "ktxvulkan.h"
#include <array>
#include <vector>
#include <string>
#include <vulkan/vulkan_core.h>
#include <slang/slang.h>
#include <slang/slang-com-ptr.h>

static bool VulkanContext_CreateInstance(VulkanContext& context, Platform& platform);

static bool VulkanContext_CreateDevice(VulkanContext& context);

static bool VulkanContext_CreateSwapchain(VulkanContext& context, Platform& platform);

static bool VulkanContext_CreateMeshBuffers(VulkanContext& context, Platform& platform);

static bool VulkanContext_CreateShaderDataBuffers(VulkanContext& context);

static bool VulkanContext_CreateSyncObjects(VulkanContext& context);

static bool VulkanContext_CreateCommandBuffers(VulkanContext& context);

static bool VulkanContext_CreateTextures(VulkanContext& context);

static bool VulkanContext_CreateShadersAndGraphicsPipeline(VulkanContext& context);

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

	if (!VulkanContext_CreateShaderDataBuffers(context)) {
		LOG_ERROR("Failed to create shader data buffers!");
		return false;
	}

	if (!VulkanContext_CreateSyncObjects(context)) {
		LOG_ERROR("Failed to create synchronization objects!");
		return false;
	}

	if (!VulkanContext_CreateCommandBuffers(context)) {
		LOG_ERROR("Failed to create command buffers!");
		return false;
	}

	if (!VulkanContext_CreateTextures(context)) {
		LOG_ERROR("Failed to create textures!");
		return false;
	}

	if (!VulkanContext_CreateShadersAndGraphicsPipeline(context)) {
		LOG_ERROR("Failed to create shaders and graphics pipeline");
		return false;
	}

	return true;
}

void VulkanContext_Destroy(VulkanContext& context) {
	if (context.device != VK_NULL_HANDLE) {
		if (vkDeviceWaitIdle(context.device) != VK_SUCCESS) {
			LOG_ERROR("Failed to wait for device idle during cleanup!");
		}
	}

	context.textureDescriptors.fill({});

	for (TextureData& texture : context.textures) {
		TextureData_Destroy(texture, context.device, context.allocator);
	}

	for (uint32_t i = 0; i < VulkanContext::MaxFramesInFlight; ++i) {
		if (context.shaderDataBuffers[i].buffer != VK_NULL_HANDLE) {
			vmaDestroyBuffer(context.allocator, context.shaderDataBuffers[i].buffer, context.shaderDataBuffers[i].allocation);
			context.shaderDataBuffers[i].buffer = VK_NULL_HANDLE;
			context.shaderDataBuffers[i].allocation = VK_NULL_HANDLE;
			context.shaderDataBuffers[i].bufferDeviceAddress = {};
		}

		if (context.inFlightFences[i] != VK_NULL_HANDLE) {
			vkDestroyFence(context.device, context.inFlightFences[i], nullptr);
			context.inFlightFences[i] = VK_NULL_HANDLE;
		}

		if (context.imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(context.device, context.imageAvailableSemaphores[i], nullptr);
			context.imageAvailableSemaphores[i] = VK_NULL_HANDLE;
		}

		context.commandBuffers[i] = VK_NULL_HANDLE;
	}

	for (VkSemaphore semaphore : context.renderFinishedSemaphores) {
		vkDestroySemaphore(context.device, semaphore, nullptr);
	}
	context.renderFinishedSemaphores.clear();

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

	if (context.pipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(context.device, context.pipelineLayout, nullptr);
		context.pipelineLayout = VK_NULL_HANDLE;
	}

	if (context.textureDescriptorSetLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(context.device, context.textureDescriptorSetLayout, nullptr);
		context.textureDescriptorSetLayout = VK_NULL_HANDLE;
	}

	if (context.descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(context.device, context.descriptorPool, nullptr);
		context.descriptorPool = VK_NULL_HANDLE;
	}

	if (context.swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(context.device, context.swapchain, nullptr);
		context.swapchain = VK_NULL_HANDLE;
	}

	if (context.surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
		context.surface = VK_NULL_HANDLE;
	}

	if (context.commandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(context.device, context.commandPool, nullptr);
		context.commandPool = VK_NULL_HANDLE;
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
	for (size_t i = 0; i < std::size(queueFamilies); i++) {
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			context.graphicsQueueFamily = (uint32_t)i;
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

	constexpr VkPhysicalDeviceFeatures enabledFeatures {
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

	const VmaVulkanFunctions vmaVulkanFunctions {
		.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
		.vkGetDeviceProcAddr = vkGetDeviceProcAddr,
	};

	const VmaAllocatorCreateInfo allocatorCreateInfo {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
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

	for (uint32_t i = 0; i < imageCount; i++) {
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

	const VkImageCreateInfo depthImageCI {
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

	constexpr VmaAllocationCreateInfo depthAllocCI {
		.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO,
	};

	if (vmaCreateImage(context.allocator, &depthImageCI, &depthAllocCI, &context.depthImage, &context.depthImageAllocation, nullptr) != VK_SUCCESS) {
		LOG_ERROR("Failed to create depth texture!");
		return false;
	}

	const VkImageViewCreateInfo depthViewCI {
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

	const VkBufferCreateInfo vertexBufferCI {
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

static bool VulkanContext_CreateShaderDataBuffers(VulkanContext& context) {
	for (uint32_t i = 0; i < VulkanContext::MaxFramesInFlight; ++i) {
		constexpr VkBufferCreateInfo bufferCI {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = sizeof(ShaderData),
			.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		};

		constexpr VmaAllocationCreateFlags bufferAllocFlags =
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT;

		constexpr VmaAllocationCreateInfo bufferAllocCI {
			.flags = bufferAllocFlags,
			.usage = VMA_MEMORY_USAGE_AUTO,
		};

		VmaAllocationInfo bufferAllocInfo {};
		if (vmaCreateBuffer(context.allocator, &bufferCI, &bufferAllocCI,
			&context.shaderDataBuffers[i].buffer, &context.shaderDataBuffers[i].allocation, &bufferAllocInfo) != VK_SUCCESS
		) {
			LOG_ERROR("Failed to create shader data buffer {}!", i);
			return false;
		}

		const VkBufferDeviceAddressInfo bufferAddressInfo {
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = context.shaderDataBuffers[i].buffer
		};

		context.shaderDataBuffers[i].bufferDeviceAddress = vkGetBufferDeviceAddress(context.device, &bufferAddressInfo);
	}

	return true;
}

static bool VulkanContext_CreateSyncObjects(VulkanContext& context) {
	constexpr VkSemaphoreCreateInfo semaphoreCI {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	constexpr VkFenceCreateInfo fenceCI {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};

	for (uint32_t i = 0; i < VulkanContext::MaxFramesInFlight; ++i) {
		if (vkCreateFence(context.device, &fenceCI, nullptr, &context.inFlightFences[i]) != VK_SUCCESS) {
			LOG_ERROR("Failed to create in-flight fence {}!", i);
			return false;
		}

		if (vkCreateSemaphore(context.device, &semaphoreCI, nullptr, &context.imageAvailableSemaphores[i]) != VK_SUCCESS) {
			LOG_ERROR("Failed to create image available semaphore {}!", i);
			return false;
		}
	}

	context.renderFinishedSemaphores.resize(context.swapchainImages.size());
	for (size_t i = 0; i < context.renderFinishedSemaphores.size(); ++i) {
		if (vkCreateSemaphore(context.device, &semaphoreCI, nullptr, &context.renderFinishedSemaphores[i]) != VK_SUCCESS) {
			LOG_ERROR("Failed to create render finished semaphore {}!", i);
			return false;
		}
	}

	return true;
}

static bool VulkanContext_CreateCommandBuffers(VulkanContext& context) {
	const VkCommandPoolCreateInfo commandPoolCI {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = context.graphicsQueueFamily,
	};

	if (vkCreateCommandPool(context.device, &commandPoolCI, nullptr, &context.commandPool) != VK_SUCCESS) {
		LOG_ERROR("Failed to create command pool!");
		return false;
	}

	const VkCommandBufferAllocateInfo commandBufferAI {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = context.commandPool,
		.commandBufferCount = VulkanContext::MaxFramesInFlight,
	};

	if (vkAllocateCommandBuffers(context.device, &commandBufferAI, std::data(context.commandBuffers)) != VK_SUCCESS) {
		LOG_ERROR("Failed to allocate command buffers!");
		return false;
	}

	return true;
}

static bool VulkanContext_CreateTextures(VulkanContext& context) {
	constexpr std::array texturePaths = {
		"res/textures/suzanne0.ktx",
		"res/textures/suzanne1.ktx",
		"res/textures/suzanne2.ktx",
	};

	static_assert(std::size(texturePaths) <= VulkanContext::MaxTextures,
		"Number of textures exceeds maximum supported by context!");

	for (size_t i = 0; i < std::size(texturePaths); ++i) {
		ktxTexture* texture = nullptr;
		auto loadError = ktxTexture_CreateFromNamedFile(texturePaths[i], KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);
		if (loadError != KTX_SUCCESS) {
			LOG_ERROR("Failed to load texture {}: {}", texturePaths[i], ktxErrorString(loadError));
			return false;
		}

		const VkFormat imageFormat = ktxTexture_GetVkFormat(texture);

		const VkImageCreateInfo imageCI {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = imageFormat,
			.extent = VkExtent3D { .width = texture->baseWidth, .height = texture->baseHeight, .depth = 1 },
			.mipLevels = texture->numLevels,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		constexpr VmaAllocationCreateInfo imageAllocCI {
			.usage = VMA_MEMORY_USAGE_AUTO,
		};

		if (vmaCreateImage(context.allocator, &imageCI, &imageAllocCI, &context.textures[i].image, &context.textures[i].allocation, nullptr) != VK_SUCCESS) {
			LOG_ERROR("Failed to create texture image {}!", texturePaths[i]);
			ktxTexture_Destroy(texture);
			return false;
		}

		const VkImageViewCreateInfo imageViewCI {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = context.textures[i].image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = imageFormat,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = texture->numLevels,
				.layerCount = 1
			}
		};

		if (vkCreateImageView(context.device, &imageViewCI, nullptr, &context.textures[i].imageView) != VK_SUCCESS) {
			LOG_ERROR("Failed to create texture image view {}!", texturePaths[i]);
			ktxTexture_Destroy(texture);
			return false;
		}

		const VkBufferCreateInfo imageSrcBufferCI {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = texture->dataSize,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		};

		constexpr VmaAllocationCreateFlags imageSrcBufferAllocFlags =
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT;

		constexpr VmaAllocationCreateInfo imageSrcBufferAllocCI {
			.flags = imageSrcBufferAllocFlags,
			.usage = VMA_MEMORY_USAGE_AUTO,
		};

		VkBuffer imageSrcBuffer {};
		VmaAllocation imageSrcBufferAllocation {};
		VmaAllocationInfo imageSrcBufferAllocInfo {};
		if (vmaCreateBuffer(context.allocator, &imageSrcBufferCI, &imageSrcBufferAllocCI, &imageSrcBuffer, &imageSrcBufferAllocation, &imageSrcBufferAllocInfo) != VK_SUCCESS) {
			LOG_ERROR("Failed to create staging buffer for texture {}!", texturePaths[i]);
			ktxTexture_Destroy(texture);
			return false;
		}

		memcpy(imageSrcBufferAllocInfo.pMappedData, texture->pData, texture->dataSize);

		constexpr VkFenceCreateInfo fenceCI {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		};

		VkFence uploadFence {};
		if (vkCreateFence(context.device, &fenceCI, nullptr, &uploadFence) != VK_SUCCESS) {
			LOG_ERROR("Failed to create upload fence for texture {}!", texturePaths[i]);
			vmaDestroyBuffer(context.allocator, imageSrcBuffer, imageSrcBufferAllocation);
			ktxTexture_Destroy(texture);
			return false;
		}

		const VkCommandBufferAllocateInfo commandBufferAI {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = context.commandPool,
			.commandBufferCount = 1,
		};

		VkCommandBuffer commandBuffer {};
		if (vkAllocateCommandBuffers(context.device, &commandBufferAI, &commandBuffer) != VK_SUCCESS) {
			LOG_ERROR("Failed to allocate command buffer for texture {}!", texturePaths[i]);
			vkDestroyFence(context.device, uploadFence, nullptr);
			vmaDestroyBuffer(context.allocator, imageSrcBuffer, imageSrcBufferAllocation);
			ktxTexture_Destroy(texture);
			return false;
		}

		constexpr VkCommandBufferBeginInfo commandBufferBI {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};

		if (vkBeginCommandBuffer(commandBuffer, &commandBufferBI) != VK_SUCCESS) {
			LOG_ERROR("Failed to begin command buffer for texture {}!", texturePaths[i]);
			vkFreeCommandBuffers(context.device, context.commandPool, 1, &commandBuffer);
			vkDestroyFence(context.device, uploadFence, nullptr);
			vmaDestroyBuffer(context.allocator, imageSrcBuffer, imageSrcBufferAllocation);
			ktxTexture_Destroy(texture);
			return false;
		}

		const VkImageMemoryBarrier2 barrierTexImage {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_NONE,
			.srcAccessMask = VK_ACCESS_2_NONE,
			.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.image = context.textures[i].image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = texture->numLevels,
				.layerCount = 1
			}
		};

		VkDependencyInfo barrierTexInfo {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &barrierTexImage,
		};

		vkCmdPipelineBarrier2(commandBuffer, &barrierTexInfo);

		std::vector<VkBufferImageCopy> bufferCopyRegions(texture->numLevels);
		for (uint32_t level = 0; level < texture->numLevels; ++level) {
			ktx_size_t mipOffset = 0;
			ktx_error_code_e ret = ktxTexture_GetImageOffset(texture, level, 0, 0, &mipOffset);
			if (ret != KTX_SUCCESS) {
				LOG_ERROR("Failed to get image offset for texture {} mip level {}: {}", texturePaths[i], level, ktxErrorString(ret));
				vkEndCommandBuffer(commandBuffer);
				vkFreeCommandBuffers(context.device, context.commandPool, 1, &commandBuffer);
				vkDestroyFence(context.device, uploadFence, nullptr);
				vmaDestroyBuffer(context.allocator, imageSrcBuffer, imageSrcBufferAllocation);
				ktxTexture_Destroy(texture);
				return false;
			}

			bufferCopyRegions[level] = {
				.bufferOffset = mipOffset,
				.imageSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = level,
					.layerCount = 1
				},
				.imageExtent = {
					.width = std::max(1u, texture->baseWidth >> level),
					.height = std::max(1u, texture->baseHeight >> level),
					.depth = 1
				}
			};
		}

		vkCmdCopyBufferToImage(commandBuffer, imageSrcBuffer, context.textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)std::size(bufferCopyRegions), std::data(bufferCopyRegions));

		const VkImageMemoryBarrier2 barrierTexImageReadable {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
			.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
			.image = context.textures[i].image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = texture->numLevels,
				.layerCount = 1
			}
		};
		barrierTexInfo.pImageMemoryBarriers = &barrierTexImageReadable;
		vkCmdPipelineBarrier2(commandBuffer, &barrierTexInfo);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			LOG_ERROR("Failed to end command buffer for texture {}!", texturePaths[i]);
			vkFreeCommandBuffers(context.device, context.commandPool, 1, &commandBuffer);
			vkDestroyFence(context.device, uploadFence, nullptr);
			vmaDestroyBuffer(context.allocator, imageSrcBuffer, imageSrcBufferAllocation);
			ktxTexture_Destroy(texture);
			return false;
		}

		const VkCommandBufferSubmitInfo commandSubmitInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = commandBuffer
		};

		const VkSubmitInfo2 submitInfo {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &commandSubmitInfo,
		};

		if (vkQueueSubmit2(context.graphicsQueue, 1, &submitInfo, uploadFence) != VK_SUCCESS) {
			LOG_ERROR("Failed to submit command buffer for texture {}!", texturePaths[i]);
			vkFreeCommandBuffers(context.device, context.commandPool, 1, &commandBuffer);
			vkDestroyFence(context.device, uploadFence, nullptr);
			vmaDestroyBuffer(context.allocator, imageSrcBuffer, imageSrcBufferAllocation);
			ktxTexture_Destroy(texture);
			return false;
		}

		if (vkWaitForFences(context.device, 1, &uploadFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
			LOG_ERROR("Failed to wait for upload fence for texture {}!", texturePaths[i]);
			vkFreeCommandBuffers(context.device, context.commandPool, 1, &commandBuffer);
			vkDestroyFence(context.device, uploadFence, nullptr);
			vmaDestroyBuffer(context.allocator, imageSrcBuffer, imageSrcBufferAllocation);
			ktxTexture_Destroy(texture);
			return false;
		}

		vkDestroyFence(context.device, uploadFence, nullptr);
		vmaDestroyBuffer(context.allocator, imageSrcBuffer, imageSrcBufferAllocation);

		const VkSamplerCreateInfo samplerCI {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = 8.0f,
			.maxLod = (float)texture->numLevels,
		};

		if (vkCreateSampler(context.device, &samplerCI, nullptr, &context.textures[i].sampler) != VK_SUCCESS) {
			LOG_ERROR("Failed to create sampler for texture {}!", texturePaths[i]);
			ktxTexture_Destroy(texture);
			return false;
		}

		ktxTexture_Destroy(texture);

		context.textureDescriptors[i] = {
			.sampler = context.textures[i].sampler,
			.imageView = context.textures[i].imageView,
			.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
		};
	}

	constexpr VkDescriptorBindingFlags descriptorBindingFlags = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

	const VkDescriptorSetLayoutBindingFlagsCreateInfo descriptorSetLayoutBindingFlagsCI {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.bindingCount = 1,
		.pBindingFlags = &descriptorBindingFlags,
	};

	constexpr VkDescriptorSetLayoutBinding samplerArrayBinding {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = VulkanContext::MaxTextures,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
	};

	const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = &descriptorSetLayoutBindingFlagsCI,
		.bindingCount = 1,
		.pBindings = &samplerArrayBinding,
	};

	if (vkCreateDescriptorSetLayout(context.device, &descriptorSetLayoutCI, nullptr, &context.textureDescriptorSetLayout) != VK_SUCCESS) {
		LOG_ERROR("Failed to create texture descriptor set layout!");
		return false;
	}

	constexpr VkDescriptorPoolSize descriptorPoolSize {
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = VulkanContext::MaxTextures,
	};

	const VkDescriptorPoolCreateInfo descriptorPoolCI {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 1,
		.poolSizeCount = 1,
		.pPoolSizes = &descriptorPoolSize,
	};

	if (vkCreateDescriptorPool(context.device, &descriptorPoolCI, nullptr, &context.descriptorPool) != VK_SUCCESS) {
		LOG_ERROR("Failed to create texture descriptor pool!");
		return false;
	}

	constexpr uint32_t varDescCount = VulkanContext::MaxTextures;
	const VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountAI {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
		.descriptorSetCount = 1,
		.pDescriptorCounts = &varDescCount,
	};

	const VkDescriptorSetAllocateInfo descriptorSetAI {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = &variableDescriptorCountAI,
		.descriptorPool = context.descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &context.textureDescriptorSetLayout,
	};

	if (vkAllocateDescriptorSets(context.device, &descriptorSetAI, &context.textureDescriptorSet) != VK_SUCCESS) {
		LOG_ERROR("Failed to allocate texture descriptor set!");
		return false;
	}

	const VkWriteDescriptorSet writeDescriptorSet {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = context.textureDescriptorSet,
		.dstBinding = 0,
		.descriptorCount = VulkanContext::MaxTextures,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = std::data(context.textureDescriptors),
	};

	vkUpdateDescriptorSets(context.device, 1, &writeDescriptorSet, 0, nullptr);

	return true;
}

static bool VulkanContext_CreateShadersAndGraphicsPipeline(VulkanContext& context) {
	Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
	slang::createGlobalSession(slangGlobalSession.writeRef());

	const std::array<slang::TargetDesc, 1> slangTargets = {
		slang::TargetDesc {
			.format = SLANG_SPIRV,
			.profile = slangGlobalSession->findProfile("spirv_1_4_vk")
		}
	};

	std::array<slang::CompilerOptionEntry, 1> slangOptions = {
		slang::CompilerOptionEntry {
			slang::CompilerOptionName::EmitSpirvDirectly,
			{slang::CompilerOptionValueKind::Int, 1}
		}
	};

	const slang::SessionDesc slangSessionDesc {
		.targets = std::data(slangTargets),
		.targetCount = (uint32_t)std::size(slangTargets),
		.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
		.compilerOptionEntries = std::data(slangOptions),
		.compilerOptionEntryCount = (uint32_t)std::size(slangOptions),
	};

	Slang::ComPtr<slang::ISession> slangSession;
	SlangResult res = slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());
	if (SLANG_FAILED(res) || !slangSession) {
		LOG_ERROR("Failed to create Slang session (result={})", (int)res);
		return false;
	}

	Slang::ComPtr<ISlangBlob> diagnostics;
	Slang::ComPtr<slang::IModule> slangModule {
		slangSession->loadModuleFromSource("triangle", "res/shaders/shader.slang", nullptr, diagnostics.writeRef())
	};

	if (diagnostics) {
		std::string text((const char*)diagnostics->getBufferPointer(), diagnostics->getBufferSize());
		LOG_ERROR("Slang loadModuleFromSource failed: {}", text.c_str());
		return false;
	}

	Slang::ComPtr<ISlangBlob> spirv;
	res = slangModule->getTargetCode(0, spirv.writeRef());
	if (SLANG_FAILED(res) || !spirv) {
		if (diagnostics) {
			std::string text((const char*)diagnostics->getBufferPointer(), diagnostics->getBufferSize());
			LOG_ERROR("Slang getTargetCode failed: {}", text.c_str());
		} else {
			LOG_ERROR("Slang getTargetCode failed (result={})", (int)res);
		}
		return false;
	}

	const VkShaderModuleCreateInfo shaderModuleCI {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = spirv->getBufferSize(),
		.pCode = (const uint32_t*)spirv->getBufferPointer(),
	};

	VkShaderModule shaderModule {};
	if (vkCreateShaderModule(context.device, &shaderModuleCI, nullptr, &shaderModule) != VK_SUCCESS) {
		LOG_ERROR("Failed to create shader module!");
		return false;
	}

	constexpr VkPushConstantRange pushConstantRange {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.size = sizeof(VkDeviceAddress),
	};

	const VkPipelineLayoutCreateInfo pipelineLayoutCI {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &context.textureDescriptorSetLayout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstantRange,
	};

	if (vkCreatePipelineLayout(context.device, &pipelineLayoutCI, nullptr, &context.pipelineLayout) != VK_SUCCESS) {
		LOG_ERROR("Failed to create pipeline layout!");
		return false;
	}

	constexpr VkVertexInputBindingDescription vertexInputBinding {
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};

	constexpr std::array<VkVertexInputAttributeDescription, 3> vertexInputAttributes = {
		VkVertexInputAttributeDescription {
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(Vertex, pos),
		},
		VkVertexInputAttributeDescription {
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(Vertex, normal),
		},
		VkVertexInputAttributeDescription {
			.location = 2,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(Vertex, texCoord),
		},
	};

	const VkPipelineVertexInputStateCreateInfo vertexInputStateCI {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertexInputBinding,
		.vertexAttributeDescriptionCount = (uint32_t)std::size(vertexInputAttributes),
		.pVertexAttributeDescriptions = std::data(vertexInputAttributes),
	};

	constexpr VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	};

	const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
		VkPipelineShaderStageCreateInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = shaderModule,
			.pName = "main",
		},
		VkPipelineShaderStageCreateInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = shaderModule,
			.pName = "main",
		},
	};

	constexpr VkPipelineViewportStateCreateInfo viewportStateCI {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};

	constexpr std::array<VkDynamicState, 2> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	const VkPipelineDynamicStateCreateInfo dynamicStateCI {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = (uint32_t)std::size(dynamicStates),
		.pDynamicStates = std::data(dynamicStates),
	};

	constexpr VkPipelineDepthStencilStateCreateInfo depthStencilStateCI {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
	};

	/*VkPipelineRenderingCreateInfo renderingCI {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &context.swapchainImageFormat,
		.depthAttachmentFormat = context.depthImageViewFormat,
	};*/

	vkDestroyShaderModule(context.device, shaderModule, nullptr);

	return true;
}
