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

#include "vk-state.h"
#include "compute.h"
#include "resources.h"
#include "structs.h"
#include "window.h"

class Engine {
	public:
#ifdef NDEBUG
		const bool enableValidationLayers = false;
#else
		const bool enableValidationLayers = true;
#endif
		std::unique_ptr<Compute> compute;
		std::unique_ptr<Resources> resources;
		std::unique_ptr<Window> window;
		VkState& state;

	public:
		Engine(uint32_t width, uint32_t height, uint32_t max_frames_in_flight, uint32_t particle_count);
		~Engine();

		void createInstance();
		void setupDebugMessenger(); 
		void createSurface(); 
		void initWindow();
		void pickPhysicalDevice(); 
		void createLogicalDevice(); 
		void initCompute(const uint32_t particleCount, const int maxFramesInFlight);
		void initResources();
		void createSwapchain(); 

		void setResourcesThings();

		void createImageViews(); // image view for swap chain 
		void createRenderPass(); 
		void createGraphicsPipeline(); 
		void createParticleGraphicsPipeline(); 
		void createCommandPool(); 

		//void setupCommandBuffer(); // consider

		void createFramebuffers(); 

		void createCommandBuffers();
		void createSyncObjects(); 

		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

		void initVulkan();
		void cleanupSwapChain();
		void cleanup();
		void recreateSwapChain();
		void initImgui();

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
		const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
		const uint32_t PARTICLE_COUNT = 2;

		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		bool isDeviceSuitable(VkPhysicalDevice device);

};
