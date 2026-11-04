#version 450 core

// Blinn-Phong with one directional light.

in vec3 v_world_position;
in vec3 v_normal;

layout(location = 2) uniform vec3 u_camera_position;
layout(location = 3) uniform vec3 u_light_direction;  // direction the light travels
layout(location = 4) uniform vec3 u_color;

layout(location = 0) out vec4 out_color;

const float kAmbient = 0.15;
const float kSpecular = 0.4;
const float kShininess = 64.0;

void main() {
    vec3 n = normalize(v_normal);
    vec3 l = normalize(-u_light_direction);
    vec3 v = normalize(u_camera_position - v_world_position);
    vec3 h = normalize(l + v);

    float diffuse = max(dot(n, l), 0.0);
    float specular = diffuse > 0.0 ? pow(max(dot(n, h), 0.0), kShininess) : 0.0;

    vec3 color = (kAmbient + diffuse) * u_color + kSpecular * specular;
    out_color = vec4(color, 1.0);
}
