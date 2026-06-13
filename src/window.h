#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "structs.h"

class Window{
public:

    	static GLFWwindow* window;

		static uint32_t WIDTH;
		static uint32_t HEIGHT;

		static bool cursorFree;
		static bool framebufferResized;

		// camera things
		static glm::vec3 *movement;
		static glm::vec2 *oldPos;
		static glm::vec2 *moveAmount; 
		static bool *resetPos;

		static void initWindow();
	
		static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
		static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

private:

};
