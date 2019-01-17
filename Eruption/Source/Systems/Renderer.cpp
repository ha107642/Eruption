#include "Renderer.h"

#include "Transform_System.h"
#include "Memory_Stream.h"
#include "io.h"
#include "Engine.h"

void Renderer::update_offsets() {
	int size = components.size();
	std::unordered_map<Model*, uint32_t> model_instances_map(components.size());
	for (int i = 0; i < size; ++i) {
		++model_instances_map[components[i].component.model];
	}

	graphics->set_model_instances(model_instances_map);
	update_command_buffers = false;
}

Render * Renderer::add_component(Entity entity) {
	Render* component = System::add_component(entity);
	if (!graphics)
		return component;

	int capacity = aligned_transforms.capacity();
	aligned_transforms.push_back(glm::mat4());
	if (capacity != aligned_transforms.capacity()) {
		graphics->resize_dynamic_buffer(aligned_transforms.memory_capacity());
	}

	update_command_buffers = true;
	return component;
}

void Renderer::delete_component(Entity entity) {
	System::delete_component(entity);
	if (!graphics) return;
	aligned_transforms.pop_back(); //Might need to change this if we don't recalculate all transforms every frame.
	update_command_buffers = true;
}

void Renderer::update(Time& time) {
	if (!graphics) return;
	int count = components.size();
	if (count == 0) return;

	for (int i = 0; i < count; ++i)
		aligned_transforms[i] = transforms[i]->to_matrix();
	
	if (update_command_buffers) {
		update_offsets();
	}

	graphics->update_dynamic_buffer(aligned_transforms.data(), aligned_transforms.memory_size());
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

	delete[] model_name;
}

unsigned long upper_power_of_two(unsigned long v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;

}


void Renderer::set_state(System_State state) {
	if (graphics) {
		int capacity = aligned_transforms.capacity();
		aligned_transforms.resize(upper_power_of_two(state.count));
		if (capacity != aligned_transforms.capacity()) {
			graphics->resize_dynamic_buffer(aligned_transforms.memory_capacity());
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
