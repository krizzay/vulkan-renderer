#include "runtime.h"
#include "engine-utils.h"
#include "compute.h"
#include "structs.h"
#include "engine.h"
#include "window.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui-master/imconfig.h"
#include "imgui-master/imgui_tables.cpp"
#include "imgui-master/imgui_internal.h"
#include "imgui-master/imgui.cpp"
#include "imgui-master/imgui_draw.cpp"
#include "imgui-master/imgui_widgets.cpp"
#include "imgui-master/imgui_demo.cpp"
#include "imgui-master/imgui_impl_glfw.cpp"
#include "imgui-master/imgui_impl_vulkan.cpp"

/*
 * TODO: look at seperating the image sampler from the images
*/

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

constexpr uint32_t PARTICLE_COUNT = 1024;
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

float speed = 50.5f;

Renderer::Renderer()
	: state{VkState::getInstance()}
{ }

void Renderer::run() {
	engine = std::make_unique<Engine>(WIDTH, HEIGHT, MAX_FRAMES_IN_FLIGHT, PARTICLE_COUNT);
	engine->initVulkan();
	engine->initImgui();
	mainLoop();
	engine.reset(); // call destructor
}

void Renderer::mainLoop() {
	while (!glfwWindowShouldClose(state.window)) {
		glfwPollEvents();
		drawFrame(); 
		
		double currentTime = glfwGetTime();
		deltaTime = currentTime - lastTime;
		epochTime += deltaTime;
		lastTime = currentTime;
	}

	vkDeviceWaitIdle(state.device);
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; //this command buffer is recorded once and tehn submitted, not sure why this is important so ya

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = state.renderPass;
	renderPassInfo.framebuffer = state.swapchainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = state.swapchainExtent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };
	//Note that the order of clearValues should be identical to the order of your attachments
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.graphicsPipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)state.swapchainExtent.width;
		viewport.height = (float)state.swapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = state.swapchainExtent;

		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = { engine->resources->indexVertexBuffer };

		VkDeviceSize offsets[] = { engine->resources->indiciesSize };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, engine->resources->indexVertexBuffer, 0, VK_INDEX_TYPE_UINT32);

		VkBuffer isntanceBuffers[] = { engine->resources->instanceBuffer };
		VkDeviceSize instanceOffsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 1, 1, isntanceBuffers, instanceOffsets);

		//camera calculations
		state.moveAmount.y = fmaxf(fminf(state.moveAmount.y, 3.14), 0.0001f);
		float a = sin(state.moveAmount.y);
		glm::vec3 dir = glm::vec3(a * cos(state.moveAmount.x), a * sin(state.moveAmount.x), -cos(state.moveAmount.y));
		glm::vec3 dirRec = glm::vec3(dir.y, -dir.x, 0);// vec at a right angle
		if (state.resetPos) {
			pos = { 0.0f, 0.0f, 0.0f };
			state.resetPos = false;
		}
		pos += (dir * speed * state.movement.x) + (dirRec * speed * state.movement.y);
		pos.z += speed * state.movement.z;

		pushConstans constants;
		constants.view = glm::lookAt(pos, pos + dir, glm::vec3(0.0f, 0.0f, 1.0f));
		constants.deltaTime = deltaTime;
		vkCmdPushConstants(commandBuffer, 
							state.pipelineLayout, 
							VK_SHADER_STAGE_VERTEX_BIT, 
							0, 
							sizeof(pushConstans), 
							&constants);
		//push moveAmount, movement and speed instead of view and then do camera calculations in the shader or sumtin

		vkCmdBindDescriptorSets(commandBuffer, 
								VK_PIPELINE_BIND_POINT_GRAPHICS, 
								state.pipelineLayout, 
								0, 
								1, 
								&engine->resources->globalDescriptorSets[state.currentFrame], 
								0, 
								nullptr);

		//draw all the objects
		
		for (size_t i = 0; i < engine->resources->objects.size(); i++) {
			
			if(!engine->resources->objects[i].render) {
				continue;
			}
			/* TODO:
				if the model isnt scaled uniformly then the normals
				become invalid and need to be corrected with
				Normal = mat3(transpose(inverse(model))) * aNormal;  
				apparently inverse is expensive so its prolly best
				to do on the cpu and send the result to the gpu
			 */
			vkCmdBindDescriptorSets(commandBuffer, 
									VK_PIPELINE_BIND_POINT_GRAPHICS, 
									state.pipelineLayout, 
									1, 
									1, 
									&engine->resources->objectDescriptorSets[i][state.currentFrame], 
									0, 
									nullptr);

			int instancesToDraw = engine->resources->objects[i].instanceCount; 
			int firstInstace = 1;

			if (instancesToDraw == 1) {
				firstInstace = 0;
			}

			//consider indirect draw
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(engine->resources->indicieOffsets[i+1] 
															- engine->resources->indicieOffsets[i]),
				instancesToDraw, static_cast<uint32_t>(engine->resources->indicieOffsets[i]),  0, firstInstace);
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		showDebugPanel();

		ImGui::Render(); 
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer, VK_NULL_HANDLE); 

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.particleGraphicsPipeline);

		VkDeviceSize particleOffsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 
								0, 
								1, 
								&engine->resources->shaderStorageBuffers[state.currentFrame], 
								particleOffsets);

		vkCmdDraw(commandBuffer, PARTICLE_COUNT, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void Renderer::showDebugPanel() {
	ImGui::ShowDemoWindow();

	ImGui::Begin(" cool debug window yippie !");

	ImGui::Text("Redering stats");
	ImGui::Text("delta time - %8.8f", deltaTime);
	ImGui::Text("frame rate - %5.3f", 1 / deltaTime);
	
	ImGui::Spacing();
	ImGui::Text("Camera");
	ImGui::SliderFloat("movement speed", &speed, 0.0f, 69.69f);   
	ImGui::InputFloat3("camera position", glm::value_ptr(pos), "%.5f");
	ImGui::Text("Scene");
	ImGui::SliderFloat("specular exponent", &specularExponent, 0.0f, 128.69f);
	ImGui::InputFloat3("light direction", glm::value_ptr(lightDir), "%.2f");

	 
	if(ImGui::CollapsingHeader("objects")) {
		int32_t id = 0; //TODO: change this, give each object an id or something
		for(auto object = engine->resources->objects.begin();
			   object != engine->resources->objects.end(); object++){

			std::string title = "object ";
			title += std::to_string(id);
			ImGui::PushID(id);
			id++;

			ImGui::Indent();
			if(ImGui::CollapsingHeader(title.c_str())){
				ImGui::InputFloat3("position", glm::value_ptr(object->position), "%.2f");
				ImGui::InputFloat3("rotation", glm::value_ptr(object->rotation), "%.2f");
				ImGui::Checkbox("rotate?", &object->rotate);
				ImGui::InputFloat3("scale", glm::value_ptr(object->scale), "%.2f");
				ImGui::Checkbox("Render?", &object->render);
			}
			ImGui::Unindent();
			ImGui::PopID();
		}
	}

	ImGui::End();
}

void Renderer::updateUniformBuffer(uint32_t currentImage) {
	//update global ubo
	GlobalUniformBufferObject gubo{};

	gubo.proj = glm::perspective(glm::radians(45.0f), state.swapchainExtent.width / (float)state.swapchainExtent.height, 0.1f, 15000.0f);

	gubo.proj[1][1] *= -1;//GLM was made for opengl where the y coordinate of the clip coordinates is inverted ; this solves that

	gubo.ambientLightCol = {1,1,1};
	gubo.ambientStrength = 0.05f;
	gubo.lightDir = glm::normalize(lightDir);
	gubo.viewPos = pos; // direction the camera is lookin
	//gubo.viewPos = glm::vec3(0,0,1);
	gubo.specExponent = specularExponent;

	
	//std::cout << "norm pos - " << glm::normalize(pos) << std::endl;

	void* data;
	vkMapMemory(state.device, engine->resources->globalUniformBuffersMemory[currentImage], 0, sizeof(gubo), 0, &data);
	memcpy(data, &gubo, sizeof(gubo));
	vkUnmapMemory(state.device, engine->resources->globalUniformBuffersMemory[currentImage]);

	//update per object ubos

	int32_t idx = 0;
	for(auto object = engine->resources->objects.begin(); 
			object != engine->resources->objects.end();
			object++)
	{
		UniformBufferObject ubo{};

		ubo.model = glm::mat4(1.0f);

		ubo.model = glm::translate(ubo.model, object->position);
		if (object->rotate == true) {
			ubo.model = glm::rotate(ubo.model, epochTime * glm::radians(25.0f), object->rotation);
		}
		ubo.model = glm::scale(ubo.model, object->scale);

		void* data;
		vkMapMemory(state.device, engine->resources->uniformBuffersMemory[idx][currentImage], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(state.device, engine->resources->uniformBuffersMemory[idx][currentImage]);

		idx++;
	}

}

void Renderer::drawFrame() {
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// Compute submission        
	vkWaitForFences(state.device, 1, &engine->compute->m_inFlightFences[state.currentFrame], VK_TRUE, UINT64_MAX);

	engine->compute->updateComputeUniformBuffer(state.currentFrame);

	vkResetFences(state.device, 1, &engine->compute->m_inFlightFences[state.currentFrame]);

	engine->compute->resetCommandBuffer(state.currentFrame, /*VkCommandBufferResetFlagBits*/ 0);
	engine->compute->recordComputeCommandBuffer(epochTime, state.currentFrame);

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &engine->compute->m_commandBuffers[state.currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &engine->compute->m_finishedSemaphores[state.currentFrame];
	
	engine->compute->submitCommandBuffer(state.currentFrame, submitInfo);

	// Graphics submission
	vkWaitForFences(state.device, 1, &state.inFlightFences[state.currentFrame], VK_TRUE, UINT64_MAX);

	updateUniformBuffer(state.currentFrame);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(state.device, 
											state.swapchain, 
											UINT64_MAX, 
											state.imageAvailableSemaphores[state.currentFrame], 
											VK_NULL_HANDLE, 
											&imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		engine->recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	recordCommandBuffer(state.commandBuffers[state.currentFrame], imageIndex);

	vkResetFences(state.device, 1, &state.inFlightFences[state.currentFrame]);

	//vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0); //not needed with the one time command buffer bit set i think maybe

	VkSemaphore waitSemaphores[] = { state.imageAvailableSemaphores[state.currentFrame], 
							engine->compute->m_finishedSemaphores[state.currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
										  VK_PIPELINE_STAGE_VERTEX_INPUT_BIT };
	submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.waitSemaphoreCount = 2;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &state.commandBuffers[state.currentFrame];

	VkSemaphore signalSemaphores[] = { state.renderFinishedSemaphores[state.currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(state.graphicsQueue, 1, &submitInfo, state.inFlightFences[state.currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapchains[] = { state.swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(state.presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || state.framebufferResized) {
		state.framebufferResized = false;
		engine->recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	state.currentFrame = (state.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
