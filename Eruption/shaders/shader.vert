#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform Camera_Matrix {
    mat4 projection;
} camera;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texture_coordinates;

layout(location = 3) in mat4 model;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_texture_coordinates;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = camera.projection * model * vec4(position, 1.0);
    frag_color = color;
	frag_texture_coordinates = texture_coordinates;
}
