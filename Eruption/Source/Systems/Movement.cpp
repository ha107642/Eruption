#include "Movement.h"

#include "Engine.h"
#include <glm/gtx/euler_angles.hpp>

void Movement::update(Velocity * const v, Entity entity, Time & time) {
	Transform* transform = v->transform.get(); //~1 ms in test (rot calc was removed)
	//Transform* transform = engine->get_component<Transform>(entity); //~6 ms in test	
	glm::vec3& pos = transform->get_position();
	glm::vec3 new_pos = pos + v->linear * time.delta_time;
	transform->set_position(new_pos);
	if (glm::abs(v->angular.x) > FLT_EPSILON || glm::abs(v->angular.y) > FLT_EPSILON || glm::abs(v->angular.z) > FLT_EPSILON) {
		glm::vec3 velocity(v->angular.x, v->angular.z, v->angular.y);
		transform->matrix = transform->matrix * glm::orientate4(velocity * time.delta_time);
	}
}

void Movement::serialize(Memory_Stream & stream, Velocity & component, Entity entity) {
	write(stream, component.linear);
	write(stream, component.angular);
	//write(stream, component.up);
}

void Movement::deserialize(Memory_Stream & stream, Velocity * component, Entity entity) {
	read(stream, &component->linear);
	read(stream, &component->angular);
	//read(stream, &component->up);
	component->transform = engine->get_component_reference<Transform>(entity);
}

bool Movement::are_components_equal(Velocity * a, Velocity * b) {
	return a->linear == b->linear && a->angular == b->angular;
}