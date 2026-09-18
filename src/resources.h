#include "engine-utils.h"
#include "structs.h"

#include <iostream>
#include <vector>
#include <array>
#include <cstring>
#include <memory>
#include <set>
#include <algorithm>
#include <optional>

class Resources {
	public:
		VkDescriptorSetLayout globalDescriptorSetLayout;
		VkDescriptorSetLayout objectDescriptorSetLayout;

		VkBuffer indexVertexBuffer;
		VkDeviceMemory indexVertexBufferMemory;

		std::vector<VkBuffer> shaderStorageBuffers;
		std::vector<VkDeviceMemory> shaderStorageBuffersMemory;

		VkBuffer instanceBuffer;
		VkDeviceMemory instanceBufferMemory;
		uint32_t instanceCount = 100;

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<uint32_t> indicieOffsets;
		std::vector<uint32_t> vertexOffsets;
		uint32_t indiciesSize;

		VkImage depthImage;
		VkDeviceMemory depthImageMemory;
		VkImageView depthImageView;

		VkImage colorImage;
		VkDeviceMemory colorImageMemory;
		VkImageView colorImageView;

		std::vector<uint32_t> mipLevels;
		std::vector<VkImage> textureImages;
		std::vector<VkDeviceMemory> textureImagesMemory;

		std::vector<VkImageView> textureImageViews;
		std::vector<VkSampler> textureSamplers;

		std::vector<VkBuffer> globalUniformBuffers;
		std::vector<VkDeviceMemory> globalUniformBuffersMemory;
		std::vector<std::vector<VkBuffer>> uniformBuffers;
		std::vector<std::vector<VkDeviceMemory>> uniformBuffersMemory;

		const std::string modelPath = "../models/";
		const std::string texturePath = "../textures/";

		// render objects
		std::vector<object> objects = { 
			//object("models/aubrey.obj", "textures/aubrey.png", glm::vec3(0,0,0), glm::vec3(0.5, 0, 0.5), true, 1),
			//object("models/model.obj", "textures/texture.png", glm::vec3(0,80,0), glm::vec3(0, 0.5, 0.5), true, 5),
			object(modelPath + "aubrey.obj", texturePath + "aubrey.png", glm::vec3(0,1000,0), 1),
			object(modelPath + "/cubeoid.obj",texturePath + "debug.png", glm::vec3(0, -500, 0), 4),
			object(modelPath + "/aubrey.obj", texturePath + "aubrey.png", glm::vec3(0,500,0), 2),
			object(modelPath + "/aubrey.obj", texturePath + "aubrey.png", glm::vec3(0,0,0), 4),
			object(modelPath + "/aubrey.obj", texturePath + "aubrey.png", glm::vec3(0,-1000,0), 2),
			object(modelPath + "/aubrey.obj", texturePath + "aubrey.png", glm::vec3(0,-1500,0), 2),
			object(modelPath + "/aubrey.obj", texturePath + "aubrey.png", glm::vec3(0,0,5), 2, 100)
    	};

		VkDescriptorPool descriptorPool;
		std::vector<std::vector<VkDescriptorSet>> objectDescriptorSets;
		// objectDescriptorSets[object index][frame in flight index]
		std::vector<VkDescriptorSet> globalDescriptorSets;

	public:
		Resources(VkDevice& device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue);
		~Resources() = default;

		void createDescriptorSetLayout();

		void createShaderStorageBuffers(uint32_t particle_count);
		void createIndexVertexBuffer();
		void createInstanceBuffer();

		void createColorResources();
		void createDepthResources();

		void createTextureImages();
		void createUniformBuffers();
		void createTextureImageViews();
		void createTextureSampler();

		void loadModel();

		void createDescriptorPool();
		void createDescriptorSets();

		void setCommandPool(std::shared_ptr<VkCommandPool>& commandPool);
		void setSwapchainExtent(std::shared_ptr<VkExtent2D>& swapChainExtent);
		void setSwapchainImageFormat(std::shared_ptr<VkFormat>& swapChainImageFormat);
		void setMsaaSamples(std::shared_ptr<VkSampleCountFlagBits>& msaaSample);

	private:
		std::shared_ptr<VkDevice> m_device; 
		std::shared_ptr<VkPhysicalDevice> m_physicalDevice;

		std::optional<std::shared_ptr<VkExtent2D>> m_swapchainExtent;
		std::optional<std::shared_ptr<VkFormat>> m_swapchainImageFormat;
		std::optional<std::shared_ptr<VkSampleCountFlagBits>> m_msaaSamples;
		std::optional<std::shared_ptr<VkCommandPool>> m_commandPool;
		std::shared_ptr<VkQueue> m_graphicsQueue;

		uint32_t WIDTH = 800;
		uint32_t HEIGHT = 600;

		uint32_t PARTICLE_COUNT = 1024;
		uint32_t MAX_FRAMES_IN_FLIGHT = 3;

};
