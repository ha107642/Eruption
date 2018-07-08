#pragma once

#include "System.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Transform {
	glm::mat4 matrix;
	//glm::vec3 position;
	//glm::vec3 rotation;
	//glm::quat rotation;
	//glm::vec3 scale = { 1, 1, 1 };

	glm::vec3& get_position() { return reinterpret_cast<glm::vec3&>(matrix[3]); }
	void set_position(const glm::vec3& position) { matrix[3] = glm::vec4(position, 1.f); }
};

class Transform_System : public System<Transform> { //Needs a better name... meh
protected:
	void update(Transform* const t, Entity entity, Time& time) override {}
	void update(Time & time) override {}
	void set_state(System_State_Delta<Transform>& delta) override;
};