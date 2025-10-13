#pragma once
#include "vulkan/vulkan.h"
#include <vector>

class compute {
	public:

		compute(VkDevice device, int maxFramesInFlight);
		~compute();

		struct ComputeUniformBufferObject; 

		void CreateComputeDescriptorSetLayout(); 
		void createComputePipeline(); 
		void createComputeDescriptorSets(); 
		void createComputeCommandBuffers(); 
									
	private:

    	VkDevice m_device;
		VkQueue m_computeQueue;

		// maybe make const?
		int m_maxFramesInFlight;

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
													



