#pragma once

#include "System.h"
#include "Transform_System.h"
#include "Graphics/Model.h"
#include "Graphics/Graphics.h"
#include "Aligned_Array.h"

extern void window_resized(GLFWwindow* window, int width, int height);

struct Render {
	Component_Reference<Transform> transform;
	Model* model;
};

class Renderer : public System<Render> {
protected:
	Graphics* graphics;
	Aligned_Array<Transform_Matrix> transforms;
	bool update_command_buffers;
	std::vector<Model*> models;

	void update(Render* const render, Entity entity, Time& time) {}
	void update(Render* const render, Transform_Matrix* transform, Entity entity, Time& time);
public:
	Renderer() : graphics(nullptr), transforms(0, 0) { }
	Renderer(Graphics* graphics) : graphics(graphics),
		update_command_buffers(false), transforms(graphics->dynamic_buffer_alignment) {
		glfwSetWindowSizeCallback(graphics->window, window_resized);
		graphics->initialize_buffers();
		graphics->resize_dynamic_buffer(transforms.memory_capacity());
	}

	Renderer(Graphics* graphics, int alignment) : graphics(graphics), 
		update_command_buffers(false), transforms(alignment) {
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