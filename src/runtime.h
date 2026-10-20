#pragma once
#include "engine.h"
#include "vk-state.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Renderer {
	public:

		Renderer();

		void run();

	private:

		std::unique_ptr<Engine> engine;
		VkState& state;

		// renderer variables
		glm::vec3 rotat = { 0.5f, 0.0f, 0.5f };
		glm::vec3 pos = { 0.0f, 0.0f, 0.0f };
		float specularExponent = 2;
		glm::vec3 lightDir = {1,0,-0.5};

		// time stuff
		double lastTime;
		float deltaTime;
		float epochTime = 0;

		void mainLoop();

		void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		void showDebugPanel();
		void updateUniformBuffer(uint32_t currentImage);
		void drawFrame();
};

