#include "Transform_System.h"

#include "Engine.h"
#include "Interpolate.h"

#include <iostream>
#include <sstream>


glm::mat4 Transform::to_matrix() {
	glm::mat4 matrix = glm::toMat4(rotation);
	matrix[0] *= scale.x;
	matrix[1] *= scale.y;
	matrix[2] *= scale.z;
	*reinterpret_cast<glm::vec3*>(&matrix[3]) = position;
	return matrix;
};

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

void Transform_System::update(Time& time) {	
	int count = components.size();
	if (engine->is_authoritative()) {
		for (int i = 0; i < count; ++i)
			update(&components[i].component, components[i].entity, time);
	}
}

void Transform_System::set_state(System_State_Delta<Transform> &delta) {
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
	}
}