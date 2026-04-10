#pragma once

#include "volk.h"
#include "SDL3/SDL.h"

#include <vector>

struct Platform {
	SDL_Window* window = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	bool vulkanLoaded = false;
};

bool Platform_Init(Platform& platform, const char* title, uint32_t width, uint32_t height);

void Platform_Destroy(Platform& platform);

VkSurfaceKHR Platform_CreateVulkanSurface(Platform& platform, VkInstance instance);

std::vector<const char*> Platform_GetRequiredExtensions(Platform& platform);

