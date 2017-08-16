#pragma once

#include <chrono>

using std::chrono::steady_clock;
using std::chrono::high_resolution_clock;

struct Time {
	//Time since last frame
	float delta_time;
	//Time since the engine was started in engine clock time (excluding paused time).
	float engine_run_time;
	//Time since engine was started in real clock time.
	float real_run_time;
	//Total number of frames
	uint32_t frame_count;
	//Frames per second
	float frames_per_second;
	//Engine time scale
	float time_scale;
	//Real clock time when engine was started.
	steady_clock::time_point start_time;
	//Current real clock time.
	steady_clock::time_point current_time;

	bool is_paused();
	void initialize();
	void update();
};