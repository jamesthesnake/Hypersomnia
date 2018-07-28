#include "application/setups/editor/gui/editor_player_gui.h"
#include "application/setups/editor/editor_player.h"
#include "application/setups/editor/editor_command_input.h"

void editor_player_gui::perform(const editor_command_input cmd_in) {
	using namespace augs::imgui;

	auto& player = cmd_in.get_player();

	auto window = make_scoped_window();

	if (!window) {
		return;
	}

	acquire_keyboard_once();

	if (ImGui::Button("Play")) {
		player.paused = false;
	}

	ImGui::SameLine();
	
	if (ImGui::Button("Pause")) {
		player.paused = true;
	}

	ImGui::SameLine();
	
	if (ImGui::Button("Stop")) {
		player.paused = true;
	}
}