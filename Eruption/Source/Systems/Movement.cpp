#include "Movement.h"

#include "Engine.h"

void Movement::update(Velocity * const v, Entity entity, Time & time) {
	Transform* transform = v->transform.get(); //~0.14 ms in test (rot calc was removed)
	//Transform* transform = engine->get_component<Transform>(entity); //~0.15 ms in test without even cleaning up the component...
	transform->position += v->linear * time.delta_time;
	//transform->rotation = glm::rotate(transform->rotation, v->angular * time.delta_time, v->up);	
	//transform->rotation = transform->rotation * glm::angleAxis(v->angular * time.delta_time, v->axis);
	//transform->position.x += v->linear.x * time.delta_time; 
	transform->rotation *= glm::quat(v->angular * time.delta_time); //Do we need to normalize here?
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