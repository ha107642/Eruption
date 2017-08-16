#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_texture_coordinates;

layout(location = 0) out vec4 out_color;

void main() {
    //out_color = vec4(frag_texture_coordinates, 0.0, 1.0);
	out_color = texture(texture_sampler, frag_texture_coordinates);
}
