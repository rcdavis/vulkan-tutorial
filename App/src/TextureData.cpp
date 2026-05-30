#include "TextureData.h"

void TextureData_Destroy(TextureData& textureData, VkDevice device, VmaAllocator allocator) {
	if (textureData.sampler != VK_NULL_HANDLE) {
		vkDestroySampler(device, textureData.sampler, nullptr);
		textureData.sampler = VK_NULL_HANDLE;
	}

	if (textureData.imageView != VK_NULL_HANDLE) {
		vkDestroyImageView(device, textureData.imageView, nullptr);
		textureData.imageView = VK_NULL_HANDLE;
	}

	if (textureData.image != VK_NULL_HANDLE) {
		vmaDestroyImage(allocator, textureData.image, textureData.allocation);
		textureData.image = VK_NULL_HANDLE;
		textureData.allocation = VK_NULL_HANDLE;
	}
}
