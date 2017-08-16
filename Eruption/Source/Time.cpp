#include "Time.h"

#include <algorithm>

inline bool Time::is_paused() { return time_scale == 0.0f; }

void Time::initialize() {
	start_time = high_resolution_clock::now();
	current_time = start_time;
	engine_run_time = 0.0f;
	real_run_time = 0.0f;
	time_scale = 1.0f;
	frame_count = 0;
	frames_per_second = 0;
}

void Time::update() {
	frame_count++;
	delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - current_time).count() * 0.001f;
	real_run_time += delta_time;

	float current_fps = 1.0f / std::max(delta_time, 0.00000001f);
	frames_per_second = frames_per_second * 0.5f + current_fps * 0.5f;

	delta_time *= time_scale;
	engine_run_time += delta_time * time_scale;

	current_time = high_resolution_clock::now();
}