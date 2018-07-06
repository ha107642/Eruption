#include "Graphics.h"
#include "io.h"

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image.h"
//#include "tiny_obj_loader.h"
#include "Engine.h"

#include <unordered_map>
#include <set>

#define VALIDATE_VULKAN

//Forward declarations.
static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
VkResult create_debug_report_callback(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);
void destroy_debug_report_callback(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);

std::vector<const char*> device_extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

void check_vk_result(VkResult err) {
	if (err == 0) return;
	printf("VkResult %d\n", err);
	if (err < 0)
		abort();
}

Graphics::Graphics() {
	extent = { 1024, 768 };
	image_count = 2;

	window = nullptr;
	camera = nullptr;
	instance = VK_NULL_HANDLE;
	surface = VK_NULL_HANDLE;
	physical_device = VK_NULL_HANDLE;
	device = VK_NULL_HANDLE;
	swap_chain = VK_NULL_HANDLE;
	render_pass = VK_NULL_HANDLE;
	descriptor_set = VK_NULL_HANDLE;
	//descriptor_set_layout = VK_NULL_HANDLE;
	descriptor_set_layouts.scene = VK_NULL_HANDLE;
	descriptor_set_layouts.model = VK_NULL_HANDLE;
	pipeline_layout = VK_NULL_HANDLE;
	pipeline = VK_NULL_HANDLE;
	descriptor_pool = VK_NULL_HANDLE;
	graphics_command_pool = VK_NULL_HANDLE;
	transfer_command_pool = VK_NULL_HANDLE;
	texture_image = VK_NULL_HANDLE;
	texture_image_memory = VK_NULL_HANDLE;
	texture_image_view = VK_NULL_HANDLE;
	texture_sampler = VK_NULL_HANDLE;
	depth_image = VK_NULL_HANDLE;
	depth_image_memory = VK_NULL_HANDLE;
	depth_image_view = VK_NULL_HANDLE;

	image_semaphore = VK_NULL_HANDLE;
	presentation_semaphore = VK_NULL_HANDLE;

	graphics_queue = VK_NULL_HANDLE;
	window_queue = VK_NULL_HANDLE;

	_is_initialized = false;
	//	is_buffer_initialized = false;
}


void Graphics::initialize(Engine *engine) {
	if (_is_initialized) return;

	{	//Init window
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		window = glfwCreateWindow(extent.width, extent.height, "Eruption", nullptr, nullptr);
		glfwSetWindowUserPointer(window, engine);
	}

	{	//Init Vulkan instance
		VkApplicationInfo app_info = {};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.apiVersion = VK_API_VERSION_1_0;
		app_info.pApplicationName = "Vulkan Test";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "Eruption";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);


		std::vector<const char*> layers, extensions;
		get_validation_layers(&layers);
		get_extensions(&extensions);

		VkInstanceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;
		create_info.enabledExtensionCount = extensions.size();
		create_info.ppEnabledExtensionNames = extensions.data();
		create_info.enabledLayerCount = layers.size();
		create_info.ppEnabledLayerNames = layers.data();

		if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
			Engine::fail("Unable to create Vulkan instance");
		}
	}

	{	//Setup window surface
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			Engine::fail("Unable to create window GLFW window surface");
		}
	}

#ifdef VALIDATE_VULKAN
	{	//Setup debug callback
		VkDebugReportCallbackCreateInfoEXT create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT /*| VK_DEBUG_REPORT_INFORMATION_BIT_EXT*/;
		create_info.pfnCallback = vulkan_debug_callback;
		create_info.pUserData = this;

		if (create_debug_report_callback(instance, &create_info, nullptr, &debug_callback) != VK_SUCCESS) {
			Engine::fail("Unable to create Vulkan debug callback");
		}
	}
#endif // VALIDATE_VULKAN

	{	//Setup physical device
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

		std::vector<VkPhysicalDevice> devices(device_count);
		vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
		for (uint32_t i = 0; i < device_count; ++i) {
			if (is_device_compatible(devices[i]))
				physical_device = devices[i];
			break;
		}
		if (physical_device == VK_NULL_HANDLE)
			Engine::fail("Unable to find suitable physical graphics device");
	}

	{	//Setup logical device
		std::set<int> queue_family_indices = { graphics_family_index, window_family_index, transfer_family_index };
		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		float queue_priority = 0.5f; //we have only one queue.

		for (int family_index : queue_family_indices) {
			VkDeviceQueueCreateInfo queue_create_info = {};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = family_index;
			queue_create_info.queueCount = 1;
			queue_create_info.pQueuePriorities = &queue_priority;

			queue_create_infos.push_back(queue_create_info);
		}

		VkPhysicalDeviceFeatures features = {};

		std::vector<const char*> layers;
		get_validation_layers(&layers);

		VkDeviceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		create_info.queueCreateInfoCount = (uint32_t)queue_create_infos.size();
		create_info.pQueueCreateInfos = queue_create_infos.data();
		create_info.pEnabledFeatures = &features;
		create_info.enabledLayerCount = layers.size();
		create_info.ppEnabledLayerNames = layers.data();
		create_info.enabledExtensionCount = device_extensions.size();
		create_info.ppEnabledExtensionNames = device_extensions.data();

		if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
			Engine::fail("Unable to create logical device");
		}

		vkGetDeviceQueue(device, graphics_family_index, 0, &graphics_queue);
		vkGetDeviceQueue(device, window_family_index, 0, &window_queue);
		vkGetDeviceQueue(device, transfer_family_index, 0, &transfer_queue);
	}

	initialize_swap_chain();
	initialize_swap_chain_image_views();
	initialize_render_pass();
	initialize_descriptor_set_layout();
	initialize_pipeline();
	initialize_command_pools();
	initialize_depth_image();
	initialize_frame_buffers();
	initialize_texture_image();
	initialize_image_view(texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &texture_image_view);
	initialize_sampler();

	initalize_command_buffers();

	{
		int unit_size = sizeof(Transform_Matrix);
		int alignment = (int)properties.limits.minUniformBufferOffsetAlignment;
		dynamic_buffer_alignment = (unit_size / alignment) * alignment;
		if (unit_size % alignment != 0)					//If we are not perfectly aligned already,
			dynamic_buffer_alignment += alignment;	//Add padding to fit the end.
	}

	//if (models.size() > 0) {
	//	initialize_buffers();
	//	//resize_dynamic_buffer((VkDeviceSize)dynamic_buffer_alignment);
	//}
	//update_descriptor_sets();

	{
		VkSemaphoreCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(device, &create_info, nullptr, &image_semaphore) != VK_SUCCESS ||
			vkCreateSemaphore(device, &create_info, nullptr, &presentation_semaphore) != VK_SUCCESS) {
			Engine::fail("Unable to create semaphores");
		}
	}

	//mvp = {};
	//mvp.model = glm::rotate(glm::mat4(), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	//mvp.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	//mvp.projection = glm::perspective(glm::quarter_pi<float>(), (float)extent.width / (float)extent.height, 0.1f, 100.0f);
	//mvp.projection[1][1] *= -1; //OpenGL inverts Y clip coordinate, which Vulkan does not. Rectify this so that the image is not upside down.

	_is_initialized = true;
}

Model* Graphics::load_model(const char * model_name) {
	for (Model* loaded_model : models) {
		if (strcmp(loaded_model->name.c_str(), model_name) == 0)
			return loaded_model;
	}

	const char* dir = "models/";
	int surrounding_length = strlen(dir) + 4 + 1; //directory + extension + \0

	int string_length = strlen(model_name) + surrounding_length;
	char* bin_name = new char[string_length];
	char* obj_name = new char[string_length];
	snprintf(bin_name, string_length, "%s%s.bin", dir, model_name);
	snprintf(obj_name, string_length, "%s%s.obj", dir, model_name);

	Model* model;
	if (file_exists(bin_name)) {
		model = new Model(Model::load_binary(bin_name));
	} else {
		model = new Model(Model::load_obj(obj_name));
		model->save_binary(bin_name);
	}

	delete[] bin_name;
	delete[] obj_name;
	model->name = model_name;
	models.push_back(model);
	return model;
}

void Graphics::initialize_command_pools() {
	if (graphics_command_pool) vkDestroyCommandPool(device, graphics_command_pool, nullptr);
	if (transfer_command_pool) vkDestroyCommandPool(device, transfer_command_pool, nullptr);

	{
		VkCommandPoolCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		create_info.queueFamilyIndex = graphics_family_index;
		create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(device, &create_info, nullptr, &graphics_command_pool) != VK_SUCCESS) {
			Engine::fail("Unable to create graphics command pool");
		}
	}

	{
		VkCommandPoolCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		create_info.queueFamilyIndex = transfer_family_index;
		create_info.flags = 0;

		if (vkCreateCommandPool(device, &create_info, nullptr, &transfer_command_pool) != VK_SUCCESS) {
			Engine::fail("Unable to create transfer command pool");
		}
	}
}

void Graphics::initialize_depth_image() {
	VkFormat depth_format = get_depth_format();

	create_image(extent, depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depth_image, &depth_image_memory);
	initialize_image_view(depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, &depth_image_view);

	{
		//HA 2017-06-04: set_image_layout always assumes you want transfer_command_pool and transfer_queue.
		//This worked absolutely fine until about one week ago, when I couldn't run the game any more.
		//I do not understand why it suddenly stopped working (graphics drivers update?), but now I need to
		//use the graphics queue for the depth image. Maybe that's what the spec says as well, I don't know.
		//Anyway, using the graphics queue, it seems to work again. I replaced what is inside set_image_layout 
		//with its contents here below.

		//set_image_layout(depth_image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		VkCommandBuffer buffer = begin_command_buffer(graphics_command_pool);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = depth_image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		VkPipelineStageFlags srcStageMask;
		VkPipelineStageFlags dstStageMask;
		set_barrier_access_masks(&barrier.srcAccessMask, &barrier.dstAccessMask, &srcStageMask, &dstStageMask, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		vkCmdPipelineBarrier(buffer, srcStageMask, dstStageMask, 0,
			0, nullptr, 0, nullptr, 1, &barrier);

		end_command_buffer(buffer, graphics_queue, graphics_command_pool);
	}
}

void Graphics::initialize_sampler() {
	if (texture_sampler) vkDestroySampler(device, texture_sampler, nullptr);
	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;

	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; //Irrelevant for repeat.

	sampler_info.anisotropyEnable = VK_FALSE;
	//sampler_info.anisotropyEnable = VK_TRUE;				 //Anisotropic filtering
	sampler_info.maxAnisotropy = 16;

	sampler_info.unnormalizedCoordinates = VK_FALSE; //Coordinates are normalized.

	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;

	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;

	if (vkCreateSampler(device, &sampler_info, nullptr, &texture_sampler) != VK_SUCCESS) {
		Engine::fail("Unable to create texture sampler");
	}
}

void Graphics::initialize_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, VkImageView* image_view) {
	if (*image_view != VK_NULL_HANDLE) vkDestroyImageView(device, *image_view, nullptr);
	VkImageViewCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.image = image;
	create_info.viewType = VK_IMAGE_VIEW_TYPE_2D; //2D for now..
	create_info.format = format;
	create_info.subresourceRange.aspectMask = aspect_flags;
	create_info.subresourceRange.baseMipLevel = 0;
	create_info.subresourceRange.levelCount = 1;
	create_info.subresourceRange.baseArrayLayer = 0;
	create_info.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device, &create_info, nullptr, image_view) != VK_SUCCESS) {
		Engine::fail("Unable to create image view");
	}
}

void Graphics::initialize_swap_chain_image_views() {
	image_views.resize(image_count);
	for (uint32_t i = 0; i < image_count; ++i) {
		initialize_image_view(images[i], surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT, &image_views[i]);
	}
}

//Refactor...
void Graphics::load_texture(Model* model, const char* texture_name) {
	if (model->texture_image) {
		printf("WARNING: Try to load a texture to a model that already has one.\n");
		return;
	}

	int texture_channels;
	VkExtent2D texture_extent;

	const char* dir = "textures/";
	int surrounding_length = strlen(dir) + 4 + 1; //directory + extension + \0

	int string_length = strlen(texture_name) + surrounding_length;
	std::vector<char*> extensions = { "jpg", "png" };
	stbi_uc* pixels = nullptr;
	for (int i = 0; i < extensions.size(); ++i) {
		char* file_name = new char[string_length];
		snprintf(file_name, string_length, "%s%s.%s", dir, texture_name, extensions[i]);
		pixels = stbi_load(file_name, (int*)&texture_extent.width, (int*)&texture_extent.height, &texture_channels, STBI_rgb_alpha);
		delete[] file_name;
		if (pixels) break;
	}

	if (!pixels) {
		Engine::fail("Unable to load texture image");
	}

	VkDeviceSize image_size = texture_extent.width * texture_extent.height * 4;

	VkImage staging_image = VK_NULL_HANDLE;
	VkDeviceMemory staging_image_memory = VK_NULL_HANDLE;
	create_image(texture_extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_image, &staging_image_memory);

	VkImageSubresource sub_resource = {};
	sub_resource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	sub_resource.mipLevel = 0;
	sub_resource.arrayLayer = 0;

	VkSubresourceLayout staging_image_layout;
	vkGetImageSubresourceLayout(device, staging_image, &sub_resource, &staging_image_layout);

	void* data;
	if (vkMapMemory(device, staging_image_memory, 0, image_size, 0, &data) != VK_SUCCESS) {
		Engine::fail("Unable to map memory");
	}

	int row_size = texture_extent.width * 4;
	if (staging_image_layout.rowPitch == row_size) { //If there is no extra padding per row
		memcpy(data, pixels, (size_t)image_size);
	} else { //We need to copy row by row.
		uint8_t* bytes = (uint8_t*)data;
		for (uint32_t y = 0; y < texture_extent.height; y++) {
			uint8_t* row_start_destination = &bytes[y * staging_image_layout.rowPitch];
			uint8_t* row_start_source = &pixels[y * row_size];
			memcpy(row_start_destination, row_start_source, row_size);
		}
	}

	vkUnmapMemory(device, staging_image_memory);

	model->device = device;

	create_image(texture_extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &model->texture_image, &model->texture_memory);

	VkCommandBuffer command_buffer = begin_command_buffer(transfer_command_pool);
	set_image_layout(staging_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, command_buffer);
	set_image_layout(model->texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, command_buffer);
	copy_image(staging_image, model->texture_image, texture_extent, command_buffer);
	set_image_layout(model->texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, command_buffer);
	end_command_buffer(command_buffer);

	//BUG! FIX!
	//BUG! FIX!
	//TODO: We need to do some synchronization if this is called in the middle of execution. This may crash the program otherwise.
	//BUG! FIX!
	//BUG! FIX!
	initialize_image_view(model->texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &model->texture_image_view);
	//initialize_texture_descriptor(model);

	stbi_image_free(pixels);
	vkDestroyImage(device, staging_image, nullptr);
	vkFreeMemory(device, staging_image_memory, nullptr);

	if (vertex_buffer.buffer != VK_NULL_HANDLE || index_buffer.buffer != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(device); //causes lag...
		initialize_buffers();
		update_buffer_descriptor_sets();
	}
}

void Graphics::initialize_texture_image() {
	int texture_channels;
	VkExtent2D texture_extent;
	stbi_uc* pixels = stbi_load("textures/chalet.jpg", (int*)&texture_extent.width, (int*)&texture_extent.height, &texture_channels, STBI_rgb_alpha);

	VkDeviceSize image_size = texture_extent.width * texture_extent.height * 4;

	if (!pixels) {
		Engine::fail("Unable to load texture image");
	}
	VkImage staging_image = VK_NULL_HANDLE;
	VkDeviceMemory staging_image_memory = VK_NULL_HANDLE;
	create_image(texture_extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_image, &staging_image_memory);

	VkImageSubresource sub_resource = {};
	sub_resource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	sub_resource.mipLevel = 0;
	sub_resource.arrayLayer = 0;

	VkSubresourceLayout staging_image_layout;
	vkGetImageSubresourceLayout(device, staging_image, &sub_resource, &staging_image_layout);

	void* data;
	if (vkMapMemory(device, staging_image_memory, 0, image_size, 0, &data) != VK_SUCCESS) {
		Engine::fail("Unable to map memory");
	}

	int row_size = texture_extent.width * 4;
	if (staging_image_layout.rowPitch == row_size) { //If there is no extra padding per row
		memcpy(data, pixels, (size_t)image_size);
	} else { //We need to copy row by row.
		uint8_t* bytes = (uint8_t*)data;
		for (uint32_t y = 0; y < texture_extent.height; y++) {
			uint8_t* row_start_destination = &bytes[y * staging_image_layout.rowPitch];
			uint8_t* row_start_source = &pixels[y * row_size];
			memcpy(row_start_destination, row_start_source, row_size);
		}
	}

	vkUnmapMemory(device, staging_image_memory);

	create_image(texture_extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texture_image, &texture_image_memory);

	VkCommandBuffer command_buffer = begin_command_buffer(transfer_command_pool);
	set_image_layout(staging_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, command_buffer);
	set_image_layout(texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, command_buffer);
	copy_image(staging_image, texture_image, texture_extent, command_buffer);
	set_image_layout(texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, command_buffer);
	end_command_buffer(command_buffer);

	stbi_image_free(pixels);
	vkDestroyImage(device, staging_image, nullptr);
	vkFreeMemory(device, staging_image_memory, nullptr);
}


void Graphics::initialize_frame_buffers() {
	for (VkFramebuffer frame_buffer : frame_buffers) if (frame_buffer) vkDestroyFramebuffer(device, frame_buffer, nullptr);

	frame_buffers.resize(image_views.size());
	for (size_t i = 0; i < image_views.size(); ++i) {
		VkImageView attachments[] = { image_views[i], depth_image_view };
		VkFramebufferCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		create_info.renderPass = render_pass;
		create_info.attachmentCount = 2;
		create_info.pAttachments = attachments;
		create_info.width = extent.width;
		create_info.height = extent.height;
		create_info.layers = 1; //Single image swap chains

		if (vkCreateFramebuffer(device, &create_info, nullptr, &frame_buffers[i]) != VK_SUCCESS) {
			Engine::fail("Unable to create frame buffer");
		}
	}
}

void Graphics::initalize_command_buffers() {
	command_buffers.resize(frame_buffers.size());
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandPool = graphics_command_pool;
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; //only single level here.
	allocate_info.commandBufferCount = command_buffers.size();
	//vkResetCommandBuffer
	if (vkAllocateCommandBuffers(device, &allocate_info, command_buffers.data()) != VK_SUCCESS) {
		Engine::fail("Unable to allocate command buffers");
	}
}

void Graphics::update_command_buffer(int i) {
	//"If a command buffer is in the executable state and the command buffer was allocated from a command pool with the 
	//VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag set, then vkBeginCommandBuffer implicitly resets the command 
	//buffer, behaving as if vkResetCommandBuffer had been called with VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT not 
	//set. It then puts the command buffer in the recording state." 
	//-- https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#commandbuffer-allocation
	//vkResetCommandBuffer(command_buffers[i], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT); 

	vkQueueWaitIdle(graphics_queue);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	if (vkBeginCommandBuffer(command_buffers[i], &begin_info) != VK_SUCCESS) {
		Engine::fail("Unable to begin recording command buffer");
	}

	VkRenderPassBeginInfo render_begin_info = {};
	render_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_begin_info.renderPass = render_pass;
	render_begin_info.framebuffer = frame_buffers[i];
	render_begin_info.renderArea.offset = { 0, 0 };
	render_begin_info.renderArea.extent = extent;

	VkClearValue clear_values[2] = {};
	clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clear_values[1].depthStencil = { 1.0f, 0 };
	render_begin_info.clearValueCount = 2;
	render_begin_info.pClearValues = clear_values;

	vkCmdBeginRenderPass(command_buffers[i], &render_begin_info, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	//vkCmdPushConstants(command_buffers[i], pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mvp), &mvp);

	VkBuffer vertex_buffers[] = { vertex_buffer.buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(command_buffers[i], index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	for (uint32_t j = 0; j < model_instances.size(); ++j) {
		uint32_t offset = j * dynamic_buffer_alignment;

		std::vector<VkDescriptorSet> descriptor_sets;
		descriptor_sets.push_back(descriptor_set);
		if (model_instances[j]->texture_descriptor_set != VK_NULL_HANDLE)
			descriptor_sets.push_back(model_instances[j]->texture_descriptor_set);

		vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, descriptor_sets.size(), descriptor_sets.data(), 1, &offset);
		vkCmdDrawIndexed(command_buffers[i], model_instances[j]->indices.size(), 1, model_instances[j]->index_offset, model_instances[j]->vertex_offset, 0);
	}

	ImDrawData* imgui_draw_data = ImGui::GetDrawData();
	if (imgui_draw_data)
		ImGui_ImplVulkan_RenderDrawData(imgui_draw_data, command_buffers[i]);

	vkCmdEndRenderPass(command_buffers[i]);

	if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS) {
		Engine::fail("Unable to end recording command buffer");
	}
}

void Graphics::resize_dynamic_buffer(VkDeviceSize size) {
	assert(descriptor_set != VK_NULL_HANDLE);
	vkDeviceWaitIdle(device);
	dynamic_buffer.initialize(this, size);
	dynamic_buffer.initialize_staging_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	dynamic_buffer.initialize_buffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	update_buffer_descriptor_sets();
}

void Graphics::initialize_buffers() {
	if (descriptor_pool)
		vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
	int model_count = models.size();

	{
		VkDeviceSize buffer_size = 0;
		for (int i = 0; i < model_count; ++i)
			buffer_size += sizeof(models[i]->vertices[0]) * models[i]->vertices.size();

		vertex_buffer.initialize(this, buffer_size);
		vertex_buffer.initialize_staging_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VkDeviceSize offset = 0;
		for (int i = 0; i < model_count; ++i) {
			VkDeviceSize model_size = sizeof(models[i]->vertices[0]) * models[i]->vertices.size();
			vertex_buffer.fill_staging_buffer(models[i]->vertices.data(), model_size, offset);
			models[i]->vertex_offset = offset / sizeof(models[i]->vertices[0]);
			offset += model_size;
		}

		vertex_buffer.initialize_buffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		vertex_buffer.copy_staging_data();
		vertex_buffer.destroy_staging_buffer();
	}

	{
		VkDeviceSize buffer_size = 0;
		for (int i = 0; i < model_count; ++i)
			buffer_size += sizeof(models[i]->indices[0]) * models[i]->indices.size();

		index_buffer.initialize(this, buffer_size);
		index_buffer.initialize_staging_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VkDeviceSize offset = 0;
		for (int i = 0; i < model_count; ++i) {
			VkDeviceSize model_size = sizeof(models[i]->indices[0]) * models[i]->indices.size();
			index_buffer.fill_staging_buffer(models[i]->indices.data(), model_size, offset);
			models[i]->index_offset = offset / sizeof(models[i]->indices[0]);
			offset += model_size;
		}

		index_buffer.initialize_buffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		index_buffer.copy_staging_data();
		index_buffer.destroy_staging_buffer();
	}

	{
		VkDeviceSize buffer_size = sizeof(Model_View_Projection);
		uniform_buffer.initialize(this, buffer_size);
		uniform_buffer.initialize_staging_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		uniform_buffer.initialize_buffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	//{
	//	VkDescriptorPoolSize pool_sizes[3] = {};
	//	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	//	pool_sizes[0].descriptorCount = 1;
	//	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	//	pool_sizes[1].descriptorCount = 1;
	//	pool_sizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//	pool_sizes[2].descriptorCount = models.size();

	//	VkDescriptorPoolCreateInfo create_info = {};
	//	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	//	create_info.poolSizeCount = 3;
	//	create_info.pPoolSizes = pool_sizes;
	//	create_info.maxSets = models.size() + 1;
	//	//create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	//	if (vkCreateDescriptorPool(device, &create_info, nullptr, &descriptor_pool) != VK_SUCCESS) {
	//		Engine::fail("Unable to create descriptor pool");
	//	}
	//}

	{
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};
		VkDescriptorPoolCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		create_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
		create_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
		create_info.pPoolSizes = pool_sizes;
		if (vkCreateDescriptorPool(device, &create_info, nullptr, &descriptor_pool) != VK_SUCCESS) {
			Engine::fail("unable to create descriptor pool");
		}
	}

	{
		VkDescriptorSetLayout layouts[] = { descriptor_set_layouts.scene };
		VkDescriptorSetAllocateInfo allocate_info = {};
		allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocate_info.descriptorPool = descriptor_pool;
		allocate_info.descriptorSetCount = 1;
		allocate_info.pSetLayouts = layouts;

		if (vkAllocateDescriptorSets(device, &allocate_info, &descriptor_set) != VK_SUCCESS) {
			Engine::fail("Unable to create descriptor set");
		}
	}

	for (int i = 0; i < model_count; ++i)
		initialize_texture_descriptor(models[i]);

	//	is_buffer_initialized = true;
}

void Graphics::update_buffer_descriptor_sets() {
	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = uniform_buffer.buffer;
	buffer_info.offset = 0;
	buffer_info.range = sizeof(Model_View_Projection);

	VkDescriptorBufferInfo dynamic_info = {};
	dynamic_info.buffer = dynamic_buffer.buffer;
	dynamic_info.offset = 0;
	dynamic_info.range = sizeof(Transform_Matrix);

	VkWriteDescriptorSet write_sets[2] = {};
	write_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_sets[0].dstSet = descriptor_set;
	write_sets[0].dstBinding = 0;
	write_sets[0].dstArrayElement = 0;
	write_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_sets[0].descriptorCount = 1;
	write_sets[0].pBufferInfo = &buffer_info;

	write_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_sets[1].dstSet = descriptor_set;
	write_sets[1].dstBinding = 1;
	write_sets[1].dstArrayElement = 0;
	write_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	write_sets[1].descriptorCount = 1;
	write_sets[1].pBufferInfo = &dynamic_info;

	vkUpdateDescriptorSets(device, 2, write_sets, 0, nullptr);
}

void Graphics::initialize_texture_descriptor(Model* model) {
	{
		VkDescriptorSetLayout layouts[] = { descriptor_set_layouts.model };
		VkDescriptorSetAllocateInfo allocate_info = {};
		allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocate_info.descriptorPool = descriptor_pool;
		allocate_info.descriptorSetCount = 1;
		allocate_info.pSetLayouts = layouts;

		if (vkAllocateDescriptorSets(device, &allocate_info, &model->texture_descriptor_set) != VK_SUCCESS) {
			Engine::fail("Unable to create descriptor set");
		}
	}

	VkDescriptorImageInfo image_info = {};
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.imageView = model->texture_image_view;
	image_info.sampler = texture_sampler;

	VkWriteDescriptorSet write_set = {};
	write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_set.dstSet = model->texture_descriptor_set;
	write_set.dstBinding = 0;
	write_set.dstArrayElement = 0;
	write_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_set.descriptorCount = 1;
	write_set.pImageInfo = &image_info;

	vkUpdateDescriptorSets(device, 1, &write_set, 0, NULL);
}

VkCommandBuffer Graphics::begin_command_buffer(VkCommandPool command_pool) {
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandPool = command_pool;
	allocate_info.commandBufferCount = 1;

	VkCommandBuffer buffer;
	if (vkAllocateCommandBuffers(device, &allocate_info, &buffer) != VK_SUCCESS) {
		Engine::fail("Unable to allocate command buffers");
	}

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(buffer, &begin_info);
	return buffer;
}

void Graphics::end_command_buffer(VkCommandBuffer buffer) { end_command_buffer(buffer, transfer_queue, transfer_command_pool); }

void Graphics::end_command_buffer(VkCommandBuffer buffer, VkQueue queue, VkCommandPool command_pool) {
	vkEndCommandBuffer(buffer);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &buffer;

	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue); //Block.

	vkFreeCommandBuffers(device, command_pool, 1, &buffer);
}

void Graphics::set_barrier_access_masks(VkAccessFlags* source_mask, VkAccessFlags* destination_mask, VkPipelineStageFlags* srcStageMask, VkPipelineStageFlags* dstStageMask, VkImageLayout old_layout, VkImageLayout new_layout) {
	if (old_layout == VK_IMAGE_LAYOUT_PREINITIALIZED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		*source_mask = VK_ACCESS_HOST_WRITE_BIT;
		*destination_mask = VK_ACCESS_TRANSFER_READ_BIT;
		*srcStageMask = VK_PIPELINE_STAGE_HOST_BIT;
		*dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_PREINITIALIZED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		*source_mask = VK_ACCESS_HOST_WRITE_BIT;
		*destination_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
		*srcStageMask = VK_PIPELINE_STAGE_HOST_BIT;
		*dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		*source_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
		*destination_mask = VK_ACCESS_SHADER_READ_BIT;
		*srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		*dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		*source_mask = 0;
		*destination_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		*srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		*dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else {
		Engine::fail("Unable to transition image layout. The layout transition is not supported");
	}
}

void Graphics::set_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, VkCommandBuffer buffer) {
	bool init_buffer = buffer == VK_NULL_HANDLE;
	if (init_buffer) buffer = begin_command_buffer(transfer_command_pool);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		bool is_stencil = format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
		if (is_stencil)
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	} else
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	VkPipelineStageFlags srcStageMask;
	VkPipelineStageFlags dstStageMask;
	set_barrier_access_masks(&barrier.srcAccessMask, &barrier.dstAccessMask, &srcStageMask, &dstStageMask, old_layout, new_layout);

	vkCmdPipelineBarrier(buffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	if (init_buffer) end_command_buffer(buffer);
}

void Graphics::copy_image(VkImage source, VkImage destination, VkExtent2D extent, VkCommandBuffer buffer) {
	bool init_buffer = buffer == VK_NULL_HANDLE;
	if (init_buffer) buffer = begin_command_buffer(transfer_command_pool);

	VkImageSubresourceLayers sub_resource = {};
	sub_resource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	sub_resource.baseArrayLayer = 0;
	sub_resource.mipLevel = 0;
	sub_resource.layerCount = 1;

	VkImageCopy region = {};
	region.srcSubresource = sub_resource;
	region.dstSubresource = sub_resource;
	region.srcOffset = { 0, 0, 0 };
	region.dstOffset = { 0, 0, 0 };
	region.extent.width = extent.width;
	region.extent.height = extent.height;
	region.extent.depth = 1;

	vkCmdCopyImage(buffer, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	if (init_buffer) end_command_buffer(buffer);
}

void Graphics::copy_buffer(VkBuffer source, VkBuffer destination, VkDeviceSize size, VkCommandBuffer buffer) {
	bool init_buffer = buffer == VK_NULL_HANDLE;
	if (init_buffer) buffer = begin_command_buffer(transfer_command_pool);

	VkBufferCopy region = {};
	region.size = size;
	vkCmdCopyBuffer(buffer, source, destination, 1, &region);

	if (init_buffer) end_command_buffer(buffer);
}

void Graphics::create_image(VkExtent2D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage* image, VkDeviceMemory* memory) {
	if (*image) vkDestroyImage(device, *image, nullptr);
	if (*memory) vkFreeMemory(device, *memory, nullptr);

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = extent.width;
	image_info.extent.height = extent.height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.format = format;
	image_info.tiling = tiling;
	image_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	image_info.usage = usage;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	//image_info.sharingMode = VK_SHARING_MODE_CONCURRENT; //VK_SHARING_MODE_EXCLUSIVE;
	//image_info.queueFamilyIndexCount = 2;
	//uint32_t family_indices[] = { (uint32_t)graphics_family_index, (uint32_t)transfer_family_index };
	//image_info.pQueueFamilyIndices = family_indices;

	if (vkCreateImage(device, &image_info, nullptr, image) != VK_SUCCESS) {
		Engine::fail("Unable to create staging image");
	}

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(device, *image, &memory_requirements);

	VkMemoryAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize = memory_requirements.size;
	allocate_info.memoryTypeIndex = get_memory_type_index(memory_requirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocate_info, nullptr, memory) != VK_SUCCESS) {
		Engine::fail("Unable to allocate staging image memory");
	}

	vkBindImageMemory(device, *image, *memory, 0);
}

void Graphics::initialize_swap_chain() {
	VkSwapchainKHR old_swap_chain = swap_chain;

	is_surface_supported(physical_device);
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
		surface_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
	} else {
		bool found = false;
		for (const VkSurfaceFormatKHR format : formats) {
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
				surface_format = format;
				found = true;
				break;
			}
		}
		if (!found) {
			std::cerr << "warning: Unable to find desired surface format!!!!!!!!!!!!!!!!!" << std::endl;
			surface_format = formats[0];
		}
	}

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; //Always use vsync'd double buffering for now. ..VK_PRESENT_MODE_MAILBOX_KHR

	if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		extent = surface_capabilities.currentExtent;
	} else {
		extent.height = glm::clamp(extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
		extent.width = glm::clamp(extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
	}

	if (surface_capabilities.maxImageCount != 0)
		image_count = glm::clamp(image_count, surface_capabilities.minImageCount, surface_capabilities.maxImageCount);
	else
		image_count = glm::min(image_count, surface_capabilities.minImageCount);

	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1; //Always 1 unless VR.
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //use VK_IMAGE_USAGE_TRANSFER_DST_BIT for post-processing.
	create_info.preTransform = surface_capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = old_swap_chain;

	//std::set<uint32_t> queue_family_indices = { (uint32_t)graphics_family_index, (uint32_t)window_family_index, (uint32_t)transfer_family_index };
	//std::vector<uint32_t> index_vector(queue_family_indices.begin(), queue_family_indices.end());
	//if (index_vector.size() > 1) {
	//	create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;	
	//	create_info.queueFamilyIndexCount = index_vector.size();
	//	create_info.pQueueFamilyIndices = index_vector.data();
	//} else {
	//	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	//}

	if (graphics_family_index != window_family_index) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;

		uint32_t queue_family_indices[] = { (uint32_t)graphics_family_index, (uint32_t)window_family_index };
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	} else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}


	if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain) != VK_SUCCESS) {
		Engine::fail("Unable to create swap chain");
	}

	if (old_swap_chain)
		vkDestroySwapchainKHR(device, old_swap_chain, nullptr);

	vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
	images.resize(image_count);
	vkGetSwapchainImagesKHR(device, swap_chain, &image_count, images.data());
}

VkFormat Graphics::get_depth_format() {
	return get_supported_format({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat Graphics::get_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat possible_format : candidates) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(physical_device, possible_format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
			return possible_format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
			return possible_format;
		}
	}

	Engine::fail("Unable to find supported format");
}

void Graphics::initialize_render_pass() {
	if (render_pass) vkDestroyRenderPass(device, render_pass, nullptr);

	VkAttachmentDescription color_attachment = {};
	color_attachment.format = surface_format.format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT; //No multisampling yet.
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //Clear before every pass.
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depth_attachment = {};
	depth_attachment.format = get_depth_format();
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_reference = {};
	depth_attachment_reference.attachment = 1;
	depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_reference = {};
	color_attachment_reference.attachment = 0;
	color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_reference;
	subpass.pDepthStencilAttachment = &depth_attachment_reference;

	VkSubpassDependency subpass_dependency = {};
	subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dependency.dstSubpass = 0;
	subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.srcAccessMask = 0;
	subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	{
		VkAttachmentDescription attachments[] = { color_attachment, depth_attachment };
		VkRenderPassCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		create_info.attachmentCount = 2;
		create_info.pAttachments = attachments;
		create_info.subpassCount = 1;
		create_info.pSubpasses = &subpass;
		create_info.dependencyCount = 1;
		create_info.pDependencies = &subpass_dependency;

		if (vkCreateRenderPass(device, &create_info, nullptr, &render_pass) != VK_SUCCESS) {
			Engine::fail("Unable to create render pass");
		}

		subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		if (vkCreateRenderPass(device, &create_info, nullptr, &imgui_render_pass) != VK_SUCCESS) {
			Engine::fail("Unable to create render pass");
		}
	}
}

void Graphics::initialize_descriptor_set_layout() {
	//if (descriptor_set_layout) vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
	if (descriptor_set_layouts.model) vkDestroyDescriptorSetLayout(device, descriptor_set_layouts.model, nullptr);
	if (descriptor_set_layouts.scene) vkDestroyDescriptorSetLayout(device, descriptor_set_layouts.scene, nullptr);

	{
		VkDescriptorSetLayoutBinding mvp_binding = {};
		mvp_binding.binding = 0;
		mvp_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		mvp_binding.descriptorCount = 1;
		mvp_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding transform_binding = {};
		transform_binding.binding = 1;
		transform_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		transform_binding.descriptorCount = 1;
		transform_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding bindings[] = { mvp_binding, transform_binding };
		VkDescriptorSetLayoutCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		create_info.bindingCount = 2;
		create_info.pBindings = bindings;

		if (vkCreateDescriptorSetLayout(device, &create_info, nullptr, &descriptor_set_layouts.scene) != VK_SUCCESS) {
			Engine::fail("Unable to create descriptor set layout");
		}
	}

	{
		VkDescriptorSetLayoutBinding sampler_binding = {};
		sampler_binding.binding = 0; //0?
		sampler_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_binding.descriptorCount = 1;
		sampler_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding bindings[] = { sampler_binding };
		VkDescriptorSetLayoutCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		create_info.bindingCount = 1;
		create_info.pBindings = bindings;

		if (vkCreateDescriptorSetLayout(device, &create_info, nullptr, &descriptor_set_layouts.model) != VK_SUCCESS) {
			Engine::fail("Unable to create descriptor set layout");
		}
	}
}

void Graphics::initialize_imgui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui_ImplGlfw_InitForVulkan(window, true);

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance;
	init_info.PhysicalDevice = physical_device;
	init_info.Device = device;
	init_info.QueueFamily = graphics_family_index;
	init_info.Queue = graphics_queue;
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = descriptor_pool;
	init_info.Allocator = VK_NULL_HANDLE;
	init_info.CheckVkResultFn = check_vk_result;
	ImGui_ImplVulkan_Init(&init_info, render_pass);

	// Setup style
	ImGui::StyleColorsDark();

	{
		// Use any command queue
		//VkCommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;
		//VkCommandBuffer command_buffer = wd->Frames[wd->FrameIndex].CommandBuffer;

		VkResult err = vkResetCommandPool(device, graphics_command_pool, 0);
		check_vk_result(err);
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		err = vkBeginCommandBuffer(command_buffers[0], &begin_info);
		check_vk_result(err);

		ImGui_ImplVulkan_CreateFontsTexture(command_buffers[0]);

		VkSubmitInfo end_info = {};
		end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		end_info.commandBufferCount = 1;
		end_info.pCommandBuffers = command_buffers.data();
		err = vkEndCommandBuffer(command_buffers[0]);
		check_vk_result(err);
		err = vkQueueSubmit(graphics_queue, 1, &end_info, VK_NULL_HANDLE);
		check_vk_result(err);

		err = vkDeviceWaitIdle(device);
		check_vk_result(err);
		ImGui_ImplVulkan_InvalidateFontUploadObjects();
	}
}

void Graphics::initialize_pipeline() {
	if (pipeline) vkDestroyPipeline(device, pipeline, nullptr);
	if (pipeline_layout) vkDestroyPipelineLayout(device, pipeline_layout, nullptr);

	std::vector<char> vertex_shader_file, fragment_shader_file;
	VkShaderModule vertex_shader, fragment_shader;

	if (!read_file_binary("shaders/shader.vert.spv", &vertex_shader_file) ||
		!read_file_binary("shaders/shader.frag.spv", &fragment_shader_file))
		Engine::fail("Unable to read shaders");
	{
		VkShaderModuleCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = vertex_shader_file.size();
		create_info.pCode = (uint32_t*)vertex_shader_file.data();
		if (vkCreateShaderModule(device, &create_info, nullptr, &vertex_shader) != VK_SUCCESS) {
			Engine::fail("Failed to create vertex shader module");
		}
	}

	{
		VkShaderModuleCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = fragment_shader_file.size();
		create_info.pCode = (uint32_t*)fragment_shader_file.data();
		if (vkCreateShaderModule(device, &create_info, nullptr, &fragment_shader) != VK_SUCCESS) {
			Engine::fail("Failed to create fragment shader module");
		}
	}

	VkPipelineShaderStageCreateInfo shader_stages[2];
	{
		VkPipelineShaderStageCreateInfo vertex_create_info = {};
		vertex_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertex_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertex_create_info.module = vertex_shader;
		vertex_create_info.pName = "main"; //Function name main()
		shader_stages[0] = vertex_create_info;

		VkPipelineShaderStageCreateInfo fragment_create_info = {};
		fragment_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragment_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragment_create_info.module = fragment_shader;
		fragment_create_info.pName = "main"; //Function name main()
		shader_stages[1] = fragment_create_info;
	}

	VkVertexInputBindingDescription binding_description = Vertex::get_binding_description();
	std::vector<VkVertexInputAttributeDescription> attribute_descriptions = Vertex::get_attribute_descriptions();

	VkPipelineVertexInputStateCreateInfo vertex_input = {};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input.vertexBindingDescriptionCount = 1;
	vertex_input.pVertexBindingDescriptions = &binding_description;
	vertex_input.vertexAttributeDescriptionCount = attribute_descriptions.size();
	vertex_input.pVertexAttributeDescriptions = attribute_descriptions.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)extent.width;
	viewport.height = (float)extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = extent;

	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE; //Requires some(?) GPU feature.
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f; //can be thicker if gpu feature "wideLines" is enabled.
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {}; //Allows anti-aliasing (if enabled).
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 0.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	//Alpha blending. 
	VkPipelineColorBlendAttachmentState color_blend_buffer = {};
	color_blend_buffer.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; //RGBA
	color_blend_buffer.blendEnable = VK_FALSE;
	//color_blend_buffer.blendEnable = VK_TRUE;
	//color_blend_buffer.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	//color_blend_buffer.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	//color_blend_buffer.colorBlendOp = VK_BLEND_OP_ADD;
	//color_blend_buffer.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	//color_blend_buffer.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	//color_blend_buffer.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable = VK_TRUE;
	depth_stencil.depthWriteEnable = VK_TRUE;
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo color_blend = {};
	color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend.logicOpEnable = VK_FALSE; //For bit-wise blending.
	color_blend.logicOp = VK_LOGIC_OP_COPY;
	color_blend.attachmentCount = 1;
	color_blend.pAttachments = &color_blend_buffer;

	//VkPushConstantRange push_constant_range = {};
	//push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	//push_constant_range.offset = 0;
	//push_constant_range.size = sizeof(mvp);

	//if we want to change things on the fly, declare a dynamic state (VkPipelineDynamicStateCreateInfo).

	{
		VkDescriptorSetLayout layouts[2];
		layouts[0] = descriptor_set_layouts.scene;
		layouts[1] = descriptor_set_layouts.model;

		VkPipelineLayoutCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		create_info.setLayoutCount = 2;
		create_info.pSetLayouts = layouts;
		//create_info.pushConstantRangeCount = 1;
		//create_info.pPushConstantRanges = &push_constant_range;

		if (vkCreatePipelineLayout(device, &create_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
			Engine::fail("Unable to create pipeline layout");
		}
	}

	{
		VkGraphicsPipelineCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		create_info.stageCount = 2;
		create_info.pStages = shader_stages;
		create_info.pVertexInputState = &vertex_input;
		create_info.pInputAssemblyState = &input_assembly;
		create_info.pViewportState = &viewport_state;
		create_info.pRasterizationState = &rasterizer;
		create_info.pMultisampleState = &multisampling;
		create_info.pDepthStencilState = &depth_stencil;
		create_info.pColorBlendState = &color_blend;
		create_info.pDynamicState = nullptr;
		create_info.layout = pipeline_layout;
		create_info.renderPass = render_pass;
		create_info.subpass = 0;//index 0.
		create_info.basePipelineHandle = VK_NULL_HANDLE; //no base pipeline to derive from.
														 //create_info.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline) != VK_SUCCESS) {
			Engine::fail("Unable to create graphics pipeline(s)");
		}
	}

	if (vertex_shader) vkDestroyShaderModule(device, vertex_shader, nullptr);
	if (fragment_shader) vkDestroyShaderModule(device, fragment_shader, nullptr);
}

bool Graphics::is_device_compatible(VkPhysicalDevice device) {
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceProperties(device, &properties);
	vkGetPhysicalDeviceFeatures(device, &features);

	if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || !features.geometryShader
		|| properties.limits.maxPushConstantsSize < sizeof(Transform_Matrix))
		return false;

	if (!are_extensions_supported(device))
		return false;

	if (!is_surface_supported(device))
		return false;

	uint32_t family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);
	std::vector<VkQueueFamilyProperties> families(family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, families.data());

	graphics_family_index = -1;
	window_family_index = -1;
	transfer_family_index = -1;

	for (uint32_t i = 0; i < family_count; ++i) {
		if (families[i].queueCount == 0)
			continue;

		if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			graphics_family_index = i;
		} else if (families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
			transfer_family_index = i;
		}
		VkBool32 supported = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supported);
		if (supported) {
			window_family_index = i;
		}

		if (graphics_family_index >= 0 && window_family_index >= 0 && transfer_family_index >= 0)
			return true;
	}

	if (graphics_family_index >= 0 && window_family_index >= 0) {
		transfer_family_index = graphics_family_index;
		return true;
	}

	return false;
}

bool Graphics::should_exit() {
	return glfwWindowShouldClose(window) != 0;
}

//Also updates surface formats.
bool Graphics::is_surface_supported(VkPhysicalDevice device) {
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surface_capabilities);

	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
	formats.resize(format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, formats.data());

	uint32_t present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
	present_modes.resize(present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, present_modes.data());

	return !formats.empty() && !present_modes.empty();
}

bool Graphics::are_extensions_supported(VkPhysicalDevice device) {
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_device_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_device_extensions.data());
	bool found = false;
	for (uint32_t i = 0; i < device_extensions.size(); ++i) {
		for (uint32_t j = 0; j < extension_count; ++j) {
			if (strcmp(device_extensions[i], available_device_extensions[j].extensionName) == 0)
				found = true;
		}
		if (!found)
			return false;
	}

	return true;
}

void Graphics::get_extensions(std::vector<const char*>* extensions) {

	uint32_t extension_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extension_count);

	for (uint32_t i = 0; i < extension_count; ++i)
		extensions->push_back(glfw_extensions[i]);

#ifdef VALIDATE_VULKAN
	extensions->push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif // VALIDATE_VULKAN
}

void Graphics::get_validation_layers(std::vector<const char*>* layers) {
#ifdef VALIDATE_VULKAN
	layers->push_back("VK_LAYER_LUNARG_standard_validation");

	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
	for (const char* layer : *layers) {
		bool found = false;
		for (const VkLayerProperties property : available_layers) {
			if (strcmp(layer, property.layerName) == 0) {
				found = true;
			}
		}

		if (!found) {
			char error_message[128];
			sprintf_s(error_message, 128, "Unable to find support for Vulkan layer %s.", layer);
			Engine::fail(error_message);
		}
	}
#endif // VALIDATE_VULKAN
}

int Graphics::get_memory_type_index(uint32_t filter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties physical_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_properties);

	int count = physical_properties.memoryTypeCount;
	for (int i = 0; i < count; ++i) {
		bool is_suitable = (filter & (1 << i)) > 0; //if it's not filtered out as an unsuitable memory type for the buffer.
		bool has_properties = (physical_properties.memoryTypes[i].propertyFlags & properties) == properties;
		if (is_suitable && has_properties) {
			return i;
		}
	}
	return -1;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
	uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {

	char * flagName;
	switch (flags) {
	case VK_DEBUG_REPORT_ERROR_BIT_EXT: flagName = "ERROR"; break;
	case VK_DEBUG_REPORT_WARNING_BIT_EXT: flagName = "Warning"; break;
	case VK_DEBUG_REPORT_INFORMATION_BIT_EXT: flagName = "Information"; break;
	case VK_DEBUG_REPORT_DEBUG_BIT_EXT: flagName = "Debug"; break;
	default: flagName = "Unknown"; break;
	}

	std::cerr << "[" << flagName << "] [" << layerPrefix << "] " << msg << std::endl;
	return VK_FALSE;
}

void Graphics::reinitialize_swap_chain() {
	initialize_swap_chain();
	initialize_swap_chain_image_views();
	initialize_render_pass();
	initialize_pipeline();
	initialize_depth_image();
	initialize_frame_buffers();
	//	initalize_command_buffers();
}

void Graphics::resize(int width, int height) {
	extent = { (uint32_t)width, (uint32_t)height };
	vkDeviceWaitIdle(device); //Needed?
	reinitialize_swap_chain();
}

void Graphics::update_dynamic_buffer(void * data, VkDeviceSize size) {
	dynamic_buffer.fill_staging_buffer(data, size);
	dynamic_buffer.copy_staging_data();
}

void Graphics::set_main_camera(Camera* camera) {
	this->camera = camera;
}

void Graphics::draw(Time& time) {
	assert(_is_initialized);

	if (!camera) return;

	{
		assert(camera != nullptr);
		Transform* camera_transform = camera->velocity.get()->transform.get();
		glm::vec3 pos = camera_transform->position;
		pos.y -= camera->zoom;
		pos.z += camera->zoom;

		Model_View_Projection mvp = {};
		mvp.model = glm::rotate(mvp.model, 0.f, glm::vec3(0.0f, 0.0f, 1.0f));
		mvp.view = glm::lookAt(pos, pos - glm::vec3(0.0f, -1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		mvp.projection = glm::perspective(glm::radians(45.0f), (float)extent.width / (float)extent.height, 0.1f, 100.0f);
		mvp.projection[1][1] *= -1;

		Transform_Matrix camera_matrix = {};
		camera_matrix.model = mvp.projection * mvp.view * mvp.model;

		uniform_buffer.fill_staging_buffer(&camera_matrix, sizeof(camera_matrix));
		uniform_buffer.copy_staging_data();
		//update_command_buffers();
	}

	uint32_t image_index;
	VkResult result = vkAcquireNextImageKHR(device, swap_chain, std::numeric_limits<uint64_t>::max(), image_semaphore, VK_NULL_HANDLE, &image_index);
	update_command_buffer(image_index);

	if (result != VK_SUCCESS) {
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			reinitialize_swap_chain();
		} else if (result == VK_SUBOPTIMAL_KHR) {
			//log?
		} else {
			Engine::fail("Unable to acquire swap chain image");
		}
	}

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore			 wait_semaphores[] = { image_semaphore };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffers[image_index];

	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &presentation_semaphore;

	if (vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
		Engine::fail("Unable to enqueue command");
	}

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &presentation_semaphore;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swap_chain;
	present_info.pImageIndices = &image_index;

	result = vkQueuePresentKHR(window_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		reinitialize_swap_chain();
	} else if (result != VK_SUCCESS) {
		Engine::fail("Unable to present to swap chain image");
	}
}

Graphics::~Graphics() {
	if (device) vkDeviceWaitIdle(device);

	if (window) glfwDestroyWindow(window);

	for (Model* model : models) if (model) delete model;
	if (command_buffers.size() > 0) vkFreeCommandBuffers(device, graphics_command_pool, command_buffers.size(), command_buffers.data());
	vertex_buffer.destroy();
	index_buffer.destroy();
	uniform_buffer.destroy();
	if (texture_image) vkDestroyImage(device, texture_image, nullptr);
	if (texture_image_memory) vkFreeMemory(device, texture_image_memory, nullptr);
	if (texture_image_view) vkDestroyImageView(device, texture_image_view, nullptr);
	if (texture_sampler) vkDestroySampler(device, texture_sampler, nullptr);
	if (depth_image) vkDestroyImage(device, depth_image, nullptr);
	if (depth_image_memory) vkFreeMemory(device, depth_image_memory, nullptr);
	if (depth_image_view) vkDestroyImageView(device, depth_image_view, nullptr);
	dynamic_buffer.destroy();

	if (descriptor_pool) vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
	if (graphics_command_pool) vkDestroyCommandPool(device, graphics_command_pool, nullptr);
	if (transfer_command_pool) vkDestroyCommandPool(device, transfer_command_pool, nullptr);
	if (presentation_semaphore) vkDestroySemaphore(device, presentation_semaphore, nullptr);
	if (image_semaphore) vkDestroySemaphore(device, image_semaphore, nullptr);
	for (VkFramebuffer frame_buffer : frame_buffers) if (frame_buffer) vkDestroyFramebuffer(device, frame_buffer, nullptr);
	if (pipeline) vkDestroyPipeline(device, pipeline, nullptr);
	if (render_pass) vkDestroyRenderPass(device, render_pass, nullptr);
	//if (descriptor_set_layout) vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
	if (descriptor_set_layouts.model) vkDestroyDescriptorSetLayout(device, descriptor_set_layouts.model, nullptr);
	if (descriptor_set_layouts.scene) vkDestroyDescriptorSetLayout(device, descriptor_set_layouts.scene, nullptr);
	if (pipeline_layout) vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
	for (VkImageView image_view : image_views) if (image_view) vkDestroyImageView(device, image_view, nullptr);
	if (swap_chain) vkDestroySwapchainKHR(device, swap_chain, nullptr);
	if (device) vkDestroyDevice(device, nullptr);
	if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
	if (debug_callback) destroy_debug_report_callback(instance, debug_callback, nullptr);
	if (instance) vkDestroyInstance(instance, nullptr);
}

VkResult create_debug_report_callback(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void destroy_debug_report_callback(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}