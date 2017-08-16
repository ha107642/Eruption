#pragma once

#include "System.h"
#include "Systems/Camera.h"

#include <GLFW\glfw3.h>

//extern void key_pressed(GLFWwindow* window, int key, int scancode, int action, int mods);
extern void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

class Input {
private:
	Component_Reference<Camera> camera;
	GLFWwindow* window;
public:
	void initialize(GLFWwindow * window);
	void update();
	bool is_down(int key);
	void handle_scroll(GLFWwindow * window, double x, double y);
	void set_camera_entity(Entity entity);
};