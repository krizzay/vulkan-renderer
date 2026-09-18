#include "resources.h"
#include "structs.h"
#include "engine-utils.h"

#include <iostream>
#include <vector>
#include <array>
#include <cstring>
#include <cmath>
#include <memory>
#include <set>
#include <algorithm>
#include <random>
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#include "stb-master/stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <glm/gtx/hash.hpp>

// vertex must be hashable for unordered map
namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

namespace {

    void copyBuffer(VkBuffer srcBuffer, 
					VkBuffer dstBuffer, 
					VkDeviceSize size, 
					VkDevice device, 
					VkCommandPool commandPool, 
					VkQueue graphicsQueue) 
	{
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(device, commandBuffer, commandPool, graphicsQueue);

        //A fence would allow you to schedule multiple transfers simultaneously and wait for all of them complete, 
        //instead of executing one at a time. That may give the driver more opportunities to optimize.
    }

    void copyBuffer(VkBuffer srcBuffer, 
					VkDeviceSize srcOffset, 
					VkBuffer dstBuffer, 
					VkDeviceSize dstOffset, 
					VkDeviceSize size, 
					VkDevice device, 
					VkCommandPool commandPool, 
					VkQueue graphicsQueue) 
	{
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = srcOffset;
        copyRegion.dstOffset = dstOffset;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(device, commandBuffer, commandPool, graphicsQueue);

        //A fence would allow you to schedule multiple transfers simultaneously and wait for all of them complete, 
        //instead of executing one at a time. That may give the driver more opportunities to optimize.
    }

    void createImage(uint32_t width, 
					 uint32_t height, 
					 uint32_t mipLevels, 
					 VkSampleCountFlagBits 
					 numSamples, 
					 VkFormat format, 
        			 VkImageTiling tiling, 
					 VkImageUsageFlags usage,
					 VkMemoryPropertyFlags properties, 
					 VkImage& image, 
					 VkDeviceMemory& imageMemory,
					 std::shared_ptr<VkDevice> device,
					 std::shared_ptr<VkPhysicalDevice> physicalDevice) {

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;//2d for textures // 3d for voxel volumes // 1d for arrays of data or gradient
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = numSamples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(*device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(*device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, 
												   properties, 
												   *physicalDevice);

        if (vkAllocateMemory(*device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(*device, image, imageMemory, 0);
    }

    void transitionImageLayout(VkImage image, 
							   VkFormat format, 
							   VkImageLayout oldLayout, 
							   VkImageLayout newLayout, 
							   uint32_t mipLevels,
							   std::shared_ptr<VkDevice> device,
					 		   std::shared_ptr<VkCommandPool> commandPool,
							   std::shared_ptr<VkQueue> graphicsQueue)
	{
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(*device, *commandPool);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            //setupBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        endSingleTimeCommands(*device, commandBuffer, *commandPool, *graphicsQueue);

        //flushSetupCommands();
    }

    void copyBufferToImage(VkBuffer buffer, 
						   VkImage image, 
						   uint32_t width, 
						   uint32_t height,
						   std::shared_ptr<VkDevice> device,
					 	   std::shared_ptr<VkCommandPool> commandPool,
						   std::shared_ptr<VkQueue> graphicsQueue)
	{
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(*device, *commandPool);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

		
        endSingleTimeCommands(*device, commandBuffer, *commandPool, *graphicsQueue);
    }

    void generateMipmaps(VkImage image, 
						 VkFormat imageFormat, 
						 int32_t texWidth, 
						 int32_t texHeight, 
						 uint32_t mipLevels,
					   	 std::shared_ptr<VkDevice> device,
						 std::shared_ptr<VkPhysicalDevice> physicalDevice,
					 	 std::shared_ptr<VkCommandPool> commandPool,
						 std::shared_ptr<VkQueue> graphicsQueue)
	{
        // Check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(*physicalDevice, imageFormat, &formatProperties);
        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }
        //TODO: implement a function that searches common texture image formats for one that does support linear blitting

        VkCommandBuffer commandBuffer = beginSingleTimeCommands(*device, *commandPool);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);
            
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
            

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }
        
        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);
        

        endSingleTimeCommands(*device, commandBuffer, *commandPool, *graphicsQueue);
    }

	inline double lerp(double a, double b, double t) {
		return (1 - t) * a + t * b;
	}
}// namespace

Resources::Resources(VkDevice& device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue) {

	m_device = std::make_shared<VkDevice>(device);
	m_physicalDevice = std::make_shared<VkPhysicalDevice>(physicalDevice);
	m_graphicsQueue = std::make_shared<VkQueue>(graphicsQueue);
}

void Resources::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 1> bindingsGlobal = { uboLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindingsGlobal.size());
	layoutInfo.pBindings = bindingsGlobal.data();

	if (vkCreateDescriptorSetLayout(*m_device, &layoutInfo, nullptr, &globalDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindingsObject = { uboLayoutBinding, samplerLayoutBinding };

	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindingsObject.size());
	layoutInfo.pBindings = bindingsObject.data();

	if (vkCreateDescriptorSetLayout(*m_device, &layoutInfo, nullptr, &objectDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void Resources::createShaderStorageBuffers(uint32_t particle_count) {

	if(!m_commandPool){
		std::cerr << "no command pool assigned in resources!" << std::endl;
		return;
	}

	// Initialize particles
	std::default_random_engine rndEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

	// Initial particle positions on a circle
	std::vector<Particle> particles(particle_count);
	for (auto& particle : particles) {
		float r = 0.25f * sqrt(rndDist(rndEngine));
		float theta = rndDist(rndEngine) * 2 * 3.14159265358979323846;
		float x = r * cos(theta) * HEIGHT / WIDTH;
		float y = r * sin(theta);
		particle.position = glm::vec2(x, y);
		particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00005f;
		particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);

	}

	VkDeviceSize bufferSize = sizeof(Particle) * particle_count;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, stagingBufferMemory, *m_device, *m_physicalDevice);

	void* data;
	vkMapMemory(*m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, particles.data(), (size_t)bufferSize);
	vkUnmapMemory(*m_device, stagingBufferMemory);

	shaderStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	shaderStorageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT 
				| VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
				shaderStorageBuffers[i], shaderStorageBuffersMemory[i], *m_device, *m_physicalDevice);

		// Copy data from the staging buffer (host) to the shader storage buffer (GPU)
		copyBuffer(stagingBuffer, 
					shaderStorageBuffers[i], 
					bufferSize, 
					*m_device, 
					**m_commandPool, 
					*m_graphicsQueue);
	}

	vkDestroyBuffer(*m_device, stagingBuffer, nullptr);
	vkFreeMemory(*m_device, stagingBufferMemory, nullptr);
}

void Resources::createIndexVertexBuffer() {
	//create vertex transfer source buffer

	VkDeviceSize vertexSourceSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer vertexStagingBuffer;
	VkDeviceMemory vertexStagingBufferMemory;
	createBuffer(vertexSourceSize, 
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
				| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				vertexStagingBuffer, 
				vertexStagingBufferMemory, 
				*m_device,	
				*m_physicalDevice);

	void* data;
	vkMapMemory(*m_device, vertexStagingBufferMemory, 0, vertexSourceSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)vertexSourceSize);
	vkUnmapMemory(*m_device, vertexStagingBufferMemory);

	//create index transfer source buffer
	VkDeviceSize indexSourceSize = sizeof(indices[0]) * indices.size();
	indiciesSize = indexSourceSize;

	VkBuffer indexStagingBuffer;
	VkDeviceMemory indexStagingBufferMemory;
	createBuffer(indexSourceSize, 
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
				| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				indexStagingBuffer, 
				indexStagingBufferMemory, 
				*m_device, 
				*m_physicalDevice);

	vkMapMemory(*m_device, indexStagingBufferMemory, 0, indexSourceSize, 0, &data);
	memcpy(data, indices.data(), (size_t)indexSourceSize);
	vkUnmapMemory(*m_device, indexStagingBufferMemory);

	//copy to index vertex buffer
	createBuffer(indexSourceSize + vertexSourceSize, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT 
				| VK_BUFFER_USAGE_VERTEX_BUFFER_BIT 
				| VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
				indexVertexBuffer, 
				indexVertexBufferMemory, 
				*m_device, 
				*m_physicalDevice);

	copyBuffer(indexStagingBuffer, 
				indexVertexBuffer, 
				indexSourceSize,
			   	*m_device, 
				**m_commandPool, 
				*m_graphicsQueue);

	copyBuffer(vertexStagingBuffer, 
				0, 
				indexVertexBuffer, 
				indexSourceSize, 
				vertexSourceSize, 
				*m_device, 
				**m_commandPool, 
				*m_graphicsQueue);

	vkDestroyBuffer(*m_device, vertexStagingBuffer, nullptr);
	vkFreeMemory(*m_device, vertexStagingBufferMemory, nullptr);
	vkDestroyBuffer(*m_device, indexStagingBuffer, nullptr);
	vkFreeMemory(*m_device, indexStagingBufferMemory, nullptr);

	vertices.clear();
	indices.clear();
}

/*
	It should be noted that in a real world application, you're not supposed to actually call vkAllocateMemory 
	for every individual buffer. The maximum number of simultaneous memory allocations is limited by the 
	maxMemoryAllocationCount. The right way to allocate memory for a large number of objects at the same time is to 
	create a custom allocator that splits up a single allocation among many different objects by using the 
	offset parameters that we've seen in many functions.
	
	as a challenge make a allocator yeah? future me? i know you may hate me for this or ignore but like dont be a bitch
	blud i would but like how tho, like is the time investement worth it? like what is hapening
*/

void Resources::createInstanceBuffer() {

	if(!m_commandPool){
		std::cerr << "no command pool assigned in resources!" << std::endl;
		return;
	}

	std::vector<glm::vec3> instanceData;

	//position offset tings
	float baseOffset = 100.0f;
	glm::vec3 posOffset;
	posOffset.y = 0;
	posOffset.z = 0;

	//color offset tings
	glm::vec3 colOffset = {0,0,0};

	glm::vec3 colx1 = { 0,0.0862745,1 };
	glm::vec3 colx2 = { 1,0.9137254,0 };
	glm::vec3 coly1 = { 1,0,0.7843137 };
	glm::vec3 coly2 = { 0,1,0.2156863 };

	instanceData.push_back(glm::vec3(0, 0, 0));
	instanceData.push_back(glm::vec3(1, 100, 1));

	for (int i = -(sqrtf(instanceCount)*0.5); i < (sqrtf(instanceCount)*0.5); i++)
	{
		for (int e = -(sqrtf(instanceCount)*0.5); e < (sqrtf(instanceCount)*0.5); e++)
		{
			posOffset.x = (baseOffset * i);
			posOffset.z = (baseOffset * e);

			instanceData.push_back(posOffset);


			float in = (i+15) / 30.0f;
			float en = (e+15) / 30.0f; 
			
			colOffset.x = lerp(lerp(coly1.x, coly2.x, in),colx1.x,en);
			colOffset.y = lerp(lerp(coly1.y, coly2.y, in),colx2.y,1-en);
			colOffset.z = lerp(coly1.z, coly2.z, in);
		   
			/*
			colOffset.x = exp(-pow((in--), 2) * 2.8);
			colOffset.z = exp(-pow((in++), 2) * 2.8);
			colOffset.y = 1-(colOffset.x+colOffset.y);
			*/

			instanceData.push_back(colOffset);
				  
		}                                            
	}

	VkDeviceSize bufferSize = sizeof(glm::vec3) * instanceData.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, *m_device, 
		*m_physicalDevice);

	void* data;
	vkMapMemory(*m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, instanceData.data(), (size_t)bufferSize);
	vkUnmapMemory(*m_device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, instanceBuffer, instanceBufferMemory, *m_device, 
		*m_physicalDevice);

	copyBuffer(stagingBuffer, 
				instanceBuffer, 
				bufferSize, 
				*m_device, 
				**m_commandPool, 
				*m_graphicsQueue);

	vkDestroyBuffer(*m_device, stagingBuffer, nullptr);
	vkFreeMemory(*m_device, stagingBufferMemory, nullptr);

	
}

void Resources::createColorResources() {

	if(!m_swapchainImageFormat || !m_swapchainExtent || !m_msaaSamples) {
		throw std::runtime_error("createColorResources is missing needed state\n[swapchainImageFormat / swapchainExtent / msaaSamples]");
	}

	VkFormat colorFormat = **m_swapchainImageFormat;

	createImage((**m_swapchainExtent).width, 
				(**m_swapchainExtent).height, 
				1, **m_msaaSamples, 
				colorFormat, 
				VK_IMAGE_TILING_OPTIMAL, 
				VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
				colorImage, 
				colorImageMemory,
				m_device,
				m_physicalDevice);

	colorImageView = createImageView(*m_device, 
									 colorImage, 
									 colorFormat, 
									 VK_IMAGE_ASPECT_COLOR_BIT, 
									 1);
}

void Resources::createDepthResources()
{
	if(!m_swapchainExtent || !m_msaaSamples) {
		throw std::runtime_error("createDepthResources is missing needed state\n[swapchainExtent / msaaSamples]");
	}

	VkFormat depthFormat = findDepthFormat(*m_physicalDevice);

	createImage((**m_swapchainExtent).width, 
				(**m_swapchainExtent).height, 
				1, 
				**m_msaaSamples, 
				depthFormat, 
				VK_IMAGE_TILING_OPTIMAL, 
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
				depthImage, 
				depthImageMemory,
				m_device,
				m_physicalDevice);

	depthImageView = createImageView(*m_device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void Resources::createTextureImages() {
	textureImages.resize(objects.size());
	textureImagesMemory.resize(objects.size());

	for (size_t i = 0; i < objects.size(); i++) {

		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(objects[i].texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;
		//mipLevels[i] = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
		mipLevels.push_back(static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1);

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(imageSize, 
					 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
					 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
					 stagingBuffer, 
					 stagingBufferMemory, 
					 *m_device, 
					 *m_physicalDevice);

		void* data;
		vkMapMemory(*m_device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(*m_device, stagingBufferMemory);

		stbi_image_free(pixels);

		createImage(texWidth, 
					texHeight, 
					mipLevels[i], 
					VK_SAMPLE_COUNT_1_BIT, 
					VK_FORMAT_R8G8B8A8_SRGB, 
					VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_TRANSFER_SRC_BIT | 
					VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
					VK_IMAGE_USAGE_SAMPLED_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
					textureImages[i], 
					textureImagesMemory[i],
					m_device,
					m_physicalDevice);

		transitionImageLayout(textureImages[i], 
							  VK_FORMAT_R8G8B8A8_SRGB, 
							  VK_IMAGE_LAYOUT_UNDEFINED, 
							  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
							  mipLevels[i],
							  m_device,
							  *m_commandPool,
							  m_graphicsQueue);

		copyBufferToImage(stagingBuffer, 
						  textureImages[i], 
						  static_cast<uint32_t>(texWidth), 
						  static_cast<uint32_t>(texHeight),
						  m_device,
						  *m_commandPool,
					  	  m_graphicsQueue);

		vkDestroyBuffer(*m_device, stagingBuffer, nullptr);
		vkFreeMemory(*m_device, stagingBufferMemory, nullptr);

		generateMipmaps(textureImages[i], 
						VK_FORMAT_R8G8B8A8_SRGB, 
						texWidth, 
						texHeight, 
						mipLevels[i],
						m_device,
						m_physicalDevice,
						*m_commandPool,
						m_graphicsQueue);

	}
}

void Resources::createUniformBuffers() {
	//create global uniform buffer
	VkDeviceSize bufferSize = sizeof(GlobalUniformBufferObject);

	globalUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	globalUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		createBuffer(bufferSize, 
					 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
					 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
					 globalUniformBuffers[i], 
					 globalUniformBuffersMemory[i], 
					 *m_device, 
					 *m_physicalDevice);
	}

	bufferSize = sizeof(UniformBufferObject);
	//create object uniform buffers

	uniformBuffers.resize(objects.size());
	uniformBuffersMemory.resize(objects.size());
	for (int i = 0; i < objects.size(); i++)
	{
		uniformBuffers[i].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemory[i].resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++) {
			createBuffer(bufferSize, 
						 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
						 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
						 uniformBuffers[i][j], 
						 uniformBuffersMemory[i][j], 
						 *m_device, 
						 *m_physicalDevice);
		}
	}
}

void Resources::createTextureImageViews() {
	textureImageViews.resize(objects.size());

	for (int i = 0; i < objects.size(); i++) {
		textureImageViews[i] = createImageView(*m_device, 
											   textureImages[i], 
											   VK_FORMAT_R8G8B8A8_SRGB, 
											   VK_IMAGE_ASPECT_COLOR_BIT, 
											   mipLevels[i]);
	}
}    

void Resources::createTextureSampler() {
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;//VK_FILTER_NEAREST
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(*m_physicalDevice, &properties);

	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	const size_t numObjects = objects.size();

	textureSamplers.resize(numObjects);
	for (size_t i = 0; i < numObjects; i++)
	{
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.minLod = 0.0f; // Optional
		samplerInfo.maxLod = static_cast<float>(mipLevels[i]);
		samplerInfo.mipLodBias = 0.0f; // Optional

		if (vkCreateSampler(*m_device, &samplerInfo, nullptr, &textureSamplers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}

	}
}

void Resources::loadModel() {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	indicieOffsets.push_back(0);
	vertexOffsets.push_back(0);

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (size_t i = 0; i < objects.size(); i++) {

		if (!tinyobj::LoadObj(&attrib, 
							  &shapes, 
							  &materials, 
							  &warn, 
							  &err, 
							  objects[i].modelPath.c_str())) 
		{
			throw std::runtime_error(warn + err);
		}

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.color = { 1.0f, 1.0f, 1.0f };

				vertex.normal = {
					attrib.normals[3 * index.vertex_index + 0],
					attrib.normals[3 * index.vertex_index + 1],
					attrib.normals[3 * index.vertex_index + 2]
				};
				//could fuck up if 2 models have the same vertex
				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
		}

		indicieOffsets.push_back(indices.size());
		vertexOffsets.push_back(vertices.size());
	}
}

void Resources::createDescriptorPool() { 

	const size_t numObjects = objects.size();

	// defines shit for different descriptor types
	std::array<VkDescriptorPoolSize, 3> poolSizes{};

	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>((MAX_FRAMES_IN_FLIGHT * numObjects)
		   												 + MAX_FRAMES_IN_FLIGHT * 2);
	//one ubo per object per frame in flight
	//compute -> ubo per frame in flight
	//global ubo per frame in flight
	
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 
			 											 numObjects + MAX_FRAMES_IN_FLIGHT);
	//one combined image sampler per object per framne in flight
	//another one per frame in flight for imgui

	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size()); // num of different descriptors
	poolInfo.pPoolSizes = poolSizes.data();                                                                 
	poolInfo.maxSets = static_cast<uint32_t>((MAX_FRAMES_IN_FLIGHT * numObjects) 
			 								 + MAX_FRAMES_IN_FLIGHT *3); 
	//one set per object per frame in flight
	//compute -> set per frame in flight
	//global set per frame in flight
	//imgui set per frame in flight
	
	if (vkCreateDescriptorPool(*m_device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void Resources::createDescriptorSets() {

	const size_t numObjects = objects.size();

	//create MAX_FRAMES_IN_FLIGHT sets per object
	objectDescriptorSets.resize(numObjects);

	for (int j = 0; j < numObjects; j++)
	{
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, objectDescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		objectDescriptorSets[j].resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(*m_device, &allocInfo, objectDescriptorSets[j].data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets! graphics");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[j][i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureImageViews[j];
			imageInfo.sampler = textureSamplers[j];

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = objectDescriptorSets[j][i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = objectDescriptorSets[j][i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(*m_device, 
								   static_cast<uint32_t>(descriptorWrites.size()), 
								   descriptorWrites.data(), 
								   0, 
								   nullptr);
		}

	}
	//create global ubo

	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, globalDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	globalDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(*m_device, &allocInfo, globalDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets! graphics, global ubo");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = globalUniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(GlobalUniformBufferObject); // ! should be global ubo

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = globalDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(*m_device, 
							   static_cast<uint32_t>(descriptorWrites.size()), 
							   descriptorWrites.data(), 
							   0, 
							   nullptr);
	}
}

void Resources::setCommandPool(std::shared_ptr<VkCommandPool>& commandPool){
	m_commandPool.emplace(commandPool);
}

void Resources::setSwapchainExtent(std::shared_ptr<VkExtent2D>& swapChainExtent){
	m_swapchainExtent.emplace(swapChainExtent);
}

void Resources::setSwapchainImageFormat(std::shared_ptr<VkFormat>& swapChainImageFormat) {
	m_swapchainImageFormat.emplace(swapChainImageFormat);
}

void Resources::setMsaaSamples(std::shared_ptr<VkSampleCountFlagBits>& msaaSample) {
	m_msaaSamples.emplace(msaaSample);
}
