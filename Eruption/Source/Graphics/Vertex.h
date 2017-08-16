#pragma once

#include <glm/glm.hpp>
#include <vulkan\vulkan.h>
#include <vector>
#include <glm/gtx/hash.hpp>

class Vertex {
public:
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texture_coordinates;

	static VkVertexInputBindingDescription get_binding_description();
	static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions();
	bool operator==(const Vertex& other) const;
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texture_coordinates) << 1);
		}
	};
}