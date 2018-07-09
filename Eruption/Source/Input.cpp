#include "Input.h"

#include  "Engine.h"

void Input::update() {
	glfwPollEvents();

	if (camera.is_initialized()) {
		const float pan_speed = 10.f;
		glm::vec3 velocity;

		if (is_down(GLFW_KEY_W)) velocity.y += pan_speed;
		if (is_down(GLFW_KEY_S)) velocity.y -= pan_speed;
		if (is_down(GLFW_KEY_D)) velocity.x += pan_speed;
		if (is_down(GLFW_KEY_A)) velocity.x -= pan_speed;

		camera_velocity->linear = velocity;
	}
}

bool Input::is_down(int key) {
	return glfwGetKey(window, key) == GLFW_PRESS;
}

void Input::handle_scroll(GLFWwindow * window, double x, double y) {
	const float zoom_speed = 1.f;
	
	if (camera.is_initialized()) {
		camera->target_zoom = glm::clamp(camera->target_zoom - (float)(y * zoom_speed), -5.f, 25.f);
		printf("scroll received: (%f, %f). zoomm = %f\n", x, y, camera->zoom);
	}
}

void Input::set_camera_entity(Entity entity) {
	camera = engine->get_component_reference<Camera>(entity);
	camera_velocity = engine->get_component_reference<Velocity>(entity);
}

void Input::initialize(GLFWwindow* window) {
	//glfwSetKeyCallback(window, key_pressed);
	glfwSetScrollCallback(window, scroll_callback);
	this->window = window;
}


//The action is one of GLFW_PRESS, GLFW_REPEAT or GLFW_RELEASE
inline void key_pressed(GLFWwindow* window, int key, int scancode, int action, int mods) {
//	engine->input.handle_keyboard_input(window, key, scancode, action, mods);	
}

//The action is one of GLFW_PRESS, GLFW_REPEAT or GLFW_RELEASE
inline void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	engine->input.handle_scroll(window, xoffset, yoffset);
}