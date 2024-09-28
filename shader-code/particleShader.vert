#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec3 fragColor;

layout( push_constant) uniform constants {
    mat4 view;
    double deltaTime;
} PushConstants;

void main() {
    gl_PointSize = 14.0;
    gl_Position = vec4(inPosition.xy, 1.0, 1.0);
    vec3 col = inColor.rgb;
    fragColor = col * vec3(inPosition.xy, 1);
}
