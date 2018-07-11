#pragma once

#include "Graphics/Graphics.h"
#include "Time.h"
#include "System.h"
#include "Entity.h"
#include "Input.h"

#include <atomic>

#define SERVER_FPS (20)

extern thread_local Engine *engine;

class Engine {
private:
	Graphics graphics;
	std::unordered_map<Entity, std::vector<ISystem*> > entity_components;
	std::atomic<Entity> last_entity = 0;
	std::vector<ISystem*> systems;
	bool _is_authoritative;
	bool enable_debug;

	bool should_exit();
	void update_systems(Time& time);
public:
	Time time;
	Input input;

	Engine() : enable_debug(false) { engine = this; }

	bool is_authoritative() { return _is_authoritative; };
	Entity new_entity();
	Entity new_entity(Entity copy_from);
	void destroy_entity(Entity *entity);

	void set_main_camera(Entity entity);
	void resize_window(int width, int height);
	void set_delta_state(Memory_Stream & stream);
	void serialize_delta_state(Memory_Stream &stream, State &base_state);
	State get_state();
	void set_state(State &state);
	void run();
	void run_client();
	void run_server(bool *should_exit = nullptr);
	static void fail(const char* reason, ...);

	void add_entity_component(Entity entity, ISystem *system);
	void remove_entity_component(Entity entity, ISystem * system);
	template<typename T> inline void register_system(System<T> *system, const char* system_name = nullptr) {
		int system_id = systems.size();
		system->system_id = system_id;
		systems.push_back(system);
		
		if (system_name)
			Debugging::debug_register_system(system, system_name);
		//systems[typeid(T)] = system;
	}

	inline ISystem* get_system(uint8_t system_id) {
		return systems[system_id];
	}

	template<typename T> inline System<T>* get_system() {
		//return (System<T>*)systems[typeid(T)];
		uint8_t id = System<T>::system_id;
		assert(id != UINT8_MAX);
		return (System<T>*)systems[id];
	}

	template<typename T> inline bool has_component(Entity entity) {
		return get_system<T>()->has_component(entity);
	}

	//WARNING: Needs to be updated every frame, since the pointer may be invalid on the next frame.
	//If you need to use this component over multiple frames, use get_component_reference().
	template<typename T> inline T* get_component(Entity entity) {
		return get_system<T>()->get_component(entity);
	}

	template<typename T> inline Component_Reference<T> get_component_reference(Entity entity) {
		return get_system<T>()->get_component_reference(entity);
	}

	template<typename T> inline T* add_component(Entity entity) {
		return get_system<T>()->add_component(entity);
	}
};

template<typename T> inline bool has_component(Entity entity) {
	return engine->has_component<T>(entity);
}

//WARNING: Needs to be updated every frame, since the pointer may be invalid on the next frame.
//If you need to use this component over multiple frames, use get_component_reference().
template<typename T> inline T* get_component(Entity entity) {
	return engine->get_component<T>(entity);
}

template<typename T> inline Component_Reference<T> get_component_reference(Entity entity) {
	return engine->get_component_reference<T>(entity);
}

template<typename T> inline T* add_component(Entity entity) {
	return engine->add_component<T>(entity);
}

template<typename T> inline System<T>* get_system() {
	return engine->get_system<T>();
}