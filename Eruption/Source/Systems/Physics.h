#pragma once

#include "System.h"
#include "Movement.h"
#include "../Motion_State.h"

#include <glm/glm.hpp>
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

struct Rigidbody {
	btRigidBody* body;
	btCollisionShape* shape;
	Motion_State* motion_state;
	float mass;
};

class Physics : public System<Rigidbody> {
private:
	///collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
	btDefaultCollisionConfiguration collision_configuration;
	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	btCollisionDispatcher dispatcher;
	///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
	btBroadphaseInterface* overlapping_pair_cache;
	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	btSequentialImpulseConstraintSolver solver;
	btDiscreteDynamicsWorld dynamics_world;

public:
	Physics();
	~Physics();

	void update(Time& time) override;
	Rigidbody * Physics::add_component(Entity entity) override;
	Rigidbody * Physics::add_component(const Entity entity, const Transform& transform, btCollisionShape* collider, const float mass);
};