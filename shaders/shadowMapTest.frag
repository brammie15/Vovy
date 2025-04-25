#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 FragPos;
layout(location = 3) in vec4 FragPosLightSpace;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 view;
    mat4 projection;

    mat4 lightSpaceMatrix;
} ubo;


layout(set = 0, binding = 1) uniform sampler2D shadowSampler;
layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;

    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    vec2 samplePos = vec2(projCoords.x,  1 - projCoords.y );
    if(samplePos.x < 0.0 || samplePos.x > 1.0 || samplePos.y < 0.0 || samplePos.y > 1.0)
    {
        outColor = vec4(0.0f, 1.0f, 1.0f, 1.0f);
        return;
    }

    float shadowValue = texture(shadowSampler, samplePos).r;

    float currentDepth = projCoords.z;

    bool isShadow = currentDepth > (shadowValue + 0.010);

    outColor = vec4(vec3(!isShadow), 1.0);
}
