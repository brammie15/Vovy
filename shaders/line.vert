#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 normal;

layout(location = 0) out vec3 fragColor;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 view;
    mat4 projection;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;


void main() {
    fragColor = inColor;
    gl_Position = ubo.projection * ubo.view * push.model * vec4(inPosition, 1.0);
}
