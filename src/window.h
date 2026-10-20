#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vk-state.h"
#include "structs.h"

class Window {
public:

		Window();
		void initWindow();
	
		static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
		static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

private:
		VkState& state;

};

