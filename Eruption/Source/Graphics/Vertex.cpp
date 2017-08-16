#include "Vertex.h"

std::vector<VkVertexInputAttributeDescription> Vertex::get_attribute_descriptions() {
	std::vector<VkVertexInputAttributeDescription> attribute_descriptions(3);

	attribute_descriptions[0].binding = 0;
	attribute_descriptions[0].location = 0;
	attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribute_descriptions[0].offset = offsetof(Vertex, position);

	attribute_descriptions[1].binding = 0;
	attribute_descriptions[1].location = 1;
	attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribute_descriptions[1].offset = offsetof(Vertex, color);

	attribute_descriptions[2].binding = 0;
	attribute_descriptions[2].location = 2;
	attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attribute_descriptions[2].offset = offsetof(Vertex, texture_coordinates);

	return attribute_descriptions;
}

VkVertexInputBindingDescription Vertex::get_binding_description() {
	VkVertexInputBindingDescription binding_description = {};
	binding_description.binding = 0;
	binding_description.stride = sizeof(Vertex);
	binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return binding_description;
}

bool Vertex::operator==(const Vertex & other) const {
	return position == other.position && color == other.color && texture_coordinates == other.texture_coordinates;
}
