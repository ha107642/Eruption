#pragma once

#include "System.h"
#include "Transform_System.h"
#include "../System_Reference.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct Velocity {
	glm::vec3 linear;
	glm::vec3 angular;
};

class Movement : public System<Velocity> {
private:
	System_Reference<Velocity, Transform> transforms;
	
	void update(Velocity* const v, Entity entity, Time& time, Transform& transform);
public:
	Movement() : transforms(*this) {}

	void update(Time& time) override;
	void serialize(Memory_Stream &stream, Velocity& component, Entity entity) override;
	void deserialize(Memory_Stream &stream, Velocity* component, Entity entity) override;
	bool are_components_equal(Velocity * a, Velocity * b) override;
};