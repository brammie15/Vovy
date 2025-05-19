#version 450
#extension GL_GOOGLE_include_directive : enable
#include "lighting.glsl"

struct DirectionalLightInfo {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float range;
};

struct PointLightInfo {
    vec3 position;
    float pad1;
    vec3 color;
    float intensity;
    float radius;
    float padding[3]; // total = 64 bytes, 16-byte aligned
};


struct CameraSettings {
    vec4 cameraPos;
    float exposure;
};

layout(std140, set = 0, binding = 0) uniform globalUBO
{
    CameraSettings camSettings;
    DirectionalLightInfo light;
    ivec4 pointLightCount;
} ubo;


layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessMap;
layout(set = 1, binding = 3) uniform sampler2D worldPosMap;
layout(set = 1, binding = 4) uniform sampler2D selectionMap;

layout(set = 2, binding = 0) readonly buffer PointLights {
    PointLight pointLights[];
};


layout(location = 0) in vec2 inTexcoord;

layout(location = 0) out vec4 outColor;

// Calculate attenuation for point lights
float calculateAttenuation(float distance, float range) {
    // Inverse square law with range cutoff
    float attenuation = 1.0 / (distance * distance);
    // Smooth falloff near the light's range
    float rangeFactor = pow(clamp(1.0 - pow(distance / range, 4.0), 0.0, 1.0), 2.0);
    return attenuation * rangeFactor;
}

vec3 calculatePointLightContribution(PointLight light, vec3 worldPos, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3 L = normalize(light.position - worldPos);
    float distance = length(light.position - worldPos);

    // Skip lights that are too far away
    if (distance > light.range) {
        return vec3(0.0);
    }

    vec3 H = normalize(V + L);
    float attenuation = calculateAttenuation(distance, light.range);
    vec3 radiance = light.color * light.intensity * attenuation;

    // Cook-Torrance BRDF
    float NDF = NormalDistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}


void main()
{
    vec3 albedo = texture(albedoMap, inTexcoord).rgb;
    vec3 normal = normalize(texture(normalMap, inTexcoord).rgb * 2.0 - 1.0);
    vec3 worldPos = texture(worldPosMap, inTexcoord).rgb;
    vec2 metallicRoughness = texture(metallicRoughnessMap, inTexcoord).bg;
    float metallic = metallicRoughness.x;
    float roughness = metallicRoughness.y;
    float ao = 1.0;

    vec3 camPos = ubo.camSettings.cameraPos.xyz;
    vec3 N = normal;
    vec3 V = normalize(camPos - worldPos);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Reflectance equation
    vec3 Lo = vec3(0.0);

    // Directional light contribution
    vec3 L = normalize(-ubo.light.direction);
    vec3 H = normalize(V + L);
    vec3 radiance = ubo.light.color * ubo.light.intensity;

    // Cook-Torrance BRDF for directional light
    float NDF = NormalDistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;

    // Point lights contribution
    for (uint i = 0; i < ubo.pointLightCount.x; ++i) {
        PointLight light = pointLights[i];
        Lo += calculatePointLightContribution(light, worldPos, N, V, albedo, metallic, roughness, F0);
    }

    // Ambient lighting
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = (ambient + Lo) * ubo.camSettings.exposure;

    // Tone mapping and gamma correction
//    color = color / (color + vec3(1.0));
//    color = pow(color, vec3(1.0/2.2));

    outColor.rgb = color;
    outColor.a = 1.0;
}