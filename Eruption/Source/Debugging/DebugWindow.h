#pragma once

class ISystem;
struct Time;

namespace Debugging {
	void render_debug_window(const Time &time);
	void system_timing_start(ISystem* system);
	void system_timing_stop(ISystem* system);
	void debug_register_system(ISystem *system, const char* system_name = nullptr);
};