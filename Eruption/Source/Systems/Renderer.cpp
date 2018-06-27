#include "Renderer.h"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "Transform_System.h"
#include "Memory_Stream.h"
#include "io.h"
#include "Engine.h"
#include "Physics.h"

void Renderer::update(Render * const render, Transform_Matrix * transform, Entity entity, Time& time) {
	Transform* t = render->transform.get();
	glm::mat4 scale = glm::scale(t->scale);
	glm::mat4 rotation = glm::toMat4(t->rotation);
	glm::mat4 translation = glm::translate(t->position);
	transform->model = translation * rotation * scale;
}

void Renderer::update_offsets() {
	int size = components.size();
	if (wireframe_enabled) {
		Physics* physics_system = (Physics*)engine->get_system<Hitbox>();
		std::vector<Component_Holder<Hitbox>> hitboxes = physics_system->get_hitboxes();
		size += hitboxes.size();
	}
	std::vector<Render_Data> render_data(size);
	for (int i = 0; i < components.size(); ++i) {
		//render_data[i].offset = i * sizeof(Transform_Matrix);
		render_data[i].model = components[i].component.model;
	}

	if (wireframe_enabled) {
		Physics* physics_system = (Physics*)engine->get_system<Hitbox>();
		std::vector<Component_Holder<Hitbox>> hitboxes = physics_system->get_hitboxes();
		for (int i = 0; i < hitboxes.size(); ++i) {
			Model model = Model::load_wireframe(hitboxes[i].component.position, hitboxes[i].component.half_size);
			Model* m = new Model(model); //FIX: MEMORY LEAK!
			render_data[components.size() + i].model = m;
			render_data[components.size() + i].type = RENDER_TYPE_WIREFRAME;
		}
	}

	graphics->update_command_buffers(render_data);
	update_command_buffers = false;
}

void Renderer::toggle_wireframe() {
	wireframe_enabled = !wireframe_enabled;
	update_offsets();
}

Render * Renderer::add_component(Entity entity) {
	Render* component = System::add_component(entity);
	if (!graphics)
		return component;

	int capacity = transforms.capacity();
	Transform_Matrix matrix = {};
	transforms.push_back(matrix);
	if (capacity != transforms.capacity()) {
		graphics->resize_dynamic_buffer(transforms.memory_capacity());
	}

	update_command_buffers = true;
	return component;
}

void Renderer::delete_component(Entity entity) {
	System::delete_component(entity);
	if (!graphics) return;
	transforms.pop_back(); //Might need to change this if we don't recalculate all transforms every frame.
	update_command_buffers = true;
}

void Renderer::update(Time& time) {
	if (!graphics) return;
	int count = components.size();
	if (count == 0) return;

	for (int i = 0; i < count; ++i)
		update(&components[i].component, &transforms[i], components[i].entity, time);
	
	if (update_command_buffers) {
		update_offsets();
	}

	graphics->update_dynamic_buffer(transforms.data(), transforms.memory_size());
}

void Renderer::serialize(Memory_Stream & stream, Render & component, Entity entity) {
	//write(stream, entity);
	int name_length = component.model->name.length();
	write(stream, name_length);
	stream.write((void*)component.model->name.c_str(), name_length);
}

void Renderer::deserialize(Memory_Stream & stream, Render * component, Entity entity) {
	int model_length;
	read(stream, &model_length);
	char* model_name = new char[model_length + 1];
	stream.read(model_name, model_length);
	model_name[model_length] = '\0';

	if (graphics) {
		component->model = graphics->load_model(model_name);
	} else {
		Model* model = nullptr;
		for (Model* loaded_model : models) {
			if (strcmp(loaded_model->name.c_str(), model_name) == 0) {
				model = loaded_model;
				break;
			}
		}
		if (!model) {
			model = new Model();
			models.push_back(model);
			model->name = model_name;
		}

		component->model = model;
	}
	component->transform = engine->get_component_reference<Transform>(entity);

	delete[] model_name;
}

void Renderer::set_state(System_State state) {
	if (graphics) {
		int capacity = transforms.capacity();
		transforms.resize(state.count);
		if (capacity != transforms.capacity()) {
			graphics->resize_dynamic_buffer(transforms.memory_capacity());
		}
	}

	System<Render>::set_state(state);
	update_command_buffers = true;
}

System_State Renderer::get_state() {
	System_State state = {};
	state.system_id = system_id;
	state.count = components.size();

	//Calculate size.
	state.size = (sizeof(Entity) + sizeof(int)) * components.size();
	for (uint32_t i = 0; i < state.count; ++i) {
		state.size += components[i].component.model->name.length(); //NO null-termination.
	}
	
	state.data = std::shared_ptr<void>(malloc(state.size), free);
	Memory_Stream stream(state.data.get(), state.size);
	for (uint32_t i = 0; i < state.count; ++i) {
		write(stream, components[i].entity);
		int name_length = components[i].component.model->name.length();
		write(stream, name_length);
		stream.write((void*)components[i].component.model->name.c_str(), name_length);
	}

	return state;
}

#include "Engine.h"

void window_resized(GLFWwindow* window, int width, int height) {
	if (width == 0 || height == 0) return;

	Engine *engine = (Engine*)glfwGetWindowUserPointer(window);
	engine->resize_window(width, height);
	((Renderer*)engine->get_system<Render>())->update_offsets();
}
