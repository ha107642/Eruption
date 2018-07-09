#pragma once

#include "System.h"
#include "Movement.h"

#include <glm/glm.hpp>

struct Hitbox {
	glm::vec3 position;
	glm::vec3 half_size;
};

class Physics : public System<Hitbox> {
private:
	float gravity = .982f;
	System_Reference<Hitbox, Velocity> velocities;
	System_Reference<Hitbox, Transform> transforms;

	void update(Hitbox* const component, Entity entity, Time& time, Velocity& velocity);
public:
	Physics() : velocities(*this), transforms(*this) {}

	void set_axis(glm::vec3 collision_data, Velocity* velocity);
	void update(Time& time) override;
	bool Physics::resolve_collision(const int i1, const int i2, glm::vec3* collision_data);
};