#version 450

layout(set = 0,binding = 0) uniform GlobalUniformBufferObject {
    mat4 proj;
    vec3 ambientCol;
    vec3 lightDir;
    vec3 viewPos;
    float ambientStrength;
    float specExp;
} gubo;

layout(set = 1,binding = 0) uniform ObjectUniformBufferObject {
    mat4 model;
} oubo;

layout( push_constant) uniform constants {
    mat4 view;
    float deltaTime;
} PushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 5) in vec3 inNormal;

layout(location = 3) in vec3 inPosOffset;
layout(location = 4) in vec3 inColOffset;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPos;

void main() {
    vec3 pos = inPosition + inPosOffset;
    gl_Position = gubo.proj * PushConstants.view * oubo.model * vec4(pos, 1.0);

    fragNormal = inNormal;
    fragPos = vec3(oubo.model * vec4(inPosition, 1.0));

    vec3 ambient = gubo.ambientStrength * gubo.ambientCol;
    fragColor = (inColor * inColOffset) * ambient; 
    fragTexCoord = inTexCoord;
}
