#version 450

layout(set = 0, binding = 0) uniform sampler2D image;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(1.0, 1.0, 0.0, 1.0);

    outColor = texture(image, fragTexCoord);
}