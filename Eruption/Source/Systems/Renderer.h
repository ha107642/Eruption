#pragma once

#include "System.h"
#include "Transform_System.h"
#include "Graphics/Model.h"
#include "Graphics/Graphics.h"
#include "Aligned_Array.h"

extern void window_resized(GLFWwindow* window, int width, int height);

struct Render {
	Model* model;
};

class Renderer : public System<Render> {
protected:
	Graphics* graphics;
	Aligned_Array<glm::mat4> aligned_transforms;
	bool update_command_buffers;
	std::vector<Model*> models;
	System_Reference<Render, Transform> transforms;

	void update(Render* const render, Entity entity, Time& time) {}
public:
	Renderer() : graphics(nullptr), aligned_transforms(0, 0), transforms(*this) { }
	Renderer(Graphics* graphics) : graphics(graphics),
		update_command_buffers(false), aligned_transforms(/*graphics->dynamic_buffer_alignment*/sizeof(glm::mat4)), transforms(*this) {
		glfwSetWindowSizeCallback(graphics->window, window_resized);
		graphics->initialize_buffers();
		graphics->resize_dynamic_buffer(aligned_transforms.memory_capacity());
	}

	Renderer(Graphics* graphics, int alignment) : graphics(graphics), 
		update_command_buffers(false), aligned_transforms(alignment), transforms(*this) {
		glfwSetWindowSizeCallback(graphics->window, window_resized);
	}

	virtual Render* add_component(Entity entity) override;
	virtual void delete_component(Entity entity) override;
	virtual void update(Time& time) override;
	virtual void serialize(Memory_Stream &stream, Render& component, Entity entity) override;
	virtual void deserialize(Memory_Stream &stream, Render* component, Entity entity) override;
	virtual void set_state(System_State state) override;
	virtual System_State get_state() override;

	void update_offsets();
};