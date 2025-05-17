#version 450
#extension GL_GOOGLE_include_directive : enable
#include "lighting.glsl"

struct DirectionalLightInfo {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct CameraSettings {
    vec4 cameraPos;
    float exposure;
};

layout(std140, set = 0, binding = 0) uniform globalUBO
{
    CameraSettings camSettings;
    DirectionalLightInfo light;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessMap;
layout(set = 1, binding = 3) uniform sampler2D worldPosMap;
layout(set = 1, binding = 4) uniform sampler2D selectionMap;

layout(location = 0) in vec2 inTexcoord;

layout(location = 0) out vec4 outColor;

void main()
{
    // Sample textures
    vec3 albedo = texture(albedoMap, inTexcoord).rgb;
    vec3 normal = normalize(texture(normalMap, inTexcoord).rgb * 2.0 - 1.0);
    vec3 worldPos = texture(worldPosMap, inTexcoord).rgb;
    vec2 metallicRoughness = texture(metallicRoughnessMap, inTexcoord).bg;
    float metallic = metallicRoughness.x;
    float roughness = metallicRoughness.y;
    float ao = 1.0; // You might want to sample this from a texture too

    // Single directional light parameters
//    vec3 lightDirection = normalize(vec3(0.577f, -0.577f, -0.577f)); // Direction pointing down and slightly forward
//    vec3 lightColor = vec3(1.0, 1.0, 1.0); // White light

    vec3 lightDirection = ubo.light.direction;
    vec3 lightColor = ubo.light.color;

    vec3 camPos = ubo.camSettings.cameraPos.xyz;

    vec3 N = normal;
    vec3 V = normalize(camPos - worldPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);

    // calculate per-light radiance (directional light has no position or attenuation)
    vec3 L = normalize(-lightDirection);
    vec3 H = normalize(V + L);
    vec3 radiance = lightColor * ubo.light.intensity; // intensity in lux (W/m²)

    // cook-torrance brdf
    float NDF = NormalDistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    vec3 ambient = vec3(0.03) * albedo * ao;
//    vec3 color = ambient + Lo;
    vec3 color = (ambient + Lo) * ubo.camSettings.exposure;

//    color = color / (color + vec3(1.0));
//    color = pow(color, vec3(1.0/2.2));

    // Debug outputs (uncomment what you need)
    // outColor.rgb = albedo; // albedo
    // outColor.rgb = normal * 0.5 + 0.5; // normal
//     outColor.rgb = vec3(metallic); // metallic
//     outColor.rgb = vec3(roughness); // roughness
    // outColor.rgb = worldPos; // worldpos
    // outColor.rgb = vec3(inTexcoord.x, inTexcoord.y, 0); // UV
    outColor.rgb = color; // final color
    outColor.a = 1.0;
}