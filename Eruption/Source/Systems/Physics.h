#pragma once

#include "System.h"
#include "Movement.h"

#include <glm/glm.hpp>

struct Hitbox {
	glm::vec3 position;
	glm::vec3 half_size;

	Component_Reference<Velocity> velocity;
};

class Physics : public System<Hitbox> {
private:
	float gravity = .982f;
public:
	void update(Hitbox* const component, Entity entity, Time& time) override;
};