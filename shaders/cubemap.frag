#version 450

layout(location = 0) in vec3 localPos;

// Input equirectangular HDR texture
layout(binding = 0) uniform sampler2D equirectangularMap;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

vec2 sampleSphericalMap(vec3 v) {
    // Convert direction to spherical coordinates
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv /= vec2(2.0 * PI, PI);
    uv += 0.5;
    return uv;
}

void main() {
    // Normalize direction vector
    vec3 dir = normalize(localPos);

    // Sample from equirectangular texture
    vec2 uv = sampleSphericalMap(dir);
    vec3 color = texture(equirectangularMap, uv).rgb;

    outColor = vec4(color, 1.0);
}