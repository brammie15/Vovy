#version 450

layout(set = 0, binding = 0) uniform sampler2D albedoMap;
layout(set = 0, binding = 1) uniform sampler2D normalMap;
layout(set = 0, binding = 2) uniform sampler2D specularityMap;
layout(set = 0, binding = 3) uniform sampler2D worldPosMap;

layout(location = 0) in vec2 inTexcoord;

layout(location = 0) out vec4 outColor;

void main() {
    // Hardcoded light parameters
    vec3 lightPosition = vec3(2.0, 5.0, 3.0);  // World space light position
    vec3 lightColor = vec3(1.0, 1.0, 1.0);      // White light
    vec3 viewPosition = vec3(0.0, 0.0, 5.0);     // Camera position

    // Sample all textures
    vec3 albedo = texture(albedoMap, inTexcoord).rgb;
    vec3 normal = texture(normalMap, inTexcoord).rgb * 2.0 - 1.0; // Unpack from [0,1] to [-1,1]
    float specularIntensity = texture(specularityMap, inTexcoord).r;
    vec3 worldPos = texture(worldPosMap, inTexcoord).rgb;

    // Simple lighting calculation (Phong model)
    vec3 lightDir = normalize(lightPosition - worldPos);
    vec3 viewDir = normalize(viewPosition - worldPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    // Diffuse component
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * albedo;

    // Specular component
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularIntensity * spec * lightColor;

    // Combine results
    outColor.rgb = diffuse + specular;
    outColor.a = 1.0;

    // Uncomment to debug specific components:
    // outColor.rgb = albedo; // Show just albedo
    // outColor.rgb = normal * 0.5 + 0.5; // Show normals (mapped back to [0,1])
    // outColor.rgb = vec3(specularIntensity); // Show specular map
    // outColor.rgb = worldPos; // Show world positions (may need scaling)
    // outColor.rgb = vec3(inTexcoord.x, inTexcoord.y, 0); // Show UV coordinates
}