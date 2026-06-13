#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "window.h"
#include "structs.h"

GLFWwindow* Window::window = nullptr;
uint32_t Window::WIDTH = 800;
uint32_t Window::HEIGHT = 600;

bool Window::cursorFree = false;
bool Window::framebufferResized = false;

glm::vec3* Window::movement = nullptr;
glm::vec2* Window::oldPos = nullptr;
glm::vec2* Window::moveAmount = nullptr; 
bool* Window::resetPos = nullptr;

void Window::key_callback(GLFWwindow* windowPtr, int key, int scancode, int action, int mods) {
    switch (key)
    {
    case(GLFW_KEY_W):
        if (action == GLFW_PRESS)
            movement->x++;
        else if (action == GLFW_RELEASE)
            movement->x--;
        break;

    case(GLFW_KEY_S):
        if (action == GLFW_PRESS)
            movement->x--;
        else if (action == GLFW_RELEASE)
            movement->x++;
        break;

    case(GLFW_KEY_A):
        if (action == GLFW_PRESS)
            movement->y--;
        else if (action == GLFW_RELEASE)
            movement->y++;
        break;

    case(GLFW_KEY_D):
        if (action == GLFW_PRESS)
            movement->y++;
        else if (action == GLFW_RELEASE)
            movement->y--;
        break;

    case(GLFW_KEY_SPACE):
        if (action == GLFW_PRESS)
            movement->z++;
        else if (action == GLFW_RELEASE)
            movement->z--;
        break;

    case(GLFW_KEY_LEFT_SHIFT):
        if (action == GLFW_PRESS)
            movement->z--;
        else if (action == GLFW_RELEASE)
            movement->z++;
        break;

    case(GLFW_KEY_R):
        *resetPos = true;
        break;

    case(GLFW_KEY_ESCAPE):
        std::cout << "\nClosing window! :]\n\n";
        glfwSetWindowShouldClose(windowPtr, 1);
        break;

    case(GLFW_KEY_Q):
        
        if(action == GLFW_PRESS){
            if(cursorFree){
                glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

            }else{
                glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            cursorFree = !cursorFree;
        }
        break; 
    
    default:
        break;
    }
}

void Window::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if(!cursorFree){  return; }

    moveAmount->x += (oldPos->x - xpos) * 0.005f;
    moveAmount->y += (oldPos->y - ypos) * 0.005f;
    
    oldPos->x = xpos;
    oldPos->y = ypos;

}

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		framebufferResized = true;
}

void Window::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan :3", nullptr, nullptr);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	if (glfwRawMouseMotionSupported()){
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}

}

