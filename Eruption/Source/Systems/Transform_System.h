#pragma once

#include "System.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct Transform {
	//glm::mat4 translation;
	glm::vec3 position;
	//glm::vec3 rotation;
	glm::quat rotation;
	glm::vec3 scale = { 1, 1, 1 };
};

class Transform_System : public System<Transform> { //Needs a better name... meh
protected:
	void update(Transform* const t, Entity entity, Time& time) override;
	void update(Time & time) override;
	void set_state(System_State_Delta<Transform>& delta) override;
public:
	//System_State_Delta<Transform> get_delta_state(System_State * base);
};