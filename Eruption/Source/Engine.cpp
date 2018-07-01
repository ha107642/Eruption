#include "Engine.h"

#include "State.h"
#include "Network/Network.h"
#include "Network/Server.h"
#include "Network/Client.h"
#include "Systems/Transform_System.h"
#include "Systems/Renderer.h"
#include "Systems/Audio.h"
#include "Systems/Camera.h"
#include "Systems/Movement.h"
#include "Systems/Interpolate.h"
#include "Systems/Physics.h"

#include <stdarg.h>
#include <chrono>
#include <thread>

thread_local Engine *engine;

void Engine::fail(const char* reason, ...) {
	fprintf(stderr, "Error: ");
	va_list va;
	va_start(va, reason);
	vfprintf(stderr, reason, va);
	va_end(va);

	//std::string dummy;
	//std::getline(std::cin, dummy);
	throw std::runtime_error(reason);
	exit(EXIT_FAILURE);
}

void Engine::add_entity_component(Entity entity, ISystem * system) {
	entity_components[entity].push_back(system);
}

void Engine::remove_entity_component(Entity entity, ISystem * system) {
	std::vector<ISystem*> &systems = entity_components.at(entity);
	for (uint32_t i = 0; i < systems.size(); ++i) {
		if (systems[i] == system) {
			systems.erase(systems.begin() + i);
			break;
		}
	}

	if (systems.size() == 0)
		entity_components.erase(entity);
}

bool Engine::should_exit() {
	return graphics.should_exit();
}

Entity house, house2, hero,goon1, goon2, goon3, sidekick, last_entity;
State state;
Render* hr;
Transform* ht;
Component_Reference<Transform> t;
void init_some_stuff(Engine& engine, Transform_System &movement, Renderer &renderer, Model* model) {
	house = engine.new_entity();
	ht = engine.add_component<Transform>(house);
	ht->position.x = 1;
	ht->scale.x = 1.0f;
	ht->scale.y = 1.0f;
	ht->scale.z = 1.0f;

	hr = engine.add_component<Render>(house);
	hr->transform = movement.get_component_reference(house);
	hr->transform->position.z = 10.f;
	if (model) hr->model = model;

	Velocity* hv = engine.add_component<Velocity>(house);
	hv->angular.z = 2.f;
	hv->linear.x = 2.f;

	Hitbox* hb = add_component<Hitbox>(house);
	hb->half_size = glm::vec3(1.f, 1.f, 1.f);

	house2 = engine.new_entity();
	ht = engine.add_component<Transform>(house2);
	ht->position.z = 1;
	ht->scale = { 0.5f, 0.5f, 0.5f };

	hv = engine.add_component<Velocity>(house2);
	hv->linear.x = 2.f;

	hr = engine.add_component<Render>(house2);
	hr->transform = movement.get_component_reference(house2);
	if (model) hr->model = model;

	hero = engine.new_entity();
	engine.add_component<Transform>(hero);
	t = movement.get_component_reference(hero);
	t->position.x = 1;
	t->position.z = 100;
	sidekick = engine.new_entity(hero);
	engine.get_component<Transform>(sidekick)->position.y = 123;

	State state;

	goon1 = engine.new_entity();
	engine.add_component<Transform>(goon1);
	goon2 = engine.new_entity(goon1);
	goon3 = engine.new_entity(goon1);

	Entity ground = engine.new_entity();
	Transform* tr = add_component<Transform>(ground);
	tr->position.z = -1;
	Render* gr = add_component<Render>(ground);
	gr->transform = get_component_reference<Transform>(ground);
	gr->model = new Model();
	gr->model->name = "ground_zx";

	Hitbox* hb2 = add_component<Hitbox>(ground);
	hb2->half_size = glm::vec3(100.f, 100.f, 1.f);
}

void do_some_stuff(Engine& engine, Transform_System &movement, Renderer &renderer, Time &time, Graphics *graphics = nullptr) {
	Transform* tra = get_component<Transform>(house);
	if (tra->position.x > 5.f) {
		tra->position.x = -5.f;
		get_component<Transform>(house2)->position.x = -5.f;
	}


	if (time.frame_count == 3) {
		t->position.x = 5;
		t->position.y = 30;
		engine.get_component<Transform>(house2)->position.z = -0.5f;
	}
	if (time.frame_count == 5) {
		engine.destroy_entity(&hero);
	}
	if (time.frame_count == 10) {
		engine.destroy_entity(&sidekick);
		engine.destroy_entity(&goon1);
		engine.destroy_entity(&goon2);
		engine.destroy_entity(&goon3);
	}

	if (time.frame_count == 1) {
		state = engine.get_state();
		//engine.set_state(state);
	}
	if (time.frame_count == 300) {
		engine.set_state(state);
		//movement.set_state(delta);
		//time.time_scale = 0.3f;
		
		Audio* audio = (Audio*)engine.get_system<Audio_Source>();
		
		Audio_Source* sound = add_component<Audio_Source>(house2);
		sound->id = audio->create_audio_source(SOUND_CHANNEL_EFFECTS, false);
		audio->play(sound);
	}

	if (time.frame_count > 350 && time.frame_count < 450) {
		
		Model* labo;
		if (graphics) {
			labo = graphics->load_model("plant");
			graphics->load_texture(labo, "plant");
		}
		else {
			labo = new Model();
			labo->name = "plant";
		}
		

		Entity house3 = engine.new_entity();
		ht = engine.add_component<Transform>(house3);
		ht->position.z = 0.3f;
		ht->position.y = (time.frame_count - 400.0f) * 0.1f;
		ht->scale = { 0.7f, 0.5f, 1.0f };

		hr = engine.add_component<Render>(house3);
		hr->transform = movement.get_component_reference(house3);
		hr->model = labo;
	}
	if (time.frame_count == 500) {
		engine.set_state(state);
	}
	if (time.frame_count > 500) {
		static Model *model = new Model();
		model->name = "cube";

		Entity new_ent = engine.new_entity();
		Transform* t = engine.add_component<Transform>(new_ent);
		t->position.z = 3.0f;
		engine.add_component<Render>(new_ent)->model = model;
		if (last_entity != ENTITY_NULL)
			engine.destroy_entity(&last_entity);
		last_entity = new_ent;
	}
}

void Engine::run() {
	_is_authoritative = true;
	graphics.initialize(this);
	input.initialize(graphics.window);

	Model* model = graphics.load_model("chalet");
	graphics.load_texture(model, "chalet");

	Transform_System tranform;
	register_system(&tranform);

	Movement movement;
	register_system(&movement);

	Renderer renderer(&graphics);
	register_system(&renderer);	
	
	Audio audio(false);
	register_system(&audio);

	init_some_stuff(*this, tranform, renderer, model);

	time.initialize();
	while (!should_exit()) {
		time.update();
		//std::cout << "FPS: " << time.frames_per_second <<  ", frame_count: " << time.frame_count << std::endl;
		input.update();

		do_some_stuff(*this, tranform, renderer, time, &graphics);
		update_systems(time);

		graphics.draw(time);
	}
}

void Engine::run_client() {
	_is_authoritative = false;
	graphics.initialize(this);
	input.initialize(graphics.window);
	Client client;
	client.initialize();
	client.connect("localhost", 1234);

	Model* model = graphics.load_model("chalet");
	graphics.load_texture(model, "chalet");

	Model* plant = graphics.load_model("plant");
	graphics.load_texture(plant, "plant");

	Model *cube = graphics.load_model("cube");
	graphics.load_texture(cube, "cube");
	
	Model *ground = graphics.load_model("ground_zx");
	graphics.load_texture(ground, "ground");

	Transform_System transform;
	register_system(&transform);

	Movement movement;
	register_system(&movement);

	Renderer renderer(&graphics);
	register_system(&renderer);

	Audio audio(false);
	register_system(&audio);

	Camera_System camera;
	register_system(&camera);

	Interpolate interp;
	register_system(&interp);

	Physics physics;
	register_system(&physics);

	//System_Reference<Velocity, Transform> test(movement);

	//we just need some dummy entity to render
	//the graphics renderer does not like when we initialize
	//stuff with nothing to render.
	Entity dummy = new_entity();
	ht = add_component<Transform>(dummy);	
	hr = add_component<Render>(dummy);
	hr->transform = transform.get_component_reference(dummy);
	hr->model = graphics.load_model("chalet");// model;

	time.initialize();
	while (!should_exit()) {
		time.update();
		//std::cout << "FPS: " << time.frames_per_second <<  ", frame_count: " << time.frame_count << std::endl;

		client.update();
		input.update();
		//do_some_stuff(movement, renderer, time, &graphics);
		update_systems(time);
		graphics.draw(time);
		client.flush();
		//std::this_thread::sleep_for(std::chrono::seconds(10));
	}
}

void Engine::run_server(bool *should_exit) {
	const std::chrono::nanoseconds FRAME_TIME(1000000000 / SERVER_FPS);
	Server server;

	_is_authoritative = true;

	server.initialize();
	time.initialize();

	Transform_System transform_system;
	register_system(&transform_system);

	Movement movement;
	register_system(&movement);

	Renderer renderer;
	register_system(&renderer);

	Audio audio(true);
	register_system(&audio);

	Camera_System camera;
	register_system(&camera);

	Interpolate interp;
	register_system(&interp);

	Physics physics;
	register_system(&physics);

	Entity main_camera = new_entity();
	add_component<Transform>(main_camera);
	Velocity* v = add_component<Velocity>(main_camera);
	Camera* c = add_component<Camera>(main_camera);
	c->target_zoom = 5.0f;
	graphics.set_main_camera(c);

	Model *model = new Model();
	model->name = "chalet";
	init_some_stuff(*this, transform_system, renderer, model);
	
	if (!should_exit) {
		should_exit = new bool();
		*should_exit = false;
	}

	while (!*should_exit) {
		time.update();

		server.update();
		do_some_stuff(*this, transform_system, renderer, time);
		update_systems(time);
		State state = get_state();
		//System_State_Delta = get_state_delta();
		server.broadcast_state(state);
		server.flush();

		std::chrono::nanoseconds update_time = high_resolution_clock::now() - time.current_time;
		std::chrono::nanoseconds sleep_time = FRAME_TIME - update_time;
		std::this_thread::sleep_for(sleep_time);
		//printf("Server: update time: %f s, frame time: %f s\n", (float)update_time.count() / (float)1000000000, (float)(high_resolution_clock::now() - time.current_time).count() / (float)1000000000);
	}
}

void Engine::set_main_camera(Entity entity, Camera * camera) {
	if (graphics.is_initialized()) 
		graphics.set_main_camera(camera);
	input.set_camera_entity(entity);
}

void Engine::resize_window(int width, int height) {
	graphics.resize(width, height);
}

void Engine::set_delta_state(Memory_Stream &stream) {
	uint8_t count;
	read(stream, &count);
	for (uint8_t i = 0; i < count; ++i) {
		uint8_t system_id;
		read(stream, &system_id);
		ISystem* system = systems[system_id];
		system->deserialize_delta_state(stream);
	}
}

void Engine::serialize_delta_state(Memory_Stream &stream, State &base_state) {
	uint8_t count = (uint8_t)base_state.states.size();
	write(stream, count);
	for (uint8_t i = 0; i < count; ++i) {
		write(stream, base_state.states[i].system_id);
		ISystem* system = systems[base_state.states[i].system_id];
		system->serialize_delta_state(stream, base_state.states[i]);
	}
}

State Engine::get_state() {
	int count = systems.size();
	std::vector<System_State> states(count);
	for (int i = 0; i < count; ++i) {
		states[i] = systems[i]->get_state();
	}

	return { engine->time.frame_count, last_entity, states, entity_components };
}

void Engine::set_state(State &state) {
	int count = state.states.size();
	for (int i = 0; i < count; ++i) {
		systems[state.states[i].system_id]->set_state(state.states[i]);
	}
	last_entity = state.last_entity;
	entity_components = state.entity_components;
}

inline void Engine::update_systems(Time &time) {
	int count = systems.size();
	for (int i = 0; i < count; ++i)
		systems[i]->update(time);
}

Entity Engine::new_entity() {
	return ++last_entity;
}

Entity Engine::new_entity(Entity copy_from) {
	Entity entity = ++last_entity;
	std::vector<ISystem*> *entity_systems = &(entity_components.at(copy_from));
	int size = entity_systems->size();
	for (int i = 0; i < size; ++i) {
		(*entity_systems)[i]->clone_component(copy_from, entity);
	}

	return entity;
}

void Engine::destroy_entity(Entity * entity) {
	//Destroy all of its components.
	std::vector<ISystem*> *entity_systems = &(entity_components.at(*entity));
	int size = entity_systems->size();
	for (int i = size - 1; i >= 0; --i) {
		(*entity_systems)[i]->delete_component(*entity);
		//entity_systems->pop_back();
	}
	//Destroy its entry in the component list, and null the entity.
	//entity_components.erase(*entity);
	*entity = ENTITY_NULL;
}
