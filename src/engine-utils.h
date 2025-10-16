#pragma once
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include <string>

bool checkValidationLayerSupport(std::vector<const char*>);

std::vector<const char*> getRequiredExtensions(bool enableValidationLayers);

std::vector<char> readFile(const std::string& filename);

VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
					VkBuffer& buffer, VkDeviceMemory& bufferMemoryi, VkDevice device, VkPhysicalDevice physicalDevice);

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice); 
