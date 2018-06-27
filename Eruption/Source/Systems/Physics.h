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
	void set_axis(glm::vec3 collision_data, Velocity* velocity);
	void update(Time& time) override;
	std::vector<Component_Holder<Hitbox>> get_hitboxes();
	bool resolve_collision(Component_Holder<Hitbox>& h1, Component_Holder<Hitbox>& h2, glm::vec3* collision_data);
};