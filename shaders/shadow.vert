#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitTangent;

//layout(location = 0) out vec2 outTexCoord;
//layout(location = 1) out vec3 outNormal;
//layout(location = 2) out vec3 FragPos;

layout(set = 0, binding = 0) uniform LightUbo {
    mat4 lightViewMatrix;       // Light's view matrix
    mat4 lightProjectionMatrix; // Light's projection matrix
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
    int objectId;
} push;

void main() {
    gl_Position = ubo.lightProjectionMatrix * ubo.lightViewMatrix * push.model * vec4(inPosition, 1.0);
//    gl_Position.z = 0.5 * gl_Position.w;
}