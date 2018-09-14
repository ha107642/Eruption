#include "Motion_State.h"

#include "Engine.h"

///synchronizes world transform from user to physics
void Motion_State::getWorldTransform(btTransform& centerOfMassWorldTrans) const {
	Transform* t = get_component<Transform>(entity);
	centerOfMassWorldTrans.setOrigin(btVector3(t->position.x, t->position.y, t->position.z));
	centerOfMassWorldTrans.setRotation(btQuaternion(t->rotation.x, t->rotation.y, t->rotation.z, t->rotation.w));
}

///synchronizes world transform from physics to user
///Bullet only calls the update of worldtransform for active objects
void Motion_State::setWorldTransform(const btTransform& centerOfMassWorldTrans) {
	Transform* t = get_component<Transform>(entity);
	const btVector3& origin = centerOfMassWorldTrans.getOrigin();
	t->position = glm::vec3(origin.x(), origin.y(), origin.z());
	const btQuaternion& rotation = centerOfMassWorldTrans.getRotation();
	t->rotation.x = rotation.x();
	t->rotation.y = rotation.y();
	t->rotation.z = rotation.z();
	t->rotation.w = rotation.w();
}
