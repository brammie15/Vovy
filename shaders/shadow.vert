#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 normal;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 FragPos;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 view;
    mat4 projection;

    mat4 lightSpaceMatrix;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

void main() {
    gl_Position = ubo.projection * ubo.view * push.model * vec4(position, 1.0);
    outTexCoord = texCoord;
    FragPos = vec3(push.model * vec4(position, 1.0));
    outNormal = mat3(transpose(inverse(push.model))) * normal;
}