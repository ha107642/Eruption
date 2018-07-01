#include "Camera.h"

#include "Graphics/Graphics.h"
#include "Engine.h"
#include "Movement.h"

Camera* Camera_System::add_component(Entity entity) {
	Camera* c = System<Camera>::add_component(entity);
	engine->set_main_camera(entity, c);
	return c;
}

void Camera_System::set_state(System_State state) {
	System<Camera>::set_state(state);
	if (components.size() > 0)
		engine->set_main_camera(components[0].entity, &components[0].component);
}

void Camera_System::update(Camera * const camera, Entity entity, Time & time) { 
	const float easing_speed = 10.f;
	if (glm::abs(camera->zoom - camera->target_zoom) < 0.01f)
		camera->zoom = camera->target_zoom;
	else
		camera->zoom = glm::mix(camera->zoom, camera->target_zoom, time.delta_time * easing_speed);
}

void Camera_System::serialize(Memory_Stream & stream, Camera & component, Entity entity) {
	write(stream, component.zoom);
}

void Camera_System::deserialize(Memory_Stream & stream, Camera * component, Entity entity) {
	read(stream, &component->zoom);
}

bool Camera_System::are_components_equal(Camera * a, Camera * b) {
	return a->zoom == b->zoom && a->target_zoom == b->target_zoom;
}
//
//System_State_Delta<Camera> Camera_System::get_delta_state(System_State & base) {
//	System_State_Delta<Camera> delta = System<Camera>::get_delta_state(base);
//	//delta.changed.clear(); //Don't send the server's data to the clients here.
//	return delta;
//}