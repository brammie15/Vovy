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


layout(std140, set = 1, binding = 0) uniform bindingInfo{
    bool hasAlbedo;
    bool hasNormal;
    bool hasSpecular;
    bool hasBump;
} textureBindingInfo;

layout(set = 1, binding = 1) uniform sampler2D albedoSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;
layout(set = 1, binding = 3) uniform sampler2D specularSampler;
layout(set = 1, binding = 4) uniform sampler2D bumpSampler;

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitTangent;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outWorldPos;
layout(location = 3) out float outSpecularity;

vec3 boolsToColor(bool hasNormal, bool hasSpecular, bool hasBump) {

    float r = hasNormal ? 1.0 : 0.0;
    float g = hasSpecular ? 1.0 : 0.0;
    float b = hasBump ? 1.0 : 0.0;

    r = hasNormal ? 0.8 : 0.2;
    g = hasSpecular ? 0.8 : 0.2;
    b = hasBump ? 0.8 : 0.2;

    return vec3(r, g, b);
}

void main(){
    if(textureBindingInfo.hasAlbedo){
        outColor.rgb = inColor * texture(albedoSampler, inTexCoord).rgb;
    }

    vec3 normal = normalize(inNormal);
    vec3 tangent = normalize(inTangent);
    vec3 bitTangent = normalize(inBitTangent);
    outNormal = vec4(normal * 0.5 + 0.5, 1.0f);

    if(textureBindingInfo.hasBump){
        vec2 texelSize = 1.0 / textureSize(bumpSampler, 0);
        mat3x3 tbn = mat3x3(tangent, bitTangent, normal);
        float scale = 1.0f;

        float height = texture(bumpSampler, inTexCoord).r;
        float deltaX = -scale * (height - texture(bumpSampler, inTexCoord + vec2(texelSize.x, 0.0)).r);
        float deltaY = -scale * (height - texture(bumpSampler, inTexCoord + vec2(0.0, texelSize.y)).r);
        vec3 bumpNormal = normalize(tbn * vec3(deltaX, deltaY, 1));
        outNormal = vec4(bumpNormal * 0.5 + 0.5, 1.0f);
    }

    outSpecularity.r = 0.0f;
    if(textureBindingInfo.hasSpecular){
        outSpecularity = texture(specularSampler, inTexCoord).r;
    }

    outWorldPos.rgb = inWorldPos.xyz;
}