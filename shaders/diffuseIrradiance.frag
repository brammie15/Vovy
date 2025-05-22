#version 450

layout(location = 0) in vec3 localPos;

// Input cubemap texture
layout(binding = 0) uniform samplerCube environmentMap;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;
const float HALF_PI = 1.57079632679;

void main() {
    // The normal direction is the direction vector pointing out from the cube face
    vec3 normal = normalize(localPos);

    // Use a fixed coordinate system for consistent sampling
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));
    vec3 tangent = right;
    vec3 bitangent = up;

    vec3 irradiance = vec3(0.0);

    float nrSamples = 0.f;
    const float sampleDelta = 0.025f;
    for(float phi = 0.0f; phi < TWO_PI; phi += sampleDelta){
        for(float theta = 0.0f; theta < HALF_PI; theta += sampleDelta){
            vec3 tangentSample = vec3(
                sin(theta) * cos(phi),
                sin(theta) * sin(phi),
                cos(theta)
            );

            vec3 sampleVec =
            tangentSample.x * tangent + tangentSample.y * bitangent +
            tangentSample.z * normal;

            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            ++nrSamples;
        }
    }

    irradiance = PI * irradiance * (1.f / float(nrSamples));
    outColor = vec4(irradiance, 1.f);
}