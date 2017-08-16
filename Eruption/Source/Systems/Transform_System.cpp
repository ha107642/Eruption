#include "Transform_System.h"

#include "Engine.h"
#include "Interpolate.h"

#include <iostream>
#include <sstream>


void Transform_System::update(Transform* const t, Entity entity, Time& time) {
	if (!engine->is_authoritative())
		return;
	
	if (entity == 1) return;
	//glm::mat4 translation = glm::scale(t->scale) * glm::rotate(0.0f, t->rotation) * glm::translate(t->position);
	//t->position.x = glm::sin(time.engine_run_time);
	//t->position.y = glm::sin(time.engine_run_time);
	//glm::quat rot = glm::angleAxis(time.delta_time, glm::vec3(0.f, 0.f, 1.f));
	//t->rotation *= rot;
	//t->rotation.x = 0.1f * glm::sin(time.delta_time);
	//t->rotation.y += time.delta_time;
}

void Transform_System::interpolate(Transform* const current, Transform* const target, float amount) {
	//Component_Holder<Transform>* holder = reinterpret_cast<Component_Holder<Transform>*>((uint32_t*)current - 1);
	//if (holder->entity == 1) {
	//	if (engine->is_authoritative())
	//		printf("Server Camera pos: { %f, %f, %f }\r\n", current->position.x, current->position.y, current->position.z);
	//	else
	//		printf("Client Camera pos: { %f, %f, %f }\r\n", current->position.x, current->position.y, current->position.z);
	//}
	//
	//current->position = glm::mix(current->position,	  target->position, amount);
	//current->rotation = glm::slerp(current->rotation, target->rotation, amount);
	//current->scale    = glm::mix(current->scale,      target->scale,    amount); 
}

void Transform_System::update(Time& time) {	
	int count = components.size();
	if (engine->is_authoritative()) {
		for (int i = 0; i < count; ++i)
			update(&components[i].component, components[i].entity, time);
	} else {
		if (interp_time < 0.0f)
			return;

		float amount = (time.delta_time / ((1 / (float)SERVER_FPS) - interp_time));

		//float b = last_amount;
		//float a = b + time.delta_time;
		//float server_frame_rate = (1.0f / (float)SERVER_FPS);
		//float b = last_amount;
		//float a = last_amount + (SERVER_FPS / time.frames_per_second);
		//float amount = (float)SERVER_FPS / time.frames_per_second;

		//interp_time = glm::clamp(interp_time + time.delta_time, 0.0f, 1.0f);
		//amount = glm::clamp(amount, 0.0f, 1.0f);//float amount = (time.engine_run_time - packet_time) / (target_time - packet_time); //meh...
		//printf("amount = %f, interp_time = %f\n", amount, interp_time);
		for (int i = 0; i < count; ++i) {
			//components = target_components;
			interpolate(&components[i].component, &target_components[i].component, amount);
			//update(&components[i].component, components[i].entity, time);
		}
	}
}

void Transform_System::set_state(System_State state) {
	System<Transform>::set_state(state);
	target_components = components;
}

void Transform_System::set_state(System_State_Delta<Transform> &delta) {
	interp_time = 0.0f;
	int count = delta.deleted.size();
	for (int i = count - 1; i >= 0; --i) {
		delete_component(delta.deleted[i]);
	}

	//if (!engine->is_authoritative()) {
	//	Interpolate* interp_system = (Interpolate*)engine->get_system<Interpolation>();

	//	for (int i = count - 1; i >= 0; --i) {
	//		if (interp_system->has_component(delta.deleted[i]))
	//			interp_system->delete_component(delta.deleted[i]);
	//	}
	//}

	count = delta.changed.size();
	if (engine->is_authoritative()) {
		for (int i = 0; i < count; ++i) {
			Transform* t = &(components.at(map.at(delta.changed[i].entity)).component);
			*t = delta.changed[i].component;
		}
	} else { 
		//As clients, we want to interpolate between components and target_components.
		//Thus, we only change the target for now (and let update interpolate).
		Interpolate* interp_system = (Interpolate*)engine->get_system<Interpolation>();
		interp_system->set_targets(delta);
		//interp_system->snap();
		//for (int i = 0; i < count; ++i) {
		//	Interpolation* interpolation = interp_system->add_component(delta.changed[i].entity);
		//	interpolation->base = *get_component(delta.changed[i].entity);
		//	interpolation->target = delta.changed[i].component;

			/*Transform* t = &(components_to_change->at(map.at(delta.changed[i].entity)).component);
			*t = delta.changed[i].component;*/
		//}
	}

	//for (int i = 0; i < count; ++i) {
	//	Interpolation* interpolation = interp_system->add_component(delta.changed[i].entity);
	//	interpolation->current = get_component_reference(delta.changed[i].entity);
	//	interpolation->base = *interpolation->current.get();
	//	interpolation->target = delta.changed[i].component;

	//	/*Transform* t = &(components_to_change->at(map.at(delta.changed[i].entity)).component);
	//	*t = delta.changed[i].component;*/
	//}

	count = delta.added.size();
	for (int i = 0; i < count; ++i) {
		Transform* t = add_component(delta.added[i].entity);
		*t = delta.added[i].component;

		if (!engine->is_authoritative()) {
			//Do the same thing for target_components as in components.
			Component_Holder<Transform> holder = {};
			holder.entity = delta.added[i].entity;
			holder.component = *t;
			target_components.push_back(holder);
		}
	}
}

void Transform_System::delete_component(Entity entity) {
	int index = map[entity];
	System<Transform>::delete_component(entity);
	
	if (!engine->is_authoritative()) {
		//Do the same thing for target_components as in components.
		Component_Holder<Transform> &index_ref = target_components[index];
		Component_Holder<Transform> &last_ref = target_components[components.size() - 1];
		std::swap(index_ref, last_ref);
		target_components.pop_back();
	}
}