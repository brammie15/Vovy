#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texCoord;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outTexCoord;

layout(push_constant) uniform PushConstants {
    mat2 transform;
    vec2 offset;
    vec3 color;
} push;

void main() {
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(position, 1.0);
    outTexCoord = texCoord;
    outColor = color * push.color;
}