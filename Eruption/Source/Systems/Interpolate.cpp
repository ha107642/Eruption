#include "Interpolate.h"

#include "Engine.h"

void Interpolate::update(Interpolation * const component, Entity entity, Time & time) {
	Transform* current = engine->get_component<Transform>(entity);	
	current->position = glm::mix(component->base.position, component->target.position, amount);
	current->rotation = glm::mix(component->base.rotation, component->target.rotation, amount);
	current->scale    = glm::mix(component->base.scale,    component->target.scale,    amount);
}

void Interpolate::update(Time &time) {
	amount = (engine->time.engine_run_time - snap_time) / (1.f / (float)SERVER_FPS);
	System<Interpolation>::update(time);
}

void Interpolate::snap() {
	int count = components.size();
	Transform_System* transform_system = (Transform_System*)engine->get_system<Transform>();
	for (int i = count - 1; i >= 0; --i) {
		Transform* current = transform_system->get_component(components[i].entity);
		*current = components[i].component.target;
		delete_component(components[i].entity);
	}
	snap_time = engine->time.engine_run_time;
}

void Interpolate::set_targets(System_State_Delta<Transform> &delta) {
	int count = delta.deleted.size();
	for (int i = count - 1; i >= 0; --i) {
		if (has_component(delta.deleted[i]))
			delete_component(delta.deleted[i]);
	}

	snap();

	count = delta.changed.size();
	for (int i = 0; i < count; ++i) {
		Entity entity = delta.changed[i].entity;
		Interpolation* interpolation;
		//if (has_component(entity)) {
		//	interpolation = get_component(entity);
		//} else {
			interpolation = add_component(entity);
		//}

		interpolation->base = *engine->get_component<Transform>(entity);
		interpolation->target = delta.changed[i].component;
	}

	//snap_time = engine->time.engine_run_time;*/
}