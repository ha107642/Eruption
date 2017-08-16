#include "Physics.h"

void Physics::update(Hitbox * const component, Entity entity, Time & time) {
	component->velocity->linear.z -= gravity * time.delta_time;
}
