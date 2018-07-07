#include "Movement.h"

#include "Engine.h"
#include <glm/gtx/euler_angles.hpp>

void Movement::update(Velocity * const v, Entity entity, Time & time) {
	Transform* transform = v->transform.get();
	transform->matrix = glm::translate(transform->matrix, v->linear * time.delta_time);
	transform->matrix *= glm::orientate4(v->angular * time.delta_time); //TODO: order?
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