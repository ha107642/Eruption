#pragma once

#include <vulkan\vulkan.h>

class Graphics;

class Buffer {
private:
	Graphics *graphics;

	void create_buffer(VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags, VkBuffer *buffer, VkDeviceMemory *memory);
public:
	VkDeviceSize size;
	VkBuffer staging_buffer;
	VkDeviceMemory staging_memory;

	VkBuffer buffer;
	VkDeviceMemory memory;

	void initialize(Graphics *graphics, VkDeviceSize size);
	void initialize_buffer(VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags);
	void initialize_staging_buffer(VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags);
	void destroy();
	void destroy_buffer();
	void destroy_staging_buffer();
	void fill_staging_buffer(void* data, VkDeviceSize data_size, VkDeviceSize offset = 0);
	void copy_staging_data(VkCommandBuffer command_buffer = VK_NULL_HANDLE);
	Buffer();
	//Buffer(Graphics *graphics, VkDeviceSize size);
	~Buffer();
};