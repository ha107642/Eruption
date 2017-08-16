#include "Buffer.h"

#include "Graphics.h"

Buffer::Buffer() : size(-1), graphics(nullptr), staging_buffer(VK_NULL_HANDLE), staging_memory(VK_NULL_HANDLE), buffer(VK_NULL_HANDLE), memory(VK_NULL_HANDLE) {}

void Buffer::initialize(Graphics *graphics, VkDeviceSize size) {
	if (this->graphics) 
		destroy();

	this->graphics = graphics;
	this->size = size;
}
Buffer::~Buffer() {
	destroy();
}

void Buffer::create_buffer(VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags, VkBuffer *buffer, VkDeviceMemory *memory) {
	assert(size >= 0);
	assert(graphics != nullptr);

	VkBufferCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	create_info.size = size; //sizeof(vertices[0]) * vertices.size();
	create_info.usage = usage_flags; //VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	create_info.sharingMode = VK_SHARING_MODE_CONCURRENT; //VK_SHARING_MODE_EXCLUSIVE
	create_info.queueFamilyIndexCount = 2;
	uint32_t family_indices[] = { (uint32_t)graphics->graphics_family_index, (uint32_t)graphics->transfer_family_index };
	create_info.pQueueFamilyIndices = family_indices;

	if (vkCreateBuffer(graphics->device, &create_info, nullptr, buffer) != VK_SUCCESS) {
		//Engine::fail("Unable to create vertex buffer");
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(graphics->device, *buffer, &memory_requirements);
	VkMemoryAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize = memory_requirements.size;
	allocate_info.memoryTypeIndex = graphics->get_memory_type_index(memory_requirements.memoryTypeBits, memory_flags); //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT

	if (vkAllocateMemory(graphics->device, &allocate_info, nullptr, memory) != VK_SUCCESS) {
		//Engine::fail("Unable to allocate vertex buffer memory");
	}

	vkBindBufferMemory(graphics->device, *buffer, *memory, 0);
}

void Buffer::initialize_buffer(VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags) {
	destroy_buffer();
	create_buffer(usage_flags, memory_flags, &buffer, &memory);
}

void Buffer::initialize_staging_buffer(VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags) {
	destroy_staging_buffer();
	create_buffer(usage_flags, memory_flags, &staging_buffer, &staging_memory);
}

void Buffer::destroy() {
	destroy_staging_buffer();
	destroy_buffer();
}

void Buffer::destroy_buffer() {
	if (buffer) vkDestroyBuffer(graphics->device, buffer, nullptr);
	if (memory) vkFreeMemory(graphics->device, memory, nullptr);
	buffer = VK_NULL_HANDLE;
	memory = VK_NULL_HANDLE;
}

void Buffer::destroy_staging_buffer() {
	if (staging_buffer) vkDestroyBuffer(graphics->device, staging_buffer, nullptr);
	if (staging_memory) vkFreeMemory(graphics->device, staging_memory, nullptr);
	staging_buffer = VK_NULL_HANDLE;
	staging_memory = VK_NULL_HANDLE;
}

void Buffer::fill_staging_buffer(void* data, VkDeviceSize data_size, VkDeviceSize offset) {
	assert(staging_buffer != VK_NULL_HANDLE);
	void* gpu_buffer;
	vkMapMemory(graphics->device, staging_memory, offset, data_size, 0, &gpu_buffer);
	memcpy(gpu_buffer, data, (size_t)data_size);
	vkUnmapMemory(graphics->device, staging_memory);
}

void Buffer::copy_staging_data(VkCommandBuffer command_buffer) {
	assert(staging_buffer != VK_NULL_HANDLE);
	assert(buffer != VK_NULL_HANDLE);

	bool init_buffer = command_buffer == VK_NULL_HANDLE;
	if (init_buffer) command_buffer = graphics->begin_command_buffer(graphics->transfer_command_pool);

	VkBufferCopy region = {};
	region.size = size;
	vkCmdCopyBuffer(command_buffer, staging_buffer, buffer, 1, &region);

	if (init_buffer) graphics->end_command_buffer(command_buffer);
}