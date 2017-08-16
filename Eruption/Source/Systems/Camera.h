#pragma once

#include "System.h"
#include "Movement.h"

struct Camera {
	float zoom = 5.f;
	float target_zoom = 5.f;
	Component_Reference<Velocity> velocity;
};

class Camera_System : public System<Camera> {
public:
	Camera* add_component(Entity entity) override;
	void set_state(System_State state) override;
	void update(Camera* const camera, Entity entity, Time& time) override;
	void serialize(Memory_Stream &stream, Camera& component, Entity entity) override;
	void deserialize(Memory_Stream &stream, Camera* component, Entity entity) override;
	bool are_components_equal(Camera* a, Camera* b) override;
	//System_State_Delta<Camera> get_delta_state(System_State &base) override;
};