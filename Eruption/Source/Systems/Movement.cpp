#include "Movement.h"

#include "Engine.h"

void Movement::update(Time & time) {
	int count = components.size();
	for (int i = 0; i < count; ++i)
		if (transforms[i])
			update(&components[i].component, components[i].entity, time, *transforms[i]);
}

void Movement::update(Velocity * const v, Entity entity, Time & time, Transform& transform) {
	transform.position += v->linear * time.delta_time;
	transform.rotation *= glm::quat(v->angular * time.delta_time); //Do we need to normalize here?
}

void Movement::serialize(Memory_Stream & stream, Velocity & component, Entity entity) {
	write(stream, component.linear);
	write(stream, component.angular);
}

void Movement::deserialize(Memory_Stream & stream, Velocity * component, Entity entity) {
	read(stream, &component->linear);
	read(stream, &component->angular);
}

bool Movement::are_components_equal(Velocity * a, Velocity * b) {
	return a->linear == b->linear && a->angular == b->angular;
}