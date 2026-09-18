#pragma once
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include <string>
#include <vector>

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
									  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	   							      const VkAllocationCallbacks* pAllocator, 
									  VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance, 
								 	VkDebugUtilsMessengerEXT debugMessenger, 
									const VkAllocationCallbacks* pAllocator);

bool checkValidationLayerSupport(std::vector<const char*>);

std::vector<const char*> getRequiredExtensions(bool enableValidationLayers);

std::vector<char> readFile(const std::string& filename);

VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);

void createBuffer(VkDeviceSize size, 
				  VkBufferUsageFlags usage, 
				  VkMemoryPropertyFlags properties,
				  VkBuffer& buffer, 
				  VkDeviceMemory& bufferMemory, 
				  VkDevice device, 
				  VkPhysicalDevice physicalDevice);

uint32_t findMemoryType(uint32_t typeFilter, 
						VkMemoryPropertyFlags properties, 
						VkPhysicalDevice physicalDevice); 

bool checkDeviceExtensionSupport(VkPhysicalDevice device, 
								const std::vector<const char*> deviceExtensions);

VkImageView createImageView(VkDevice device, 
							VkImage image, 
							VkFormat format, 
							VkImageAspectFlags aspectFlags, 
							uint32_t mipLevels);

VkFormat findDepthFormat(VkPhysicalDevice device);

VkFormat findSupportedFormat(VkPhysicalDevice device, 
							const std::vector<VkFormat>& candidates, 
							VkImageTiling tiling, 
							VkFormatFeatureFlags features);

VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);

void endSingleTimeCommands(VkDevice device, 
						   VkCommandBuffer commandBuffer, 
					       VkCommandPool commandPool, 
						   VkQueue graphicsQueue);

bool hasStencilComponent(VkFormat format);
