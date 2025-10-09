#pragma once
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

bool checkValidationLayerSupport(std::vector<const char*>);

std::vector<const char*> getRequiredExtensions(bool enableValidationLayers);

void testFun();
