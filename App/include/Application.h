#pragma once

#include "Platform.h"
#include "VulkanContext.h"
#include "ShaderData.h"
#include "glm/fwd.hpp"

class Application {
public:
	constexpr static int WIDTH = 1280;
	constexpr static int HEIGHT = 720;

public:
	Application() = default;
	~Application();

	void Run();

private:
	bool Init();
	void Shutdown();

	void MainLoop();
	void Render();

private:
	Platform mPlatform;
	VulkanContext mVulkanContext;
	ShaderData mShaderData;

	std::array<glm::vec3, 3> mObjRotations = {
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f)
	};

	glm::vec3 mCamPos = glm::vec3(0.0f, 0.0f, -6.0f);

	bool mIsRunning = false;
	bool mUpdateSwapchain = false;
};
