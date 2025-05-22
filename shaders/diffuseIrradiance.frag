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

    vec3 irradiance = vec3(0.0);

    // Number of samples per axis (total samples = sampleCount^2)
    const uint sampleCount = 32;
    float sampleDelta = 0.025;
    float totalWeight = 0.0;

    // Convolution by sampling hemisphere oriented around normal
    for(float phi = 0.0; phi < TWO_PI; phi += sampleDelta) {
        for(float theta = 0.0; theta < HALF_PI; theta += sampleDelta) {
            // Spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(
            sin(theta) * cos(phi),
            sin(theta) * sin(phi),
            cos(theta)
            );

            // Tangent space to world space
            vec3 sampleVec = tangentSample.x * right +
            tangentSample.y * up +
            tangentSample.z * normal;

            // Sample the environment map
            vec3 sampleColor = texture(environmentMap, sampleVec).rgb;

            // Lambertian diffuse BRDF - cosine weight
            irradiance += sampleColor * cos(theta) * sin(theta);
            totalWeight += cos(theta) * sin(theta);
        }
    }

    // Normalize and scale by PI (account for Lambertian BRDF)
    irradiance = PI * irradiance / totalWeight;

    outColor = vec4(irradiance, 1.0);
}