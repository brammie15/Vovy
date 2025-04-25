#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 FragPos;


layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 view;
    mat4 projection;

    mat4 lightSpaceMatrix;
} ubo;
layout(set = 0, binding = 1) uniform sampler2D shadowSampler; //Euuh

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

void main() {
    outColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);
}