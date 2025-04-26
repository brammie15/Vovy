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

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    vec2 samplePos = vec2(projCoords.x,  projCoords.y);
    if(samplePos.x < 0.0 || samplePos.x > 1.0 || samplePos.y < 0.0 || samplePos.y > 1.0) {
        return 0;
    }

    float closestDepth = texture(shadowSampler, samplePos).r;
    float currentDepth = projCoords.z;

//    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    float bias = 0.005;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowSampler, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowSampler, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

//    shadow = currentDepth - bias > closestDepth ? 0.0 : 1.0;

//    if(projCoords.z > 1.0){
//        return 0.0;
//    }

    return shadow;
}

void main() {
    vec3 lightDir = normalize(vec3(0.0f, -0.5f, 0.5f));
    vec3 normal = normalize(inNormal);

    float shadow = ShadowCalculation(FragPosLightSpace, normal, lightDir);

    vec3 ambient = vec3(0.2f, 0.2f, 0.2f);

    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuseColor = texture(texSampler, inTexCoord).rgb;

    vec3 lighting = (ambient + (1.0 - shadow)) * (diffuseColor);

//    outColor = vec4(diffuseColor * (1 - shadow), 1.0f);
    outColor = vec4(lighting, 1.0f);
}