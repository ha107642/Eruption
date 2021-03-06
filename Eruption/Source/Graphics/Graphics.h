#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string>
#include <glm/glm.hpp>

#include "Vertex.h"
#include "Model.h"
#include "Time.h"
#include "Buffer.h"
#include "Systems/Camera.h"

class Engine;

struct Model_View_Projection {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

class Graphics {
private:
	friend class Buffer;

	bool _is_initialized;

	std::vector<Model*> models; //TODO: Make contiguous (if needed?).
	std::vector<Model*> model_instances;
	Entity camera_entity;

	VkExtent2D extent;
	int graphics_family_index;
	int window_family_index;
	int transfer_family_index;
	uint32_t image_count;

	struct {
		VkDescriptorSetLayout scene;
		VkDescriptorSetLayout model;
	} descriptor_set_layouts;

	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical_device;
	VkPhysicalDeviceProperties properties;
	VkDevice device;
	VkSwapchainKHR swap_chain;
	std::vector<VkImage> images;
	std::vector<VkImageView> image_views;
	VkRenderPass imgui_render_pass;
	VkRenderPass render_pass;
	//VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorSet descriptor_set;
	VkDescriptorPool descriptor_pool;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
	std::vector<VkFramebuffer> frame_buffers;
	VkCommandPool graphics_command_pool;
	VkCommandPool transfer_command_pool;

	VkImage texture_image;
	VkDeviceMemory texture_image_memory;
	VkImageView texture_image_view;
	VkSampler texture_sampler;

	VkImage depth_image;
	VkDeviceMemory depth_image_memory;
	VkImageView depth_image_view;

	Buffer vertex_buffer;
	Buffer index_buffer;
	Buffer uniform_buffer;
	Buffer dynamic_buffer;

	//VkDeviceSize dynamic_buffer_size;

	std::vector<VkCommandBuffer> command_buffers;

	VkSemaphore image_semaphore;
	VkSemaphore presentation_semaphore;

	VkSubmitInfo graphics_submit_info;

	VkSurfaceCapabilitiesKHR surface_capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;

	VkSurfaceFormatKHR surface_format;

	VkQueue graphics_queue;
	VkQueue window_queue;
	VkQueue transfer_queue;

	VkDebugReportCallbackEXT debug_callback;

	void get_validation_layers(std::vector<const char*>* layers);
	int get_memory_type_index(uint32_t filter, VkMemoryPropertyFlags properties);
	void get_extensions(std::vector<const char*>* extensions);
	bool are_extensions_supported(VkPhysicalDevice device);
	bool is_surface_supported(VkPhysicalDevice device);
	//void load_model();

	void initialize_pipeline();
	void initialize_swap_chain();
	void initialize_swap_chain_image_views();
	void initialize_render_pass();
	void initialize_descriptor_set_layout();
	void initialize_texture_image();
	void initialize_command_pools();
	void initialize_depth_image();
	void initialize_frame_buffers();
	void initalize_command_buffers();
	void update_buffer_descriptor_sets();
	void initialize_texture_descriptor(Model* model);
	void initialize_sampler();
	void initialize_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, VkImageView * image_view);
	void reinitialize_swap_chain();

	VkCommandBuffer begin_command_buffer(VkCommandPool command_pool);
	void end_command_buffer(VkCommandBuffer buffer);
	void end_command_buffer(VkCommandBuffer buffer, VkQueue queue, VkCommandPool command_pool);
	void set_barrier_access_masks(VkAccessFlags* source_mask, VkAccessFlags* destination_mask, VkPipelineStageFlags* srcStageMask, VkPipelineStageFlags* dstStageMask, VkImageLayout old_layout, VkImageLayout new_layout);
	void set_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, VkCommandBuffer buffer = VK_NULL_HANDLE);
	void copy_image(VkImage source, VkImage destination, VkExtent2D extent, VkCommandBuffer buffer = VK_NULL_HANDLE);
	void copy_buffer(VkBuffer source, VkBuffer destination, VkDeviceSize size, VkCommandBuffer buffer = VK_NULL_HANDLE);
	void create_image(VkExtent2D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage * image, VkDeviceMemory * memory);
	VkFormat get_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat get_depth_format();
public:
	int dynamic_buffer_alignment;
	GLFWwindow* window;

	Graphics();
	~Graphics();
	bool is_initialized() { return _is_initialized; }
	void initialize_imgui();
	void initialize(Engine *engine);
	void initialize_buffers();
	bool is_device_compatible(VkPhysicalDevice device);
	bool should_exit();
	void resize(int width, int height);
	void update_command_buffer(int i);
	void resize_dynamic_buffer(VkDeviceSize size);
	void update_dynamic_buffer(void * data, VkDeviceSize size);
	Model* load_model(const char* model_name);
	void load_texture(Model* model, const char * texture_name);
	void set_model_instances(const std::vector<Model*> model_instances) { this->model_instances = model_instances; }

	void set_main_camera(const Entity camera_entity);
	void draw(Time& time);
};