#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTexCoord;


layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
    mat2 transform;
    vec2 offset;
    vec3 color;
} push;

void main() {
//    outColor = vec4(push.color, 1.0f);
//    outColor = vec4(inTexCoord, 0.0f, 1.0f);
    outColor = texture(texSampler, vec2(inTexCoord.x, 1.0f - inTexCoord.y));
}