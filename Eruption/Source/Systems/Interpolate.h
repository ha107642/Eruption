#pragma once

#include "System.h"
#include "Transform_System.h"

struct Interpolation {
	Transform base;
	Transform target;
};

class Interpolate : public System<Interpolation> {
public:
	float amount;
	float snap_time = 0.0f;
	void update(Interpolation* const component, Entity entity, Time& time) override;
	void update(Time & time) override;
	void snap();
	void set_targets(System_State_Delta<Transform>& delta);
};