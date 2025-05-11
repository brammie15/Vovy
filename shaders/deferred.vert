#version 450

layout(set = 0, binding = 0) uniform MatrixUBO
{
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform constants
{
    mat4 model;
} modelData;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitTangent;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec2 outTexcoord;
layout(location = 3) out vec3 outNormal;
layout(location = 4) out vec3 outTangent;
layout(location = 5) out vec3 outBitTangent;

void main()
{
    gl_Position = ubo.proj * ubo.view * modelData.model * vec4(inPosition, 1.0);
    outColor = inColor;
    outNormal = normalize(mat3(modelData.model) * normal);
    outTangent = normalize(mat3(modelData.model) * tangent);
    outBitTangent = normalize(mat3(modelData.model) * bitTangent);
    outTexcoord = texCoord;
    outPosition = (modelData.model * vec4(inPosition, 1.0)).rgb;
}