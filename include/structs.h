#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct pushConstans {
    glm::mat4 view;  //64 / 128 bytes available
    float deltaTime;      //72 / 128 bytes
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsAndComputeFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 normal;

    static std::array<VkVertexInputBindingDescription, 2> getBindingDescriptions() {
        std::array<VkVertexInputBindingDescription, 2> bindingDescriptions{};
        //vertex data
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        //instance data
        bindingDescriptions[1].binding = 1;
        bindingDescriptions[1].stride = sizeof(glm::vec3)*2;
        bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

        return bindingDescriptions;
    }

    static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions{};

        //vertex pos, changes per vertex
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        //vertex colour, changes per vertex
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        //vertex texture coordinate, changes per vertex
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        //vertex position offset, changes per instance
        attributeDescriptions[3].binding = 1;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = 0;

        //vertex colour offset, changes per instance
        attributeDescriptions[4].binding = 1;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[4].offset = sizeof(glm::vec3);

        //vertex normal, changes per vertex
        attributeDescriptions[5].binding = 0;
        attributeDescriptions[5].location = 5;
        attributeDescriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[5].offset = offsetof(Vertex, normal);

        return attributeDescriptions;
    }
    
    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

struct Particle {
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec4 color;

    static VkVertexInputBindingDescription getBindingDescriptions() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Particle);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Particle, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Particle, color);

        return attributeDescriptions;
    }
};

/*
 Vulkan expects the data in your structure to be aligned in memory in a specific way, for example:

    Scalars have to be aligned by N (= 4 bytes given 32 bit floats).
    A vec2 must be aligned by 2N (= 8 bytes)
    A vec3 or vec4 must be aligned by 4N (= 16 bytes)
    A nested structure must be aligned by the base alignment of its members rounded up to a multiple of 16.
    A mat4 matrix must have the same alignment as a vec4.

You can find the full list of alignment requirements in the specification.

Our original shader with just three mat4 fields already met the alignment requirements. As each mat4 is 4 x 4 x 4 = 
64 bytes in size, model has an offset of 0, view has an offset of 64 and proj has an offset of 128. All of these are multiples of 16 and that's why it worked fine.

The new structure starts with a vec2 which is only 8 bytes in size and therefore throws off all of the offsets. 
Now model has an offset of 8, view an offset of 72 and proj an offset of 136, none of which are multiples of 16. 
To fix this problem we can use the alignas specifier introduced in C++11:
*/

//uniformBufferObject is the object ubo
struct UniformBufferObject {
    //glm::vec2 someVec2;
    //alignas(16) glm::mat4 model;
    //alignas alligs the shit together, be explicit with alignas as when using nested structures the 
    //GLM_FORCE_DEFAULT_ALIGNED_GENTYPES wont save ya 
    glm::mat4 model;
    //glm::mat4 view; view is in push constants
};

struct GlobalUniformBufferObject {
    glm::mat4 proj;
    alignas(16) glm::vec3 ambientLightCol;
    alignas(16) glm::vec3 lightDir;
    alignas(16) glm::vec3 viewPos;
    alignas(4) float ambientStrength;
    alignas(4) float specExponent;
    //alignas(16) glm::vec3 viewPos;
};

struct ComputeUniformBufferObject {
    glm::vec3 colOffset;
};

/*
 As some of the structures and function calls hinted at, it is actually possible to bind multiple descriptor sets 
 simultaneously. You need to specify a descriptor layout for each descriptor set when creating the pipeline layout. 
 Shaders can then reference specific descriptor sets like this:

 layout(set = 0, binding = 0) uniform UniformBufferObject { ... }

 You can use this feature to put descriptors that vary per-object and descriptors that are shared into separate descriptor sets. 
 In that case you avoid rebinding most of the descriptors across draw calls which is potentially more efficient.
*/

