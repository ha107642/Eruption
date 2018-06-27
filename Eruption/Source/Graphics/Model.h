#pragma once

#include "Vertex.h"

#include <vector>
#include <string>

class Model {
public:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::string name;
	
	int32_t  vertex_offset;
	uint32_t index_offset;

	VkDescriptorSet texture_descriptor_set;
	VkDevice device;
	VkDeviceMemory texture_memory;
	VkImage texture_image;
	VkImageView texture_image_view;
	
	Model();
	~Model();
	void destroy();

	void save_binary(const char* file_name);

	static Model load_obj(const char* file_name);
	static Model load_binary(const char* file_name);
	static Model load_wireframe(glm::vec3 position, glm::vec3 half_size);
};