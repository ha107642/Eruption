#include "Physics.h"

#include "Engine.h"

void Physics::update(Hitbox * const component, Entity entity, Time & time, Velocity& velocity) {
	velocity.linear.z -= gravity * time.delta_time;
}

void Physics::set_axis(glm::vec3 collision_data, Velocity* velocity) {
	if (collision_data.x > 0.f)
		velocity->linear.x = glm::min(0.f, velocity->linear.x);
	if (collision_data.x < 0.f)
		velocity->linear.x = glm::max(0.f, velocity->linear.x);
	if (collision_data.y > 0.f)
		velocity->linear.y = glm::min(0.f, velocity->linear.y);
	if (collision_data.y < 0.f)
		velocity->linear.y = glm::max(0.f, velocity->linear.y);
	if (collision_data.z > 0.f)
		velocity->linear.z = glm::min(0.f, velocity->linear.z);
	if (collision_data.z < 0.f)
		velocity->linear.z = glm::max(0.f, velocity->linear.z);
}

void Physics::update(Time & time) {
	int count = components.size();
	for (int i = 0; i < count; ++i)
		if (velocities[i])
			update(&components[i].component, components[i].entity, time, *velocities[i]);

	for (int i = 0; i < count; ++i) {
		for (int j = i + 1; j < count; ++j) {
			glm::vec3 collision_data;
			if (components[i].entity == components[j].entity)
				continue;

			bool collision = resolve_collision(i, j, &collision_data);
			if (collision) {
				printf("We have a collision! collision_data: (%f, %f, %f)\n", collision_data.x, collision_data.y, collision_data.z); 
				
				if (velocities[i]) {
					if (velocities[j]) {
						//Move both.
						set_axis(collision_data, velocities[i]);
						set_axis(collision_data, velocities[j]);

						transforms[i]->position -= collision_data * 0.5f;
						transforms[j]->position += collision_data * 0.5f;
					} else {
						//Move i.
						set_axis(collision_data, velocities[i]);
						transforms[i]->position -= collision_data;
					}
				} else if (velocities[j]) {
					//Move j.
					set_axis(collision_data, velocities[j]);
					transforms[j]->position -= collision_data;
				}
			}
		}
	}
}

bool Physics::resolve_collision(const int i1, const int i2, glm::vec3* collision_data) {
	Transform* t1 = transforms[i1];
	Transform* t2 = transforms[i2];
	Hitbox& a = components[i1].component;
	Hitbox& b = components[i2].component;

	glm::vec3 a_pos = a.position + t1->position;
	glm::vec3 b_pos = b.position + t2->position;

	float values[6];
	values[0] = a_pos.x + a.half_size.x - (b_pos.x - b.half_size.x);
	values[1] = a_pos.x - a.half_size.x - (b_pos.x + b.half_size.x);
	values[2] = a_pos.y + a.half_size.y - (b_pos.y - b.half_size.y);
	values[3] = a_pos.y - a.half_size.y - (b_pos.y + b.half_size.y);
	values[4] = a_pos.z + a.half_size.z - (b_pos.z - b.half_size.z);
	values[5] = a_pos.z - a.half_size.z - (b_pos.z + b.half_size.z);

	if (values[0] > 0 &&
		values[1] < 0 &&
		values[2] > 0 &&
		values[3] < 0 &&
		values[4] > 0 &&
		values[5] < 0) {

		float lowest = glm::abs(values[0]);
		int low_index = 0;
		for (int i = 1; i < 6; ++i) {
			if (glm::abs(values[i]) < lowest) {
				lowest = glm::abs(values[i]);
				low_index = i;
			}
		}

		switch (low_index) {
		case 0: collision_data->x = lowest; break;
		case 1: collision_data->x = -lowest; break;
		case 2: collision_data->y = lowest; break;
		case 3: collision_data->y = -lowest; break;
		case 4: collision_data->z = lowest; break;
		case 5: collision_data->z = -lowest; break;
		default:
			break;
		}

		return true;
	}

	return false;
}
