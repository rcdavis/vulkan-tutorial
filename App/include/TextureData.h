#pragma once

#include "volk.h"
#include "vk_mem_alloc.h"

struct TextureData {
	VmaAllocation allocation = VK_NULL_HANDLE;
	VkImage image = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;
};

void TextureData_Destroy(TextureData& textureData, VkDevice device, VmaAllocator allocator);
