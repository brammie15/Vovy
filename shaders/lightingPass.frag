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
    float aperture;
    float shutterSpeed;
    float iso;
    float _pad0; // Padding to maintain 16-byte alignment
};

layout(std140, set = 0, binding = 0) uniform globalUBO
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    CameraSettings camSettings;
    DirectionalLightInfo light;
    ivec4 pointLightCount;
    vec2 viewportSize;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessMap;
layout(set = 1, binding = 3) uniform sampler2D worldPosMap;
layout(set = 1, binding = 4) uniform sampler2D selectionMap;

layout(set = 1, binding = 5) uniform sampler2D depthMap;

layout(set = 2, binding = 0) uniform textureCube hdriTexture;
layout(set = 2, binding = 1) uniform sampler hdriSampler;

layout(set = 2, binding = 2) uniform textureCube diffuseIrradianceMap;
layout(set = 2, binding = 3) uniform sampler diffuseIrradianceSampler;


layout(set = 3, binding = 0) readonly buffer PointLights {
    PointLight pointLights[];
};


layout(location = 0) in vec2 inTexcoord;

layout(location = 0) out vec4 outColor;

// Calculate attenuation for point lights
float calculateAttenuation(float distance, float range) {
    float attenuation = 1.0 / (distance * distance);
    float rangeFactor = pow(clamp(1.0 - pow(distance / range, 4.0), 0.0, 1.0), 2.0);
    return attenuation * rangeFactor;
}

vec3 GetWorldPositionFromDepth(in float depth, in vec2 fragCoords, in vec2 resolution, in mat4 invProj, in mat4 invView) {
    vec2 ndc = vec2(
    (fragCoords.x / resolution.x) * 2.0 - 1.0,
    (fragCoords.y / resolution.y) * 2.0 - 1.0
    );
    ndc.y *= -1.0;
    const vec4 clipPos = vec4(ndc, depth, 1.0);

    vec4 viewPos = invProj * clipPos;
    viewPos /= viewPos.w;

    vec4 worldPos = invView * viewPos;
    return worldPos.xyz;
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
    float G = GeometrySmith(N, V, L, roughness, false);
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
    //hdri
    vec3 albedo = texture(albedoMap, inTexcoord).rgb;
    vec3 normal = normalize(texture(normalMap, inTexcoord).rgb * 2.0 - 1.0);
    vec3 worldPos = texture(worldPosMap, inTexcoord).rgb;
    vec2 metallicRoughness = texture(metallicRoughnessMap, inTexcoord).bg;
    float metallic = metallicRoughness.x;
    float roughness = metallicRoughness.y;
    float ao = 1.0;
    float depth = texture(depthMap, inTexcoord).r;

    if(depth >= 1.f){
        vec2 fragCoord = vec2(gl_FragCoord.x, gl_FragCoord.y);
        const vec3 sampleDirection = GetWorldPositionFromDepth(depth, fragCoord, ubo.viewportSize, inverse(ubo.projectionMatrix), inverse(ubo.viewMatrix));
        vec3 normalizedSampleDirection = normalize(sampleDirection);
        outColor = vec4(texture(samplerCube(hdriTexture, hdriSampler), normalizedSampleDirection).rgb, 1.0);
//        outColor.rgb = normalizedSampleDirection;
        return;
    }

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
    float G = GeometrySmith(N, V, L, roughness, false);
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

    const vec3 prefilteredDiffuseIrradianec = texture(samplerCube(diffuseIrradianceMap, diffuseIrradianceSampler), vec3(normal.x,-normal.y, normal.z)).rgb;
    const vec3 diffuse = albedo * prefilteredDiffuseIrradianec;


    vec3 ambient = kD * diffuse;
    vec3 color = ambient + Lo;

    // Tone mapping and gamma correction
//    color = color / (color + vec3(1.0));
//    color = pow(color, vec3(1.0/2.2));

    outColor.rgb = color;
    outColor.a = 1.0;

    //    float z = depth * 2.0 - 1.0; // back to NDC
//    float linearDepth = (2.0 * ubo.projectionMatrix[3][2]) /
//    (ubo.projectionMatrix[2][2] - z * ubo.projectionMatrix[2][3]);
//
//    // Normalize depth to a viewable range (e.g., between 0 and 1)
//    float depthView = clamp(linearDepth / 100.0, 0.0, 1.0); // adjust 100.0 to your far plane or visual preference
//    outColor.rgb = vec3(depthView);
}