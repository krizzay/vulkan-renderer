#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>
#include <chrono>
#include <unordered_map>
#include <random>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb-master/stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

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

#include "engine-utils.h"
#include "compute.h"
#include "structs.h"
#include "engine.h"
#include "window.h"

/*
        AT SOME POINT SEPERATE THE IMAGE SAMPLER AND IMAGE MAYBE IDK RESEARSH IF YOU NEED AND OR CAN / SHOULD DO THAT!
*/

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const uint32_t PARTICLE_COUNT = 1024;
const int MAX_FRAMES_IN_FLIGHT = 3;

float speed = 50.5f;

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

class object {
public:
    std::string modelPath;
    std::string texturePath;

    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    bool rotate;

    uint32_t instanceCount;

    bool render = true;

    object(std::string _modelPath, std::string _texturePath, glm::vec3 _pos, uint32_t _scale) {
        modelPath = _modelPath;
        texturePath = _texturePath;
        position = _pos;
        rotate = false;
        instanceCount = 1;
        scale = glm::vec3(_scale);
    }

    object(std::string _modelPath, std::string _texturePath, glm::vec3 _pos, uint32_t _scale, uint32_t _instanceCount) {
        modelPath = _modelPath;
        texturePath = _texturePath;
        position = _pos;
        rotate = false;
        instanceCount = _instanceCount;
        scale = glm::vec3(_scale);
    }

    object(std::string _modelPath, std::string _texturePath, glm::vec3 _pos, glm::vec3 _rotate, uint32_t _instanceCount, uint32_t _scale) {
        modelPath = _modelPath;
        texturePath = _texturePath;
        position = _pos;
        rotate = true;
        rotation = _rotate;
        instanceCount = _instanceCount;
        scale = glm::vec3(_scale, _scale, _scale);
    }
};

class HelloTriangleApplication {
    
public:
    void run() {
		engine = std::unique_ptr<Engine>(new Engine(WIDTH, HEIGHT));
		Window::initWindow();
        initVulkan();
        initImgui();
        mainLoop();
        cleanup();
    }

private:

	std::unique_ptr<Engine> engine;

    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;
    VkDescriptorSetLayout globalDescriptorSetLayout;
    VkDescriptorSetLayout objectDescriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkPipelineLayout particlePipelineLayout;
    VkPipeline particleGraphicsPipeline;

	// render objects
    std::vector<object> objects = { 
        //object("models/aubrey.obj", "textures/aubrey.png", glm::vec3(0,0,0), glm::vec3(0.5, 0, 0.5), true, 1),
        //object("models/model.obj", "textures/texture.png", glm::vec3(0,80,0), glm::vec3(0, 0.5, 0.5), true, 5),
        object("../models/aubrey.obj", "../textures/aubrey.png", glm::vec3(0,1000,0), 1),
        object("../models/cubeoid.obj", "../textures/debug.png", glm::vec3(0, -500, 0), 4),
        object("../models/aubrey.obj", "../textures/aubrey.png", glm::vec3(0,500,0), 2),
        object("../models/aubrey.obj", "../textures/aubrey.png", glm::vec3(0,0,0), 4),
        object("../models/aubrey.obj", "../textures/aubrey.png", glm::vec3(0,-1000,0), 2),
        object("../models/aubrey.obj", "../textures/aubrey.png", glm::vec3(0,-1500,0), 2),
        object("../models/aubrey.obj", "../textures/aubrey.png", glm::vec3(0,0,5), 2, 100)
    };


    std::vector<uint32_t> mipLevels;
    std::vector<VkImage> textureImages;
    std::vector<VkDeviceMemory> textureImagesMemory;

    VkCommandPool commandPool;

    VkBuffer indexVertexBuffer;
    VkDeviceMemory indexVertexBufferMemory;

    VkBuffer instanceBuffer;
    VkDeviceMemory instanceBufferMemory;
    uint32_t instanceCount = 100;

    std::vector<VkBuffer> globalUniformBuffers;
    std::vector<VkDeviceMemory> globalUniformBuffersMemory;//
    std::vector<std::vector<VkBuffer>> uniformBuffers;
    std::vector<std::vector<VkDeviceMemory>> uniformBuffersMemory;

    std::vector<VkBuffer> shaderStorageBuffers;
	std::vector<VkDeviceMemory> shaderStorageBuffersMemory;


    VkDescriptorPool descriptorPool;
    std::vector<std::vector<VkDescriptorSet>> objectDescriptorSets;
    // objectDescriptorSets[object index][frame in flight index]
    std::vector<VkDescriptorSet> globalDescriptorSets;

    std::vector<VkCommandBuffer> commandBuffers;

    VkCommandBuffer setupBuffer;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    std::vector<VkImageView> textureImageViews;
    std::vector<VkSampler> textureSamplers;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<uint32_t> indicieOffsets;
    std::vector<uint32_t> vertexOffsets;
    uint32_t indiciesSize;

    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

	// renderer variables

    glm::vec3 rotat = { 0.5f, 0.0f, 0.5f };
    glm::vec3 pos = { 0.0f, 0.0f, 0.0f };
	float specularExponent = 2;
	glm::vec3 lightDir = {1,0,-0.5};

	// time stuff
    double lastTime;
    float deltaTime;
    float epochTime = 0;


	// engine
    void initVulkan() {
	
	    std::cout << "initialising vulkan! :>\n";


        engine->createInstance(); 
		engine->initCompute(PARTICLE_COUNT, MAX_FRAMES_IN_FLIGHT);
        engine->setupDebugMessenger(); 
        engine->createSurface(); 
        engine->pickPhysicalDevice(); 
        engine->createLogicalDevice(); 
        engine->createSwapChain(); 

        createImageViews(); // image view for swap chain // engine
        createRenderPass(); // engine
		engine->compute->CreateComputeDescriptorSetLayout(); 
        createDescriptorSetLayout(); // buffers
        createGraphicsPipeline(); // engine
        createParticleGraphicsPipeline(); // engine
        engine->compute->createComputePipeline(); 
        createCommandPool(); // engine

        createShaderStorageBuffers(); // buffers
        //setupCommandBuffer();//my garbage code // see if faster 
        createColorResources(); // buffers
        createDepthResources(); // buffers

        createFramebuffers(); //  engine
        createTextureImages(); // buffers
        createTextureImageViews(); // buffers
        createTextureSampler(); // buffers

        loadModel(); // buffers
        createIndexVertexBuffer(); // buffers
        createInstanceBuffer(); // buffers
        createUniformBuffers(); // buffers
        engine->compute->createComputeUniformBuffers(engine->physicalDevice);

        createDescriptorPool(); // buffers
        createDescriptorSets(); // buffers
        engine->compute->createComputeDescriptorSets(descriptorPool, shaderStorageBuffers); 

        createCommandBuffers(); // buffers
        engine->compute->createComputeCommandBuffers(commandPool); 
        createSyncObjects(); // engine
		engine->compute->createComputeSyncObjects();

        std::cout << "vulkan initialised :]\n";
    }
    
    void initImgui() {
	
	    std::cout << "initialising imgui! :>\n";

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(Window::window, true);

        ImGui_ImplVulkan_InitInfo info = {};
        info.Instance = engine->instance;
        info.Queue = engine->graphicsQueue;
        info.DescriptorPool = descriptorPool;
        info.RenderPass = renderPass;
        info.Subpass = 0;
        info.Device = engine->device;
        info.PhysicalDevice = engine->physicalDevice;
        info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
        info.ImageCount = MAX_FRAMES_IN_FLIGHT;
        info.MSAASamples = engine->msaaSamples;

        ImGui_ImplVulkan_Init(&info);     
        ImGui_ImplVulkan_CreateFontsTexture();

        std::cout << "imgui initialised :]\n";
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(Window::window)) {
            glfwPollEvents();
            drawFrame(); 
            
            double currentTime = glfwGetTime();
            deltaTime = currentTime - lastTime;
            epochTime += deltaTime;
            lastTime = currentTime;
        }

        vkDeviceWaitIdle(engine->device);
    }

    void cleanupSwapChain() {
        vkDestroyImageView(engine->device, colorImageView, nullptr);
        vkDestroyImage(engine->device, colorImage, nullptr);
        vkFreeMemory(engine->device, colorImageMemory, nullptr);

        vkDestroyImageView(engine->device, depthImageView, nullptr);
        vkDestroyImage(engine->device, depthImage, nullptr);
        vkFreeMemory(engine->device, depthImageMemory, nullptr);

        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(engine->device, framebuffer, nullptr);
        }

        vkDestroyPipeline(engine->device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(engine->device, pipelineLayout, nullptr);
        vkDestroyPipeline(engine->device, particleGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(engine->device, particlePipelineLayout, nullptr);
        vkDestroyRenderPass(engine->device, renderPass, nullptr);

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(engine->device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(engine->device, engine->swapChain, nullptr);
    }

    void cleanup() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        cleanupSwapChain();

        for (int i = 0; i < objects.size(); i++) {
            vkDestroySampler(engine->device, textureSamplers[i], nullptr);
            vkDestroyImageView(engine->device, textureImageViews[i], nullptr);

            vkDestroyImage(engine->device, textureImages[i], nullptr);
            vkFreeMemory(engine->device, textureImagesMemory[i], nullptr);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(engine->device, globalUniformBuffers[i], nullptr);
            vkFreeMemory(engine->device, globalUniformBuffersMemory[i], nullptr);
        }

        for (int i = 0; i < objects.size(); i++) {
            for (int j = 0; j < MAX_FRAMES_IN_FLIGHT; j++) {
                vkDestroyBuffer(engine->device, uniformBuffers[i][j], nullptr);
                vkFreeMemory(engine->device, uniformBuffersMemory[i][j], nullptr);
            }
        }

        vkDestroyDescriptorPool(engine->device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(engine->device, globalDescriptorSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(engine->device, objectDescriptorSetLayout, nullptr);

        std::cout << "deleting index vertex buffer" << std::endl;
        vkDestroyBuffer(engine->device, indexVertexBuffer, nullptr);
        vkFreeMemory(engine->device, indexVertexBufferMemory, nullptr);

        std::cout << "deleting instance buffer" << std::endl;
        vkDestroyBuffer(engine->device, instanceBuffer, nullptr);
        vkFreeMemory(engine->device, instanceBufferMemory, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            std::cout << "deleting shader storage buffer " << i << std::endl;
            vkDestroyBuffer(engine->device, shaderStorageBuffers[i], nullptr);
            vkFreeMemory(engine->device, shaderStorageBuffersMemory[i], nullptr);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(engine->device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(engine->device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(engine->device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(engine->device, commandPool, nullptr);

		std::cout << "we are about to destroy device" << std::endl;
        vkDestroyDevice(engine->device, nullptr);

        if (engine->enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(engine->instance, engine->debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(engine->instance, engine->surface, nullptr);
        vkDestroyInstance(engine->instance, nullptr);

        glfwDestroyWindow(Window::window);

        glfwTerminate();
		std::cout << "cleanup complete" << std::endl;
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(Window::window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(Window::window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(engine->device);

        cleanupSwapChain();

        engine->createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createParticleGraphicsPipeline();
        createColorResources();
        createDepthResources();
        createFramebuffers();
    }

    void createImageViews() {
        swapChainImageViews.resize(engine->swapChainImages.size());

        for (uint32_t i = 0; i < engine->swapChainImages.size(); i++) {
            swapChainImageViews[i] = createImageView(engine->swapChainImages[i], engine->swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = engine->swapChainImageFormat;
        colorAttachment.samples = engine->msaaSamples;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = engine->msaaSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = engine->swapChainImageFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        subpass.pResolveAttachments = &colorAttachmentResolveRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(engine->device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("../shaders/vert.spv");
        auto fragShaderCode = readFile("../shaders/frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, engine->device);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, engine->device);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescriptions = Vertex::getBindingDescriptions();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

/* 
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;
         */

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        //viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        //viewportState.pScissors = &scissor;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
        multisampling.minSampleShading = .2f; // min fraction for sample shading; closer to one is smoother
        multisampling.rasterizationSamples = engine->msaaSamples;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        std::array<VkDescriptorSetLayout, 2> layouts = { globalDescriptorSetLayout, objectDescriptorSetLayout };
        pipelineLayoutInfo.setLayoutCount = layouts.size();
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        /*          this is how using multiple ranges would look
        std::array<VkPushConstantRange, 2> pushConstantRanges{};

        pushConstantRanges[0].offset = 0;
        pushConstantRanges[0].size = sizeof(glm::mat4);
        pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        pushConstantRanges[1].offset = sizeof(glm::mat4);
        pushConstantRanges[1].size = sizeof(double);
        pushConstantRanges[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
        pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
        */

        VkPushConstantRange pushConsts;
        pushConsts.offset = 0;
        pushConsts.size = sizeof(pushConstans);
        pushConsts.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        pipelineLayoutInfo.pPushConstantRanges = &pushConsts;
        pipelineLayoutInfo.pushConstantRangeCount = 1;

        depthStencil.depthBoundsTestEnable = VK_FALSE;//keep fragment in dept range
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional

        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        if (vkCreatePipelineLayout(engine->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;


        if (vkCreateGraphicsPipelines(engine->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
    

        vkDestroyShaderModule(engine->device, fragShaderModule, nullptr);
        vkDestroyShaderModule(engine->device, vertShaderModule, nullptr);
    }
    
    void createParticleGraphicsPipeline() {
        auto vertShaderCode = readFile("../shaders/partVert.spv");
        auto fragShaderCode = readFile("../shaders/partFrag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, engine->device);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, engine->device);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescriptions = Particle::getBindingDescriptions();
        auto attributeDescriptions = Particle::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescriptions;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)engine->swapChainExtent.width;
        viewport.height = (float)engine->swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = engine->swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
        multisampling.minSampleShading = .2f; // min fraction for sample shading; closer to one is smoother
        multisampling.rasterizationSamples = engine->msaaSamples;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &objectDescriptorSetLayout;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

        VkPushConstantRange pushConsts;
        pushConsts.offset = 0;
        pushConsts.size = sizeof(pushConstans);
        pushConsts.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        pipelineLayoutInfo.pPushConstantRanges = &pushConsts;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        /*
        depthStencil.depthBoundsTestEnable = VK_FALSE;//keep fragment in dept range
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional

        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional
        */
        if (vkCreatePipelineLayout(engine->device, &pipelineLayoutInfo, nullptr, &particlePipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.layout = particlePipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;


        if (vkCreateGraphicsPipelines(engine->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &particleGraphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create particle graphics pipeline!");
        }


        vkDestroyShaderModule(engine->device, fragShaderModule, nullptr);
        vkDestroyShaderModule(engine->device, vertShaderModule, nullptr);
    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            std::array<VkImageView, 3> attachments = {
                colorImageView,
                depthImageView,
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = engine->swapChainExtent.width;
            framebufferInfo.height = engine->swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(engine->device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = engine->findQueueFamilies(engine->physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value();

        if (vkCreateCommandPool(engine->device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void createShaderStorageBuffers() {

        // Initialize particles
        std::default_random_engine rndEngine((unsigned)time(nullptr));
        std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

        // Initial particle positions on a circle
        std::vector<Particle> particles(PARTICLE_COUNT);
        for (auto& particle : particles) {
            float r = 0.25f * sqrt(rndDist(rndEngine));
            float theta = rndDist(rndEngine) * 2 * 3.14159265358979323846;
            float x = r * cos(theta) * HEIGHT / WIDTH;
            float y = r * sin(theta);
            particle.position = glm::vec2(x, y);
            particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00005f;
            particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);

        }

        VkDeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, engine->device, engine->physicalDevice);

        void* data;
        vkMapMemory(engine->device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, particles.data(), (size_t)bufferSize);
        vkUnmapMemory(engine->device, stagingBufferMemory);

        shaderStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        shaderStorageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shaderStorageBuffers[i], shaderStorageBuffersMemory[i], engine->device, engine->physicalDevice);
            // Copy data from the staging buffer (host) to the shader storage buffer (GPU)
            copyBuffer(stagingBuffer, shaderStorageBuffers[i], bufferSize);
        }

        vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
        vkFreeMemory(engine->device, stagingBufferMemory, nullptr);
    }

    void createIndexVertexBuffer() {
        //create vertex transfer source buffer

        VkDeviceSize vertexSourceSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer vertexStagingBuffer;
        VkDeviceMemory vertexStagingBufferMemory;
        createBuffer(vertexSourceSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexStagingBuffer, vertexStagingBufferMemory, engine->device,	engine->physicalDevice);

        void* data;
        vkMapMemory(engine->device, vertexStagingBufferMemory, 0, vertexSourceSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)vertexSourceSize);
        vkUnmapMemory(engine->device, vertexStagingBufferMemory);

        //create index transfer source buffer
        VkDeviceSize indexSourceSize = sizeof(indices[0]) * indices.size();
        indiciesSize = indexSourceSize;

        VkBuffer indexStagingBuffer;
        VkDeviceMemory indexStagingBufferMemory;
        createBuffer(indexSourceSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indexStagingBuffer, indexStagingBufferMemory, engine->device, engine->physicalDevice);

        vkMapMemory(engine->device, indexStagingBufferMemory, 0, indexSourceSize, 0, &data);
        memcpy(data, indices.data(), (size_t)indexSourceSize);
        vkUnmapMemory(engine->device, indexStagingBufferMemory);

        //copy to index vertex buffer
        createBuffer(indexSourceSize + vertexSourceSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | 
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexVertexBuffer, indexVertexBufferMemory, engine->device, engine->physicalDevice);

        copyBuffer(indexStagingBuffer, indexVertexBuffer, indexSourceSize);

        copyBuffer(vertexStagingBuffer, 0 , indexVertexBuffer, indexSourceSize, vertexSourceSize);

        vkDestroyBuffer(engine->device, vertexStagingBuffer, nullptr);
        vkFreeMemory(engine->device, vertexStagingBufferMemory, nullptr);
        vkDestroyBuffer(engine->device, indexStagingBuffer, nullptr);
        vkFreeMemory(engine->device, indexStagingBufferMemory, nullptr);

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

    void createInstanceBuffer() {
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
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, engine->device, engine->physicalDevice);

        void* data;
        vkMapMemory(engine->device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, instanceData.data(), (size_t)bufferSize);
        vkUnmapMemory(engine->device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, instanceBuffer, instanceBufferMemory, engine->device, engine->physicalDevice);

        copyBuffer(stagingBuffer, instanceBuffer, bufferSize);

        vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
        vkFreeMemory(engine->device, stagingBufferMemory, nullptr);

        
    }
    
    //something in createtextureimage is giving an error :(    what is it????? since when?? huh?
    void createTextureImages() {
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
            createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, engine->device, engine->physicalDevice);

            void* data;
            vkMapMemory(engine->device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
            vkUnmapMemory(engine->device, stagingBufferMemory);

            stbi_image_free(pixels);

            createImage(texWidth, texHeight, mipLevels[i], VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImages[i], textureImagesMemory[i]);

            transitionImageLayout(textureImages[i], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels[i]);
            copyBufferToImage(stagingBuffer, textureImages[i], static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));


            vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
            vkFreeMemory(engine->device, stagingBufferMemory, nullptr);

            generateMipmaps(textureImages[i], VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels[i]);

        }
    }

    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples , VkFormat format, 
        VkImageTiling tiling, VkImageUsageFlags usage,VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
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

        if (vkCreateImage(engine->device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(engine->device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, engine->physicalDevice);

        if (vkAllocateMemory(engine->device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(engine->device, image, imageMemory, 0);
    }
    
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
        // Check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(engine->physicalDevice, imageFormat, &formatProperties);
        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }
        //implement a function that searches common texture image formats for one that does support linear blitting

        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

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
        

        endSingleTimeCommands(commandBuffer);
    }
    
    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(engine->device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(engine->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(engine->graphicsQueue);

        vkFreeCommandBuffers(engine->device, commandPool, 1, &commandBuffer);
    }

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

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

        endSingleTimeCommands(commandBuffer);

        //flushSetupCommands();
    }

    // garbage code ahead
    void setupCommandBuffer()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        vkAllocateCommandBuffers(engine->device, &allocInfo, &setupBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        vkBeginCommandBuffer(setupBuffer, &beginInfo);
    }

    void flushSetupCommands()
    {
        vkEndCommandBuffer(setupBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &setupBuffer;

        vkQueueSubmit(engine->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(engine->graphicsQueue); 

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        vkBeginCommandBuffer(setupBuffer, &beginInfo);
    }
    //end of garbage code
    
    void createDepthResources()
    {
        VkFormat depthFormat = findDepthFormat();

        createImage(engine->swapChainExtent.width, engine->swapChainExtent.height, 1, engine->msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);

        depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }
    
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(engine->physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    VkFormat findDepthFormat() {
        return findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

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

        endSingleTimeCommands(commandBuffer);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);

        //A fence would allow you to schedule multiple transfers simultaneously and wait for all of them complete, 
        //instead of executing one at a time. That may give the driver more opportunities to optimize.

    }

    void copyBuffer(VkBuffer srcBuffer, VkDeviceSize srcOffset, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = srcOffset;
        copyRegion.dstOffset = dstOffset;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);

        //A fence would allow you to schedule multiple transfers simultaneously and wait for all of them complete, 
        //instead of executing one at a time. That may give the driver more opportunities to optimize.

    }

    void createDescriptorSetLayout() {
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

        if (vkCreateDescriptorSetLayout(engine->device, &layoutInfo, nullptr, &globalDescriptorSetLayout) != VK_SUCCESS) {
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

        if (vkCreateDescriptorSetLayout(engine->device, &layoutInfo, nullptr, &objectDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void createUniformBuffers() {
        //create global uniform buffer
        VkDeviceSize bufferSize = sizeof(GlobalUniformBufferObject);

        globalUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        globalUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, globalUniformBuffers[i], globalUniformBuffersMemory[i], engine->device, engine->physicalDevice);
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
                createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i][j], uniformBuffersMemory[i][j], engine->device, engine->physicalDevice);
            }
        }
    }

    void createTextureImageViews() {
        textureImageViews.resize(objects.size());

        for (int i = 0; i < objects.size(); i++) {
            textureImageViews[i] = createImageView(textureImages[i], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels[i]);
        }
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(engine->device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }

    void createTextureSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;//VK_FILTER_NEAREST
        samplerInfo.minFilter = VK_FILTER_LINEAR;

        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(engine->physicalDevice, &properties);

        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        textureSamplers.resize(objects.size());
        for (int i = 0; i < objects.size(); i++)
        {
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.minLod = 0.0f; // Optional
            samplerInfo.maxLod = static_cast<float>(mipLevels[i]);
            samplerInfo.mipLodBias = 0.0f; // Optional

            if (vkCreateSampler(engine->device, &samplerInfo, nullptr, &textureSamplers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture sampler!");
            }

        }
    }

    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        if (vkAllocateCommandBuffers(engine->device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; //this command buffer is recorded once and tehn submitted, not sure why this is important so ya

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = engine->swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };
        //Note that the order of clearValues should be identical to the order of your attachments
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float)engine->swapChainExtent.width;
            viewport.height = (float)engine->swapChainExtent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = engine->swapChainExtent;

            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            VkBuffer vertexBuffers[] = { indexVertexBuffer };

            VkDeviceSize offsets[] = { indiciesSize };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, indexVertexBuffer, 0, VK_INDEX_TYPE_UINT32);

            VkBuffer isntanceBuffers[] = { instanceBuffer };
            VkDeviceSize instanceOffsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 1, 1, isntanceBuffers, instanceOffsets);

            //camera calculations
            engine->moveAmount.y = fmaxf(fminf(engine->moveAmount.y, 3.14), 0.0001f);
            float a = sin(engine->moveAmount.y);
            glm::vec3 dir = glm::vec3(a * cos(engine->moveAmount.x), a * sin(engine->moveAmount.x), -cos(engine->moveAmount.y));
            glm::vec3 dirRec = glm::vec3(dir.y, -dir.x, 0);// vec at a right angle
            if (engine->resetPos) {
                pos = { 0.0f, 0.0f, 0.0f };
                engine->resetPos = false;
            }
            pos += (dir * speed * engine->movement.x) + (dirRec * speed * engine->movement.y);
            pos.z += speed * engine->movement.z;

            pushConstans constants;
            constants.view = glm::lookAt(pos, pos + dir, glm::vec3(0.0f, 0.0f, 1.0f));
            constants.deltaTime = deltaTime;
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstans), &constants);
            //push moveAmount, movement and speed instead of view and then do camera calculations in the shader or sumtin

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &globalDescriptorSets[currentFrame], 0, nullptr);

            //draw all the objects
            
            for (int i = 0; i < objects.size(); i++) {
                
                if(!objects[i].render) {
                    continue;
                }
                /* TODO:
                    if the model isnt scaled uniformly then the normals
                    become invalid and need to be corrected with
                    Normal = mat3(transpose(inverse(model))) * aNormal;  
                    apparently inverse is expensive so its prolly best
                    to do on the cpu and send the result to the gpu
                 */
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &objectDescriptorSets[i][currentFrame], 0, nullptr);

                int instancesToDraw = objects[i].instanceCount; 
                int firstInstace = 1;

                if (instancesToDraw == 1) {
                    firstInstace = 0;
                }

                //consider indirect draw
                vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indicieOffsets[i+1] - indicieOffsets[i]),
                    instancesToDraw, static_cast<uint32_t>(indicieOffsets[i]),  0, firstInstace);
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            showDebugPanel();

            ImGui::Render(); 
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer, VK_NULL_HANDLE); 

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, particleGraphicsPipeline);

            VkDeviceSize particleOffsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &shaderStorageBuffers[currentFrame], particleOffsets);

            vkCmdDraw(commandBuffer, PARTICLE_COUNT, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void showDebugPanel (){
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
            for(int i = 0; i < objects.size(); i++){

                std::string title = "object ";
                title += std::to_string(i);
                ImGui::PushID(i);

                ImGui::Indent();
                if(ImGui::CollapsingHeader(title.c_str())){
                    ImGui::InputFloat3("position", glm::value_ptr(objects[i].position), "%.2f");
                    ImGui::InputFloat3("rotation", glm::value_ptr(objects[i].rotation), "%.2f");
                    ImGui::Checkbox("rotate?", &objects[i].rotate);
                    ImGui::InputFloat3("scale", glm::value_ptr(objects[i].scale), "%.2f");
                    ImGui::Checkbox("Render?", &objects[i].render);
                }
                ImGui::Unindent();
                ImGui::PopID();
            }
        }

        ImGui::End();
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(engine->device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(engine->device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(engine->device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    void loadModel() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        indicieOffsets.push_back(0);
        vertexOffsets.push_back(0);

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (int i = 0; i < objects.size(); i++) {

            if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objects[i].modelPath.c_str())) {
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

    void updateUniformBuffer(uint32_t currentImage) {
        //update global ubo
        GlobalUniformBufferObject gubo{};

        gubo.proj = glm::perspective(glm::radians(45.0f), engine->swapChainExtent.width / (float)engine->swapChainExtent.height, 0.1f, 15000.0f);

        gubo.proj[1][1] *= -1;//GLM was made for opengl where the y coordinate of the clip coordinates is inverted ; this solves that

        gubo.ambientLightCol = {1,1,1};
        gubo.ambientStrength = 0.05f;
        gubo.lightDir = glm::normalize(lightDir);
        gubo.viewPos = pos; // direction the camera is lookin
        //gubo.viewPos = glm::vec3(0,0,1);
        gubo.specExponent = specularExponent;

        
        //std::cout << "norm pos - " << glm::normalize(pos) << std::endl;

        void* data;
        vkMapMemory(engine->device, globalUniformBuffersMemory[currentImage], 0, sizeof(gubo), 0, &data);
        memcpy(data, &gubo, sizeof(gubo));
        vkUnmapMemory(engine->device, globalUniformBuffersMemory[currentImage]);

        //update per object ubos

        for (size_t i = 0; i < objects.size(); i++) {
            UniformBufferObject ubo{};

            ubo.model = glm::mat4(1.0f);

            ubo.model = glm::translate(ubo.model, objects[i].position);
            if (objects[i].rotate == true) {
                ubo.model = glm::rotate(ubo.model, epochTime * glm::radians(25.0f), objects[i].rotation);
            }
            ubo.model = glm::scale(ubo.model, objects[i].scale);

            void* data;
            vkMapMemory(engine->device, uniformBuffersMemory[i][currentImage], 0, sizeof(ubo), 0, &data);
            memcpy(data, &ubo, sizeof(ubo));
            vkUnmapMemory(engine->device, uniformBuffersMemory[i][currentImage]);
        }


    }

    void createColorResources() {
        VkFormat colorFormat = engine->swapChainImageFormat;

        createImage(engine->swapChainExtent.width, engine->swapChainExtent.height, 1, engine->msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
        colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    void createDescriptorPool() { 

        std::array<VkDescriptorPoolSize, 3> poolSizes{};// defines shit for different descriptor types

        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>((MAX_FRAMES_IN_FLIGHT * objects.size()) + MAX_FRAMES_IN_FLIGHT *2);
        //one ubo per object per frame in flight
        //compute -> ubo per frame in flight
        //global ubo per frame in flight
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * objects.size() + MAX_FRAMES_IN_FLIGHT);
        //one combined image sampler per object per framne in flight
        //another one per frame in flight for imgui

        poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size()); // num of different descriptors
        poolInfo.pPoolSizes = poolSizes.data();                                                                 
        poolInfo.maxSets = static_cast<uint32_t>((MAX_FRAMES_IN_FLIGHT * objects.size()) + MAX_FRAMES_IN_FLIGHT *3); // num of descriptor sets
        //one set per object per frame in flight
        //compute -> set per frame in flight
        //global set per frame in flight
        //imgui set per frame in flight
        if (vkCreateDescriptorPool(engine->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createDescriptorSets() {
        //create MAX_FRAMES_IN_FLIGHT sets per object
        objectDescriptorSets.resize(objects.size());

        for (int j = 0; j < objects.size(); j++)
        {
            std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, objectDescriptorSetLayout);
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = descriptorPool;
            allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
            allocInfo.pSetLayouts = layouts.data();

            objectDescriptorSets[j].resize(MAX_FRAMES_IN_FLIGHT);
            if (vkAllocateDescriptorSets(engine->device, &allocInfo, objectDescriptorSets[j].data()) != VK_SUCCESS) {
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

                vkUpdateDescriptorSets(engine->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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
        if (vkAllocateDescriptorSets(engine->device, &allocInfo, globalDescriptorSets.data()) != VK_SUCCESS) {
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

            vkUpdateDescriptorSets(engine->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void drawFrame() {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // Compute submission        
        vkWaitForFences(engine->device, 1, &engine->compute->m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        engine->compute->updateComputeUniformBuffer(currentFrame);

        vkResetFences(engine->device, 1, &engine->compute->m_inFlightFences[currentFrame]);

        engine->compute->resetCommandBuffer(currentFrame, /*VkCommandBufferResetFlagBits*/ 0);
        engine->compute->recordComputeCommandBuffer(epochTime, currentFrame);

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &engine->compute->m_commandBuffers[currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &engine->compute->m_finishedSemaphores[currentFrame];
		
		engine->compute->submitCommandBuffer(currentFrame, submitInfo);

        // Graphics submission
        vkWaitForFences(engine->device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        updateUniformBuffer(currentFrame);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(engine->device, engine->swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        vkResetFences(engine->device, 1, &inFlightFences[currentFrame]);

        //vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0); //not needed with the one time command buffer bit set i think maybe

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame], engine->compute->m_finishedSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT };
        submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.waitSemaphoreCount = 2;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(engine->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { engine->swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(engine->presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || Window::framebufferResized) {
			Window::framebufferResized = false;
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

	float lerp(float v0, float v1, float t) {
 		 return v0 + t * (v1 - v0);
	}

	
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
