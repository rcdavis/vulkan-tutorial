#pragma once

#include "glm/glm.hpp"
#include "volk.h"
#include "vk_mem_alloc.h"

struct ShaderData {
	glm::mat4 proj;
	glm::mat4 view;
	glm::mat4 model[3];
	glm::vec4 lightPos { 0.0f, -10.0f, 10.0f, 0.0f };
	uint32_t selected = 1;
};

struct ShaderDataBuffer {
	VmaAllocation allocation = VK_NULL_HANDLE;
	VmaAllocationInfo allocationInfo {};

	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceAddress bufferDeviceAddress = {};
};
