#pragma once

class ISystem;
struct Time;

namespace Debugging {
	struct Timing_Data;

	void render_debug_window(const Time &time);

	Timing_Data& timing_start(char* name);
	void timing_stop(Timing_Data& timing);
	void system_timing_start(ISystem* system);
	void system_timing_stop(ISystem* system);
	void debug_register_system(ISystem *system, const char* system_name = nullptr);
};