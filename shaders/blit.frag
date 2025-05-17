#version 450

layout(set = 0, binding = 0) uniform exposureUBO {
    float exposure;
};

layout(set = 0, binding = 1) uniform sampler2D image;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(1.0, 1.0, 0.0, 1.0);

    vec3 hdrColor = texture(image, fragTexCoord).rgb;

    hdrColor *= exposure;

    // Reinhard
    hdrColor = hdrColor / (hdrColor + vec3(1.0));

    // Gamma correction
    hdrColor = pow(hdrColor, vec3(1.0 / 2.2));

    outColor = vec4(hdrColor, 1.0);
}