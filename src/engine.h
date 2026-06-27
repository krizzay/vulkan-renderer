#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "compute.h"
#include "structs.h"
#include "window.h"

class Engine {
	public:
#ifdef NDEBUG
		const bool enableValidationLayers = false;
#else
		const bool enableValidationLayers = true;
#endif
    	VkSurfaceKHR surface;

		bool resetPos = false;
		bool cursorFree = false;
		bool framebufferResized = false;

		// camera things
		glm::vec3 movement;
		glm::vec2 oldPos = {0,0};
		glm::vec2 moveAmount = glm::vec2(0, 1.570795);

		VkInstance instance;
    	VkDebugUtilsMessengerEXT debugMessenger;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device;

		VkQueue graphicsQueue;
		VkQueue presentQueue;
    
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;

		std::unique_ptr<Compute> compute;

		Engine(uint32_t width, uint32_t height);
		~Engine();

		void createInstance();
		void setupDebugMessenger(); 
		void createSurface(); 
		void pickPhysicalDevice(); 
		void createLogicalDevice(); 
		void initCompute(const uint32_t particleCount, const int maxFramesInFlight);
		void createSwapChain(); 

		void createImageViews(); // image view for swap chain 
		void createRenderPass(); 
		void createGraphicsPipeline(); 
		void createParticleGraphicsPipeline(); 
		void createCommandPool(); 

		//void setupCommandBuffer();//my garbage code // see if faster 

		void createFramebuffers(); 

		void createSyncObjects(); 

		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		bool isDeviceSuitable(VkPhysicalDevice device);
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    	VkSampleCountFlagBits getMaxUsableSampleCount();
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	private:

		const std::vector<const char*> validationLayers = {
			"VK_LAYER_KHRONOS_validation"//,
			//"VK_LAYER_LUNARG_monitor"    //     this validation layer doesnt work on linux even though apparently it should?? idk man
		};

		const std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		const uint32_t WIDTH = 800;
		const uint32_t HEIGHT = 600;



};
