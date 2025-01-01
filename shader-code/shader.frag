#version 450

layout(set = 0,binding = 0) uniform GlobalUniformBufferObject {
    mat4 proj;
    vec3 ambientCol;
    vec3 lightDir;
    float ambientStrength;
} gubo;

layout(set = 1,binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void main() {
    float diff = max(dot(fragNormal, gubo.lightDir), 0.0);
    vec3 diffuse = diff * vec3(1, 1, 1); 

    vec3 result = (fragColor + diffuse) * texture(texSampler, fragTexCoord).rgb;
    outColor = vec4(result, 1.0);
}
