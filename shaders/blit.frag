#version 450
#extension GL_GOOGLE_include_directive : enable
#include "PhysicalCameraMath.glsl"

struct CameraSettings {
    vec4 cameraPos;
    float aperture;
    float shutterSpeed;
    float iso;
    float _pad0; // Padding to maintain 16-byte alignment
};

layout(std140, set = 0, binding = 0) uniform globalUBO
{
    CameraSettings camSettings;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D image;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(1.0, 1.0, 0.0, 1.0);

    float ev100 = CalculateEV100FromPhysicalCamera(
    ubo.camSettings.aperture,
    ubo.camSettings.shutterSpeed,
    ubo.camSettings.iso
    );
    float exposure = ConvertEV100ToExposure(ev100);

    vec3 hdrColor = texture(image, fragTexCoord).rgb;

    // Apply exposure
    hdrColor *= exposure;

    // Reinhard tone mapping
    hdrColor = hdrColor / (hdrColor + vec3(1.0));

    // Gamma correction
    hdrColor = pow(hdrColor, vec3(1.0 / 2.2));

    outColor = vec4(hdrColor, 1.0);
}