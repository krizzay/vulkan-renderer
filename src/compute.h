#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Compute {
	public:
		VkQueue m_queue;
		std::vector<VkSemaphore> m_finishedSemaphores;
		std::vector<VkFence> m_inFlightFences;
		std::vector<VkCommandBuffer> m_commandBuffers;

		Compute(VkDevice device, int maxFramesInFlight, uint32_t particleCount);
		~Compute();

		// setup
		void CreateComputeDescriptorSetLayout(); 
		void createComputePipeline(); 
		void createComputeDescriptorSets(VkDescriptorPool descriptorPool, std::vector<VkBuffer> shaderStorageBuffers); 
		void createComputeCommandBuffers(VkCommandPool commandPool); 
		void createComputeSyncObjects();
		void createComputeUniformBuffers(VkPhysicalDevice physicalDevice);

		// runtime
		void resetCommandBuffer(uint32_t currentFrame, uint32_t commandBufferResetFlagBits);
		void updateComputeUniformBuffer(uint32_t currentImage);
		void recordComputeCommandBuffer(float deltaTime, uint32_t currentFrame);
		void submitCommandBuffer(uint32_t currentFrame, VkSubmitInfo submitInfo);
									
	private:

		VkDevice m_device;

		// maybe make const?
		int m_maxFramesInFlight;
		uint32_t m_particleCount;

		VkDescriptorSetLayout m_descriptorSetLayout;
		VkPipelineLayout m_pipelineLayout;
		VkPipeline m_pipeline;

		std::vector<VkBuffer> m_uniformBuffers;
		std::vector<VkDeviceMemory> m_uniformBuffersMemory;

		std::vector<VkDescriptorSet> m_descriptorSets;


};	
													



