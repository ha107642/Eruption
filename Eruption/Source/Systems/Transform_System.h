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
	std::vector<Component_Holder<Transform>> target_components;
	float interp_time = -1.0f;

	void update(Transform* const t, Entity entity, Time& time) override;
	void interpolate(Transform * const current, Transform * const target, float amount);
	void update(Time & time) override;
	void set_state(System_State state) override;
	void set_state(System_State_Delta<Transform>& delta) override;
	void delete_component(Entity entity) override;
public:
	//System_State_Delta<Transform> get_delta_state(System_State * base);
};