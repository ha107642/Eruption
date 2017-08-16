#pragma once

#include "System.h"
#include "Transform_System.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct Velocity {
	glm::vec3 linear;
	glm::vec3 angular; //The magnitude represents the speed..
	//float angular;
	//glm::vec3 axis = { 0.f, 0.0f, 1.f };
	//glm::quat angular = { 1.0f, 0.f, 0.f, 0.f };

	Component_Reference<Transform> transform;
};

class Movement : public System<Velocity> {
	void update(Velocity* const v, Entity entity, Time& time) override;
	void serialize(Memory_Stream &stream, Velocity& component, Entity entity) override;
	void deserialize(Memory_Stream &stream, Velocity* component, Entity entity) override;
	bool are_components_equal(Velocity * a, Velocity * b) override;
};