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

vec3 toneMapReinhard(vec3 hdr) {
    return hdr / (hdr + vec3(1.0));
}

vec3 gammaCorrect(vec3 color) {
    return pow(color, vec3(1.0 / 2.2));
}

vec3 Uncharted2ToneMappingCurve(in vec3 color){
    const float a = 0.15f;
    const float b = 0.50f;
    const float c = 0.10f;
    const float d = 0.20f;
    const float e = 0.02f;
    const float f = 0.30f;
    return ((color * (a * color + c * b) + d * e) / (color * (a * color + b) + d * f)) - e / f;
}

vec3 Uncharted2ToneMapping(in vec3 color){
    const float W = 11.2f;
    const vec3 curvedColor = Uncharted2ToneMappingCurve(color);
    float whiteScale = 1.f / Uncharted2ToneMappingCurve(vec3(W)).r;
    return clamp(curvedColor * whiteScale, 0.f, 1.f);
}

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
//    outColor = vec4(clamp(hdrColor.rgb, 0.0, 1.0), 1.0);

    //    // Apply exposure
    hdrColor *= exposure;
//    // Reinhard tone mapping
    hdrColor = Uncharted2ToneMapping(hdrColor);

    // Gamma correction
//    hdrColor = pow(hdrColor, vec3(1.0 / 2.2));
    outColor = vec4(hdrColor, 1.0);
}