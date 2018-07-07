#include "DebugWindow.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include "../System.h"
#include "../Time.h"

namespace Debugging {
	struct System_Timing_Data {
		const char* system_name;
		float time;
		steady_clock::time_point start_time;
		steady_clock::time_point end_time;
		ISystem* system;		
	};

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

			for (const System_Timing_Data& data : system_timings) {
				ImGui::Text("%-*s (%d): %.2f ms [%d]", longest_system_name, data.system_name, data.system->get_system_id(), data.time * 1000.f, data.system->get_component_count());
			}

			ImGui::Text("Want to use mouse: %d", ImGui::GetIO().WantCaptureMouse);
			ImGui::Text("Want to use keyboard: %d", ImGui::GetIO().WantCaptureKeyboard);
		}

		ImGui::End();
		
		ImGui::Render();
	}
	
	void system_timing_start(ISystem * system) {
		uint8_t system_id = system->get_system_id();

		system_timings[system_id].start_time = high_resolution_clock::now();
	}

	void system_timing_stop(ISystem * system) {
		uint8_t system_id = system->get_system_id();
		system_timings[system_id].end_time = high_resolution_clock::now();
		system_timings[system_id].time = std::chrono::duration_cast<std::chrono::microseconds>(system_timings[system_id].end_time - system_timings[system_id].start_time).count() * 0.000001f;
	}

	void debug_register_system(ISystem * system, const char * system_name) {
		System_Timing_Data data = {};
		data.system = system;
		data.system_name = system_name;
		system_timings.push_back(data);
		int name_length = strlen(system_name);
		if (name_length > longest_system_name)
			longest_system_name = name_length;
	}
};