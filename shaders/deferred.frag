#version 450
#extension GL_GOOGLE_include_directive : enable

#include "lighting.glsl"

layout(set = 0, binding = 0) uniform MatrixUBO
{
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform constants
{
    mat4 model;
    int objectId;

} modelData;


layout(std140, set = 1, binding = 0) uniform bindingInfo{
    bool hasAlbedo;
    bool hasNormal;
    bool hasSpecular;
    bool hasBump;
} textureBindingInfo;

layout(set = 1, binding = 1) uniform sampler2D albedoSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;
layout(set = 1, binding = 3) uniform sampler2D metallicRoughnessSampler;
layout(set = 1, binding = 4) uniform sampler2D bumpSampler;

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitTangent;


layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outWorldPos;
layout(location = 3) out vec4 outMetallicRoughness;
layout(location = 4) out vec4 outSelection;

const bool USE_BUMP_MAP = false;

vec3 boolsToColor(bool hasNormal, bool hasSpecular, bool hasBump) {

    float r = hasNormal ? 1.0 : 0.0;
    float g = hasSpecular ? 1.0 : 0.0;
    float b = hasBump ? 1.0 : 0.0;

    r = hasNormal ? 0.8 : 0.2;
    g = hasSpecular ? 0.8 : 0.2;
    b = hasBump ? 0.8 : 0.2;

    return vec3(r, g, b);
}

vec3 hashToColor(float n) {
    // Large prime-based hashing for each channel
    float r = fract(sin(n * 12.9898) * 43758.5453);
    float g = fract(sin(n * 78.233) * 12345.6789);
    float b = fract(sin(n * 37.719) * 98765.4321);

    // Optionally boost contrast for visibility
    return vec3(r, g, b);
}

void main(){

    if(textureBindingInfo.hasAlbedo) {
//        outAlbedo.rgb = pow(texture(albedoSampler, inTexCoord).rgb, vec3(2.2)); // Assume sRGB
        outAlbedo.rgb = texture(albedoSampler, inTexCoord).rgb; // Assume sRGB

        if(outAlbedo.a < 0.1) {
            discard;
        }
    } else {
        outAlbedo.rgb = pow(inColor, vec3(2.2));
    }
    outAlbedo.a = 1.0;

    vec3 normal = normalize(inNormal);
    vec3 tangent = normalize(inTangent);
    vec3 bitTangent = normalize(inBitTangent);

    if (USE_BUMP_MAP && textureBindingInfo.hasBump) {
        // Bump mapping code
        mat3 tbn = mat3(tangent, bitTangent, normal);
        vec2 texCoord = clamp(inTexCoord, vec2(0.001), vec2(0.999));
        float height = texture(bumpSampler, texCoord).r;

        vec2 texelSize = vec2(1.0) / textureSize(bumpSampler, 0);
        float h1 = texture(bumpSampler, texCoord + vec2(texelSize.x, 0.0)).r;
        float h2 = texture(bumpSampler, texCoord - vec2(texelSize.x, 0.0)).r;
        float h3 = texture(bumpSampler, texCoord + vec2(0.0, texelSize.y)).r;
        float h4 = texture(bumpSampler, texCoord - vec2(0.0, texelSize.y)).r;

        float deltaX = (h1 - h2) * 0.5;
        float deltaY = (h3 - h4) * 0.5;

        vec3 bumpNormal = normalize(tbn * vec3(-deltaX, -deltaY, 1.0));
        normal = bumpNormal;
    } else if (!USE_BUMP_MAP && textureBindingInfo.hasNormal) {
        // Normal mapping code
        vec3 testBitangent = cross(normal, tangent);
        mat3 tbn = mat3(tangent, testBitangent, normal);
        vec3 sampledNormal = texture(normalSampler, inTexCoord).rgb;
        sampledNormal = sampledNormal * 2.0 - 1.0;
        normal = normalize(tbn * sampledNormal);
    }

    outNormal = vec4(clamp(normal * 0.5 + 0.5, vec3(0.0), vec3(1.0)), 1.0);

    outMetallicRoughness = vec4(0.0, 0.5, 1.0, 1.0);
    if(textureBindingInfo.hasSpecular) {
        vec3 mr = texture(metallicRoughnessSampler, inTexCoord).rgb;
        outMetallicRoughness.b = mr.b;  // Metallic in blue channel (common)
        outMetallicRoughness.g = mr.g;  // Roughness in green channel
    }

//    if(textureBindingInfo.hasAO) {
//        outMetallicRoughnessAO.b = texture(aoSampler, inTexCoord).r;
//    }

    outWorldPos.rgb = inWorldPos.xyz;

    outSelection.rgb = hashToColor(float(modelData.objectId));
//    outSelection.r = float(modelData.objectId & 0xFF) / 255.0f;
//    outSelection.g = float((modelData.objectId >> 8) & 0xFF) / 255.0f;
//    outSelection.b = float((modelData.objectId >> 16) & 0xFF) / 255.0f;

//    outNormal = vec4(boolsToColor(textureBindingInfo.hasNormal, textureBindingInfo.hasSpecular, textureBindingInfo.hasBump), 1.0f);
}