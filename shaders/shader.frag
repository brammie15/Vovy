#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inNormal;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 view;
    mat4 projection;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

void main() {
//    outColor = vec4(push.color, 1.0f);
//    outColor = texture(texSampler, vec2(inTexCoord.x, inTexCoord.y));
    vec3 ambient = vec3(0.2f, 0.2f, 0.2f);
    vec3 lightDir = normalize(vec3(1.0f, -1.0f, -1.0f));

    vec3 normal = normalize(inNormal);
    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuseColor = texture(texSampler, inTexCoord).rgb;

    vec3 result = (ambient + diffuseColor * diff) * diffuseColor;

    outColor = vec4(result, 1.0f);
}