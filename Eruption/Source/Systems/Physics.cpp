#include "Physics.h"

#include "Engine.h"
//#include "../Debugging/DebugWindow.h"

Physics::Physics() : dispatcher(&collision_configuration), overlapping_pair_cache(new btDbvtBroadphase()),
dynamics_world(&dispatcher, overlapping_pair_cache, &solver, &collision_configuration) {
	dynamics_world.setGravity(btVector3(0, -9.82f, 0));
}

Physics::~Physics() {
	for (size_t i = 0; i < components.size(); ++i) {
		delete components[i].component.shape;
		components[i].component.shape = nullptr;
	}
}

Rigidbody * Physics::add_component(Entity entity) {
	Transform* transform = engine->get_component<Transform>(entity);
	btBoxShape* shape = new btBoxShape(reinterpret_cast<const btVector3&>(transform->scale));
	return add_component(entity, *transform, shape, 0.f);
}

Rigidbody * Physics::add_component(const Entity entity, const Transform& offset_transform, btCollisionShape* collider, const float mass) {
	Rigidbody* body = System::add_component(entity);

	body->mass = mass;
	body->shape = collider;
	
	//rigidbody is dynamic if and only if mass is non zero, otherwise static
	bool isDynamic = (body->mass != 0.f);

	btVector3 localInertia(0, 0, 0);
	if (isDynamic)
		body->shape->calculateLocalInertia(body->mass, localInertia);

	//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
	body->motion_state = new Motion_State(entity);
	body->motion_state->offset_transform = offset_transform;
	btRigidBody::btRigidBodyConstructionInfo rigidbody_info(btScalar(body->mass), body->motion_state, body->shape, localInertia);
	body->body = new btRigidBody(rigidbody_info);

	//add the body to the dynamics world
	dynamics_world.addRigidBody(body->body);
	return body;
}

void Physics::update(Time & time) {
	dynamics_world.stepSimulation(time.delta_time);
}