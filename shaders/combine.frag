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

    vec3 albedo = texture(albedoMap, inTexcoord).rgb;
    vec3 normal = texture(normalMap, inTexcoord).rgb * 2.0 - 1.0;
    float specularIntensity = texture(specularityMap, inTexcoord).r;
    vec3 worldPos = texture(worldPosMap, inTexcoord).rgb;

    vec3 lightDir = normalize(lightPosition - worldPos);
    vec3 viewDir = normalize(viewPosition - worldPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * albedo;

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularIntensity * spec * lightColor;

    outColor.rgb = diffuse + specular;
    outColor.a = 1.0;

     outColor.rgb = albedo; //albedo
//     outColor.rgb = normal * 0.5 + 0.5; // normal
//     outColor.rgb = vec3(specularIntensity); // s pecular
     //outColor.rgb = worldPos; // worldpos
//     outColor.rgb = vec3(inTexcoord.x, inTexcoord.y, 0); //UV
}