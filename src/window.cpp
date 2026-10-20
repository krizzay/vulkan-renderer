#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "window.h"
#include "vk-state.h"
#include "structs.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

Window::Window()
	: state{VkState::getInstance()}
{ }

void Window::key_callback(GLFWwindow* windowPtr, int key, int scancode, int action, int mods) {
	VkState& state = VkState::getInstance();

    switch (key)
    {
    case(GLFW_KEY_W):
        if (action == GLFW_PRESS)
            state.movement.x++;
        else if (action == GLFW_RELEASE)
            state.movement.x--;
        break;

    case(GLFW_KEY_S):
        if (action == GLFW_PRESS)
            state.movement.x--;
        else if (action == GLFW_RELEASE)
            state.movement.x++;
        break;

    case(GLFW_KEY_A):
        if (action == GLFW_PRESS)
            state.movement.y--;
        else if (action == GLFW_RELEASE)
            state.movement.y++;
        break;

    case(GLFW_KEY_D):
        if (action == GLFW_PRESS)
            state.movement.y++;
        else if (action == GLFW_RELEASE)
            state.movement.y--;
        break;

    case(GLFW_KEY_SPACE):
        if (action == GLFW_PRESS)
            state.movement.z++;
        else if (action == GLFW_RELEASE)
            state.movement.z--;
        break;

    case(GLFW_KEY_LEFT_SHIFT):
        if (action == GLFW_PRESS)
            state.movement.z--;
        else if (action == GLFW_RELEASE)
            state.movement.z++;
        break;

    case(GLFW_KEY_R):
        state.resetPos = true;
        break;

    case(GLFW_KEY_ESCAPE):
        std::cout << "\nClosing window! :]\n\n";
        glfwSetWindowShouldClose(windowPtr, 1);
        break;

    case(GLFW_KEY_Q):
        
        if(action == GLFW_PRESS){
            if(state.cursorFree){
                glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

            }else{
                glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            state.cursorFree = !state.cursorFree;
        }
        break; 
    
    default:
        break;
    }
}

void Window::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	VkState& state = VkState::getInstance();

    if(!state.cursorFree){ return; }

    state.moveAmount.x += (state.oldPos.x - xpos) * 0.005f;
    state.moveAmount.y += (state.oldPos.y - ypos) * 0.005f;
    
    state.oldPos.x = xpos;
    state.oldPos.y = ypos;

}

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	VkState& state = VkState::getInstance();

	state.framebufferResized = true;
}

void Window::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	state.window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan :3", nullptr, nullptr);
	glfwSetFramebufferSizeCallback(state.window, framebufferResizeCallback);

	glfwSetKeyCallback(state.window, key_callback);
	glfwSetCursorPosCallback(state.window, cursor_position_callback);
	glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	if (glfwRawMouseMotionSupported()){
		glfwSetInputMode(state.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}

}

