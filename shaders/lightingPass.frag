#version 450
#extension GL_GOOGLE_include_directive : enable
#include "lighting.glsl"
#include "DebugModes.glsl"

struct DirectionalLightInfo {
    vec3 direction;
    vec3 color;
    float intensity;
    mat4 shadowViewProj;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float range;
};

struct CameraSettings {
    vec4 cameraPos;
    float aperture;
    float shutterSpeed;
    float iso;
    float _pad0;// Padding to maintain 16-byte alignment
};

layout(std140, set = 0, binding = 0) uniform globalUBO
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    CameraSettings camSettings;
    DirectionalLightInfo light;
    ivec4 pointLightCount;
    vec2 viewportSize;
    vec2 _padding;// Padding to maintain 16-byte alignment
    int debugMode;
} ubo;


layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessMap;
layout(set = 1, binding = 3) uniform sampler2D worldPosMap;
layout(set = 1, binding = 4) uniform sampler2D selectionMap;

layout(set = 1, binding = 5) uniform sampler2D depthMap;

layout(set = 1, binding = 6) uniform sampler2D shadowMap;

layout(set = 2, binding = 0) uniform textureCube hdriTexture;
layout(set = 2, binding = 1) uniform sampler hdriSampler;

layout(set = 2, binding = 2) uniform textureCube diffuseIrradianceMap;
layout(set = 2, binding = 3) uniform sampler diffuseIrradianceSampler;


layout(set = 3, binding = 0) readonly buffer PointLights {
    PointLight pointLights[];
};

layout(location = 0) in vec2 inTexcoord;

layout(location = 0) out vec4 outColor;

float calculateShadow(vec3 worldPos)
{
    vec4 lightSpacePosition = ubo.light.shadowViewProj * vec4(worldPos, 1.0);
    lightSpacePosition.xyz /= lightSpacePosition.w;

    if (lightSpacePosition.z < 0.0 || lightSpacePosition.z > 1.0 ||
    lightSpacePosition.x < -1.0 || lightSpacePosition.x > 1.0 ||
    lightSpacePosition.y < -1.0 || lightSpacePosition.y > 1.0) {
        return 0.0;
    }

    vec3 shadowMapUV = vec3(lightSpacePosition.xy * 0.5 + 0.5, lightSpacePosition.z);
    shadowMapUV.y = 1.0 - shadowMapUV.y;// Flip Y for Vulkan

    vec3 N = normalize(texture(normalMap, inTexcoord).rgb * 2.0 - 1.0);
    vec3 L = normalize(-ubo.light.direction);
    float bias = max(0.005 * (1.0 - dot(N, L)), 0.001);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float shadowDepth = texture(shadowMap, shadowMapUV.xy + vec2(x, y) * texelSize).r;
            shadow += (shadowMapUV.z - bias) > shadowDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

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

    if (ubo.debugMode == DEBUG_MODE_FULLSCREEN_SHADOWS) {
        float depthValue = texture(shadowMap, inTexcoord).r;
        outColor = vec4(vec3(depthValue), 1.0);// Show as grayscale
        return;
    }

    //hdri
    vec3 albedo = texture(albedoMap, inTexcoord).rgb;
    vec3 normal = normalize(texture(normalMap, inTexcoord).rgb * 2.0 - 1.0);
    vec3 worldPos = texture(worldPosMap, inTexcoord).rgb;
    vec2 metallicRoughness = texture(metallicRoughnessMap, inTexcoord).bg;
    float metallic = metallicRoughness.x;
    float roughness = metallicRoughness.y;
    float ao = 1.0;
    float depth = texture(depthMap, inTexcoord).r;

    if (depth >= 1.f){
        vec2 fragCoord = vec2(gl_FragCoord.x, gl_FragCoord.y);
        const vec3 sampleDirection = GetWorldPositionFromDepth(depth, fragCoord, ubo.viewportSize, inverse(ubo.projectionMatrix), inverse(ubo.viewMatrix));
        vec3 normalizedSampleDirection = normalize(sampleDirection);
        normalizedSampleDirection.y *= -1.0;// Flip Y for Vulkan
        outColor = vec4(texture(samplerCube(hdriTexture, hdriSampler), normalizedSampleDirection).rgb, 1.0);
//                outColor.rgb = normalizedSampleDirection;
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

    float shadow = calculateShadow(worldPos);
    Lo *= 1 - shadow;

    // Point lights contribution
    for (uint i = 0; i < ubo.pointLightCount.x; ++i) {
        PointLight light = pointLights[i];
        Lo += calculatePointLightContribution(light, worldPos, N, V, albedo, metallic, roughness, F0);
    }

    const vec3 prefilteredDiffuseIrradianec = texture(samplerCube(diffuseIrradianceMap, diffuseIrradianceSampler), vec3(normal.x, -normal.y, normal.z)).rgb;
    const vec3 diffuse = albedo * (prefilteredDiffuseIrradianec * 1);


    vec3 ambient = kD * diffuse;
    vec3 color = ambient + Lo;

    outColor.rgb = color;
    outColor.a = 1.0;


    switch (ubo.debugMode) {
        case DEBUG_MODE_ALBEDO:
        outColor.rgb = albedo;
        break;
        case DEBUG_MODE_NORMAL:
        outColor.rgb = normal * 0.5 + 0.5;
        break;
        case DEBUG_MODE_SPECULAR:
        outColor.rgb = F0;
        break;
        case DEBUG_MODE_ROUGHNESS:
        outColor.rgb = vec3(roughness);
        break;
        case DEBUG_MODE_METALLIC:
        outColor.rgb = vec3(metallic);
        break;
        case DEBUG_MODE_DEPTH:
        outColor.rgb = vec3(depth);
        break;
        case DEBUG_MODE_POSITION:
        outColor.rgb = worldPos;
        break;
        case DEBUG_MODE_UV:
        outColor.rgb = vec3(inTexcoord, 0.0);
        break;
        case DEBUG_MODE_LIGHTING:
        outColor.rgb = Lo;
        break;
        case DEBUG_MODE_SPECULAR_LIGHTING:
        outColor.rgb = specular;
        break;
        case DEBUG_MODE_SHADOWS:
        outColor.rgb = vec3(shadow);
        break;
        default :
        break;
    }


}