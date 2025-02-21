#version 450

layout(set = 0,binding = 0) uniform GlobalUniformBufferObject {
    mat4 proj;
    vec3 ambientCol;
    vec3 lightDir;
    vec3 viewPos;
    float ambientStrength;
    float specExp;
} gubo;

layout(set = 1,binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void main() {
    //diffuse
    float diff = max(dot(fragNormal, -gubo.lightDir), 0.0);
    vec3 diffuse = diff * vec3(1, 1, 1); 

    //specular
    float specularStrength = 1;
    vec3 viewDir = normalize(gubo.viewPos - fragPos);
    vec3 reflectDir = reflect(gubo.lightDir, fragNormal);  

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), gubo.specExp);
    vec3 specular = specularStrength * spec * vec3(1, 1, 1);  

    //ambient
    vec3 am = gubo.ambientCol * gubo.ambientStrength;

    /*both adding specular to the result and the end or before multiplying with colour seems to work....*/
    vec3 result = (am + diffuse /*+ specular*/) * texture(texSampler, fragTexCoord).rgb;
    result = result + specular;
    
    outColor = vec4(result, 1.0);
}
