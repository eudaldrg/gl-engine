#version 450 core

// Lit meshes. The model matrix holds translation and uniform scale only, so its upper 3x3
// transforms normals correctly.

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

layout(location = 0) uniform mat4 u_model;
layout(location = 1) uniform mat4 u_view_projection;

out vec3 v_world_position;
out vec3 v_normal;

void main() {
    vec4 world = u_model * vec4(in_position, 1.0);
    v_world_position = world.xyz;
    v_normal = mat3(u_model) * in_normal;
    gl_Position = u_view_projection * world;
}
