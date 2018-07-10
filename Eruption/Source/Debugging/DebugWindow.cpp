#include "DebugWindow.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include "../System.h"
#include "../Time.h"

namespace Debugging {
	struct Timing_Data {
		char* name;
		float time;
		steady_clock::time_point start_time;
		steady_clock::time_point end_time;
		float average_time;
	};

	struct System_Timing_Data {
		Timing_Data timing;
		ISystem* system;
	};

	std::vector<Timing_Data> timings;
	std::vector<System_Timing_Data> system_timings;
	int longest_system_name;
	bool debug_window_active;

	void render_debug_window(const Time &time) {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z), false))
			debug_window_active = !debug_window_active;

		if (!debug_window_active) {
			ImGui::Render();
			return;
		}
		//static bool open_the_damn_window = true;
		//ImGui::ShowDemoWindow(&open_the_damn_window);

		if (ImGui::Begin("System Timings", &debug_window_active)) {
			ImGui::Text("Frame time: %.2f ms", time.delta_time * 1000.f);
			ImGui::Text("FPS: %.2f", time.frames_per_second);

			for (System_Timing_Data& data : system_timings) {
				data.timing.average_time = data.timing.average_time * 0.5f + data.timing.time * 0.5f;
				ImGui::Text("%-*s: %.2f ms [%d]", longest_system_name, data.timing.name, data.timing.average_time * 1000.f, data.system->get_component_count());
				data.timing.time = 0.f;
			}

			for (Timing_Data& data : timings) {
				data.average_time = data.average_time * 0.5f + data.time * 0.5f;
				ImGui::Text("%-*s: %.2f ms", longest_system_name, data.name, data.average_time * 1000.f);
				data.time = 0.f;
			}

			ImGui::Text("Want to use mouse: %d", ImGui::GetIO().WantCaptureMouse);
			ImGui::Text("Want to use keyboard: %d", ImGui::GetIO().WantCaptureKeyboard);
		}

		ImGui::End();
		
		ImGui::Render();
	}


	Timing_Data& timing_start(char* name) {
		Timing_Data* timing = nullptr;
		for (size_t i = 0; i < timings.size(); ++i) {
			if (timings[i].name == name) {
				timing = &timings[i];
				break;
			}
		}
		if (!timing) {
			timings.emplace_back();
			timing = &timings.back();
			timing->name = name;
			timing->time = 0.f;

			int name_length = strlen(name);
			if (name_length > longest_system_name)
				longest_system_name = name_length;
		}

		timing->start_time = high_resolution_clock::now();
		return *timing;
	}

	Timing_Data& timing_start(Timing_Data& timing) {
		timing.start_time = high_resolution_clock::now();
		return timing;
	}

	void timing_stop(Timing_Data& timing) {
		timing.end_time = high_resolution_clock::now();
		timing.time += std::chrono::duration_cast<std::chrono::microseconds>(timing.end_time - timing.start_time).count() * 0.000001f;
	}

	void system_timing_start(ISystem * system) {
		uint8_t system_id = system->get_system_id();

		system_timings[system_id].timing.start_time = high_resolution_clock::now();
	}

	void system_timing_stop(ISystem * system) {
		uint8_t system_id = system->get_system_id();
		system_timings[system_id].timing.end_time = high_resolution_clock::now();
		system_timings[system_id].timing.time += std::chrono::duration_cast<std::chrono::microseconds>(system_timings[system_id].timing.end_time - system_timings[system_id].timing.start_time).count() * 0.000001f;
	}

	void debug_register_system(ISystem * system, const char * system_name) {
		System_Timing_Data data = {};
		data.system = system;

		int name_length = strlen(system_name) + 5;
		data.timing.name = new char[name_length];
		sprintf_s(data.timing.name, name_length, "%s (%d)", system_name, system->get_system_id());
		if (name_length > longest_system_name)
			longest_system_name = name_length;
		
		system_timings.push_back(data);
	}
};