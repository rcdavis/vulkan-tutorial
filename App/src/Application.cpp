#include "Application.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "Utils/Log.h"
#include "Utils/DebugUtils.h"

#include <array>

static constexpr std::array ValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef DEBUG
static constexpr bool EnableValidationLayers = true;
#else
static constexpr bool EnableValidationLayers = false;
#endif

static constexpr uint32_t WindowWidth = 800;
static constexpr uint32_t WindowHeight = 600;

Application::Application() :
	mInstance(VK_NULL_HANDLE),
	mDebugMessenger(VK_NULL_HANDLE),
	mWindow(nullptr)
{}

Application::~Application() {
	Cleanup();
}

void Application::Run() {
	InitWindow();
	InitVulkan();
	MainLoop();
}

void Application::InitWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	mWindow = glfwCreateWindow(WindowWidth, WindowHeight, "Vulkan", nullptr, nullptr);
}

void Application::InitVulkan() {
	volkInitialize();
	CreateInstance();
	SetupDebugMessenger();
}

void Application::MainLoop() {
	while (!glfwWindowShouldClose(mWindow)) {
		glfwPollEvents();

		glfwSwapBuffers(mWindow);
	}
}

void Application::Cleanup() {
	if constexpr (EnableValidationLayers) {
		if (mDebugMessenger != VK_NULL_HANDLE) {
			vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
			//DebugUtils::DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
			mDebugMessenger = VK_NULL_HANDLE;
		}
	}

	if (mInstance != VK_NULL_HANDLE) {
		vkDestroyInstance(mInstance, nullptr);
		mInstance = VK_NULL_HANDLE;
	}

	glfwTerminate();
	mWindow = nullptr;
}

void Application::CreateInstance() {
	if constexpr (EnableValidationLayers) {
		if (!CheckValidationLayerSupport())
			throw std::runtime_error("Validation layers requested, but not available!");
	}

	constexpr VkApplicationInfo appInfo {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_4
	};

	const auto extensions = GetRequiredExtensions();

	VkInstanceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = (uint32_t)std::size(extensions),
		.ppEnabledExtensionNames = std::data(extensions)
	};

	if constexpr (EnableValidationLayers) {
		createInfo.enabledLayerCount = (uint32_t)std::size(ValidationLayers);
		createInfo.ppEnabledLayerNames = std::data(ValidationLayers);
	}

	if (const VkResult result = vkCreateInstance(&createInfo, nullptr, &mInstance); result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan Instance");
	}

	volkLoadInstance(mInstance);
}

bool Application::CheckValidationLayerSupport() {
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, std::data(availableLayers));

	for (const char* layerName : ValidationLayers) {
		bool layerFound = false;

		for (const VkLayerProperties& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
			return false;
	}

	return true;
}

std::vector<const char*> Application::GetRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** const glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if constexpr (EnableValidationLayers)
		extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}

void Application::SetupDebugMessenger() {
	if constexpr (!EnableValidationLayers)
		return;

	constexpr VkDebugUtilsMessageSeverityFlagsEXT severityFlags =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

	constexpr VkDebugUtilsMessageTypeFlagsEXT typeFlags =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	constexpr VkDebugUtilsMessengerCreateInfoEXT createInfo {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = nullptr,
		.flags = 0,
		.messageSeverity = severityFlags,
		.messageType = typeFlags,
		.pfnUserCallback = DebugUtils::VulkanDebugCallback,
		.pUserData = nullptr
	};

	//const VkResult result = DebugUtils::CreateDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mDebugMessenger);
	const VkResult result = vkCreateDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mDebugMessenger);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to setup debug messenger!");
	}
}
