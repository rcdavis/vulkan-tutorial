#pragma once

#include "volk.h"
#include "vk_mem_alloc.h"

#include "ShaderData.h"
#include "TextureData.h"

#include <array>
#include <vector>
#include <vulkan/vulkan_core.h>

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

	constexpr static uint32_t MaxFramesInFlight = 2;
	constexpr static uint32_t MaxTextures = 3;

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

	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VmaAllocation vertexBufferAllocation = VK_NULL_HANDLE;

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	VkDescriptorSetLayout textureDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet textureDescriptorSet = VK_NULL_HANDLE;

	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;

	VkCommandPool commandPool = VK_NULL_HANDLE;
	std::array<VkCommandBuffer, MaxFramesInFlight> commandBuffers{};

	std::array<TextureData, MaxTextures> textures{};
	std::array<VkDescriptorImageInfo, MaxTextures> textureDescriptors{};

	std::array<ShaderDataBuffer, MaxFramesInFlight> shaderDataBuffers;

	std::vector<VkSemaphore> renderFinishedSemaphores{};
	std::array<VkSemaphore, MaxFramesInFlight> imageAvailableSemaphores{};
	std::array<VkFence, MaxFramesInFlight> inFlightFences{};

	VkQueue graphicsQueue = VK_NULL_HANDLE;
	uint32_t graphicsQueueFamily = InvalidQueueFamily;

	VkFormat depthImageViewFormat = VK_FORMAT_UNDEFINED;
};

bool VulkanContext_Init(VulkanContext& context, Platform& platform);

void VulkanContext_Destroy(VulkanContext& context);
