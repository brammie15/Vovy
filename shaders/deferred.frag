#version 450

layout(set = 0, binding = 0) uniform MatrixUBO
{
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform constants
{
    mat4 model;
} modelData;

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragTangent;
layout(location = 3) in vec3 fragBitangent;
layout(location = 4) in vec2 fragTexCoord;
layout(location = 5) in vec3 fragWorldPos;

layout(location = 0) out vec4 outAlbedo_Opacity;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outWorldPos;
layout(location = 3) out float outSpecularity;

void main(){
    outAlbedo_Opacity = vec4(1.0, 0.0, 1.0, 1.0);

    outAlbedo_Opacity.rgb = texture(textureSampler, fragTexCoord).rgb;

    outWorldPos.rgb = fragWorldPos.xyz;

    outWorldPos = vec4(fragTexCoord.x,fragTexCoord.y, 0.0f, 0.0f);
}