#pragma once
#include "vulkan/vulkan.h"
#include <vector>

class Compute {
	public:

		Compute(VkDevice device, int maxFramesInFlight, uint32_t particleCount);
		~Compute();

		void CreateComputeDescriptorSetLayout(); 
		void createComputePipeline(); 
		void createComputeDescriptorSets(); 
		void createComputeCommandBuffers(); 
		void createComputeSyncObjects();

		void updateComputeUniformBuffer();
		void recordComputeCommandBuffer();
									
	private:

    		VkDevice m_device;
		VkQueue m_computeQueue;

		// maybe make const?
		int m_maxFramesInFlight;
		uint32_t m_particleCount

	    	VkDescriptorSetLayout m_computeDescriptorSetLayout;
    		VkPipelineLayout m_computePipelineLayout;
		VkPipeline m_computePipeline;

		std::vector<VkBuffer> m_computeUniformBuffers;
		std::vector<VkDeviceMemory> m_computeUniformBuffersMemory;

		std::vector<VkDescriptorSet> m_computeDescriptorSets;

		std::vector<VkCommandBuffer> m_computeCommandBuffers;

		std::vector<VkSemaphore> m_computeFinishedSemaphores;
		std::vector<VkFence> m_computeInFlightFences;
}	
													



