#include "Model.h"

#include "Engine.h"
#include "tiny_obj_loader.h"
#include "io.h"

#include <unordered_map>
#include <fstream>

Model::Model() : texture_descriptor_set(VK_NULL_HANDLE), device(VK_NULL_HANDLE), texture_memory(VK_NULL_HANDLE), texture_image(VK_NULL_HANDLE), texture_image_view(VK_NULL_HANDLE) {}

Model::~Model() {
	destroy();
}

void Model::destroy() {
	if (texture_image_view) vkDestroyImageView(device, texture_image_view, nullptr);
	if (texture_image) vkDestroyImage(device, texture_image, nullptr);
	if (texture_memory) vkFreeMemory(device, texture_memory, nullptr);
}

//byte order!
void Model::save_binary(const char * file_name) {

	std::ofstream file;
	file.open(file_name, std::ios::out | std::ios::binary | std::ios::trunc);

	uint32_t version = 0;
	uint32_t file_type = 0;
	write<uint32_t>(file, version);
	write<uint32_t>(file, file_type);

	write_vector<Vertex>(file, vertices);
	write_vector<uint32_t>(file, indices);
	file.close();
}

//byte order!
Model Model::load_binary(const char * file_name) {
	Model model = Model();

	std::ifstream file;
	file.open(file_name, std::ios::in | std::ios::binary);

	uint32_t version, file_type;
	read<uint32_t>(file, &version);
	read<uint32_t>(file, &file_type);
	if (version == 0 && file_type == 0) {
		read_vector<Vertex>(file, model.vertices);
		read_vector<uint32_t>(file, model.indices);

	} else {
		file.close();
		Engine::fail("Unable to load model. Unrecognized version and/or file type");
	}
	file.close();
	return model;
}

Model Model::load_wireframe(glm::vec3 position, glm::vec3 half_size) {
	Model model = Model();
	model.vertex_offset = 0;
	model.index_offset = 0;
	
	model.vertices.resize(8);
	
	model.vertices[0].position = position + half_size;
	
	model.vertices[1].position = position + half_size;
	model.vertices[1].position.x = model.vertices[1].position.x - half_size.x * 2;

	model.vertices[2].position = position + half_size;
	model.vertices[2].position.y = model.vertices[1].position.y - half_size.y * 2;

	model.vertices[3].position = position + half_size;
	model.vertices[3].position.z = model.vertices[1].position.z - half_size.z * 2;

	model.vertices[4].position = position - half_size;

	model.vertices[5].position = position - half_size;
	model.vertices[5].position.x = model.vertices[1].position.x + half_size.x * 2;

	model.vertices[6].position = position - half_size;
	model.vertices[6].position.y = model.vertices[1].position.y + half_size.y * 2;

	model.vertices[7].position = position - half_size;
	model.vertices[7].position.z = model.vertices[1].position.z + half_size.z * 2;

	model.indices.resize(36);

	//Draw the top.
	model.indices[0]  = 0;
	model.indices[1]  = 1;
	model.indices[2]  = 3;
	model.indices[3]  = 6;
	model.indices[4]  = 3;
	model.indices[5]  = 1;
	
	//Front
	model.indices[6]  = 3;
	model.indices[7]  = 6;
	model.indices[8]  = 5;
	model.indices[9]  = 4;
	model.indices[10] = 5;
	model.indices[11] = 6;

	//Back
	model.indices[12] = 0;
	model.indices[13] = 2;
	model.indices[14] = 1;
	model.indices[15] = 7;
	model.indices[16] = 1;
	model.indices[17] = 2;

	//Right
	model.indices[18] = 0;
	model.indices[19] = 3;
	model.indices[20] = 2;
	model.indices[21] = 5;
	model.indices[22] = 2;
	model.indices[23] = 3;

	//Left
	model.indices[24] = 1;
	model.indices[25] = 7;
	model.indices[26] = 6;
	model.indices[27] = 4;
	model.indices[28] = 6;
	model.indices[29] = 7;

	//Below
	model.indices[30] = 2;
	model.indices[31] = 7;
	model.indices[32] = 5;
	model.indices[33] = 4;
	model.indices[34] = 5;
	model.indices[35] = 7;

	return model;
}

Model Model::load_obj(const char * file_name) {
	tinyobj::attrib_t attributes;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string error;

	std::unordered_map<Vertex, int> vertex_map = {};

	if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &error, file_name)) {
		Engine::fail(error.c_str());
	}

	Model model = Model();

	for (const tinyobj::shape_t& shape : shapes) {
		for (const tinyobj::index_t& index : shape.mesh.indices) {
			Vertex vertex = {};

			vertex.position = {
				attributes.vertices[3 * index.vertex_index + 0],
				attributes.vertices[3 * index.vertex_index + 1],
				attributes.vertices[3 * index.vertex_index + 2]
			};

			if (index.texcoord_index >= 0) {
				vertex.texture_coordinates = {
					attributes.texcoords[2 * index.texcoord_index + 0],
					1.0f - attributes.texcoords[2 * index.texcoord_index + 1]
				};
			}

			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (vertex_map.find(vertex) == vertex_map.end()) {
				vertex_map[vertex] = model.vertices.size();
				model.vertices.push_back(vertex);
			}

			model.indices.push_back(vertex_map[vertex]);
		}
	}
	
	return model;
}