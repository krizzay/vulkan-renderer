#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// singelton struct with common vulkan things which are shared/all used in the different files
struct VkState {

	VkSurfaceKHR surface;
    GLFWwindow* window; //TODO: raw ptr very naughty

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	VkRenderPass renderPass;

	// commands
		VkQueue graphicsQueue;
		VkQueue presentQueue;

		VkCommandPool commandPool;
		std::vector<VkCommandBuffer> commandBuffers;

		VkCommandBuffer setupBuffer;

	// swapchain
		VkSwapchainKHR swapchain;
		std::vector<VkImage> swapchainImages;
		VkFormat swapchainImageFormat;
		VkExtent2D swapchainExtent;
		std::vector<VkImageView> swapchainImageViews;
		std::vector<VkFramebuffer> swapchainFramebuffers;

	// pipelines
		VkPipelineLayout particlePipelineLayout;
		VkPipeline particleGraphicsPipeline;

		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;

	// sync
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		uint32_t currentFrame = 0;

	// runtime
		bool resetPos = false;
		bool cursorFree = false;
		bool framebufferResized = false;

		glm::vec3 movement;
		glm::vec2 oldPos = {0,0};
		glm::vec2 moveAmount = glm::vec2(0, 1.570795);

	static VkState& getInstance() {
		static VkState instance;
		return instance;
	}

private:
	VkState() {}
};
