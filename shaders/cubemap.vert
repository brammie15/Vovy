#version 450

// Hardcoded cube vertices (36 vertices for 12 triangles)
const vec3 positions[36] = vec3[](
// Front face
vec3(-1.0, -1.0,  1.0), vec3( 1.0, -1.0,  1.0), vec3( 1.0,  1.0,  1.0),
vec3( 1.0,  1.0,  1.0), vec3(-1.0,  1.0,  1.0), vec3(-1.0, -1.0,  1.0),
// Back face
vec3(-1.0, -1.0, -1.0), vec3(-1.0,  1.0, -1.0), vec3( 1.0,  1.0, -1.0),
vec3( 1.0,  1.0, -1.0), vec3( 1.0, -1.0, -1.0), vec3(-1.0, -1.0, -1.0),
// Top face
vec3(-1.0,  1.0, -1.0), vec3(-1.0,  1.0,  1.0), vec3( 1.0,  1.0,  1.0),
vec3( 1.0,  1.0,  1.0), vec3( 1.0,  1.0, -1.0), vec3(-1.0,  1.0, -1.0),
// Bottom face
vec3(-1.0, -1.0, -1.0), vec3( 1.0, -1.0, -1.0), vec3( 1.0, -1.0,  1.0),
vec3( 1.0, -1.0,  1.0), vec3(-1.0, -1.0,  1.0), vec3(-1.0, -1.0, -1.0),
// Right face
vec3( 1.0, -1.0, -1.0), vec3( 1.0,  1.0, -1.0), vec3( 1.0,  1.0,  1.0),
vec3( 1.0,  1.0,  1.0), vec3( 1.0, -1.0,  1.0), vec3( 1.0, -1.0, -1.0),
// Left face
vec3(-1.0, -1.0, -1.0), vec3(-1.0, -1.0,  1.0), vec3(-1.0,  1.0,  1.0),
vec3(-1.0,  1.0,  1.0), vec3(-1.0,  1.0, -1.0), vec3(-1.0, -1.0, -1.0)
);

// Hardcoded UVs aren't needed for cube map rendering
layout(location = 0) out vec3 localPos;

// Push constants for view and projection matrices
layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 proj;
} pc;

void main() {
    // Get hardcoded vertex position
    vec3 pos = positions[gl_VertexIndex];
    localPos = pos; // Pass to fragment shader

    // Apply view and projection matrices
    gl_Position = pc.proj * pc.view * vec4(pos, 1.0);
}