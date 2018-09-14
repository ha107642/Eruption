#pragma once

#include "Systems/Transform_System.h"
#include "Entity.h"

#include <LinearMath/btMotionState.h>

struct Transform;

class Motion_State : public btMotionState {
public:
	Transform offset_transform;
	Entity entity;

	Motion_State() : entity(ENTITY_NULL) {}
	Motion_State(const Entity entity) : entity(entity) {}

	///synchronizes world transform from user to physics
	void getWorldTransform(btTransform& centerOfMassWorldTrans) const override;

	///synchronizes world transform from physics to user
	///Bullet only calls the update of worldtransform for active objects
	void setWorldTransform(const btTransform& centerOfMassWorldTrans) override;
};