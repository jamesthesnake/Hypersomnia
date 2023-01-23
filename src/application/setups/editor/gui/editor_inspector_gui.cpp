#include "augs/templates/enum_introspect.h"
#include "augs/templates/history.hpp"
#include "application/setups/editor/editor_setup.hpp"
#include "augs/misc/imgui/imgui_utils.h"
#include "augs/misc/imgui/imgui_control_wrappers.h"
#include "augs/misc/imgui/imgui_enum_combo.h"
#include "augs/string/string_templates.h"

#include "application/setups/editor/resources/editor_sound_resource.h"
#include "application/setups/editor/resources/editor_sprite_resource.h"
#include "application/setups/editor/resources/editor_light_resource.h"
#include "application/setups/editor/resources/editor_prefab_resource.h"

#include "application/setups/editor/nodes/editor_sound_node.h"
#include "application/setups/editor/nodes/editor_sprite_node.h"
#include "application/setups/editor/nodes/editor_light_node.h"
#include "application/setups/editor/nodes/editor_prefab_node.h"

#include "application/setups/editor/gui/editor_inspector_gui.h"
#include "application/setups/editor/detail/simple_two_tabs.h"

#include "application/setups/editor/detail/maybe_different_colors.h"
#include "application/setups/editor/resources/resource_traits.h"
#include "augs/templates/introspection_utils/introspective_equal.h"
#include "application/setups/editor/detail/make_command_from_selections.h"
#include "test_scenes/test_scene_flavours.h"

#define MULTIPROPERTY(label, MEMBER) \
{\
	bool values_different = false;\
\
	for (auto& ee : es) {\
		if (!(ee.after.MEMBER == insp.MEMBER)) {\
			values_different = true;\
		}\
	}\
\
	auto cols = maybe_different_value_cols({}, values_different);\
\
	last_result = edit_property(result, label, insp.MEMBER);\
\
	if (last_result) {\
		for (auto& ee : es) {\
			ee.after.MEMBER = insp.MEMBER;\
		}\
	}\
}

#define EDIT_FUNCTION template <class T> std::string perform_editable_gui


void editor_tweaked_widget_tracker::reset() {
	last_tweaked.reset();
}

void editor_tweaked_widget_tracker::poll_change(const std::size_t current_command_index) {
	if (last_tweaked) {
		const auto possible_session = tweak_session { ImGui::GetCurrentContext()->LastActiveId, current_command_index };

		if (last_tweaked.value() != possible_session) {
			last_tweaked.reset();
		}
	}
}

bool editor_tweaked_widget_tracker::changed(const std::size_t current_command_index) const {
	const auto current_session = tweak_session { ImGui::GetCurrentContext()->LastActiveId, current_command_index };
	return last_tweaked != current_session;
}

void editor_tweaked_widget_tracker::update(const std::size_t current_command_index) {
	const auto current_session = tweak_session { ImGui::GetCurrentContext()->LastActiveId, current_command_index };
	last_tweaked = current_session;
}

std::string get_hex_representation(const unsigned char*, size_t length);

std::string as_hex(const rgba& col) {
	return to_uppercase(get_hex_representation(std::addressof(col.r), 4));
}

template <class T>
bool edit_property(
	std::string& result,
	const std::string& label,
	T& property
) {
	using namespace augs::imgui;

	if constexpr(std::is_same_v<T, rgba>) {
		if (label.size() > 0 && label[0] == '#') {
			if (color_picker(label, property)) {
				result = typesafe_sprintf("Set Color to %x in %x", as_hex(property));
				return true;
			}
		}
		else {
			if (color_edit(label, property)) {
				result = typesafe_sprintf("Set %x to %x in %x", label, as_hex(property));
				return true;
			}
		}
	}
	else if constexpr(std::is_same_v<T, bool>) {
		if (checkbox(label, property)) {
			if (property) {
				result = typesafe_sprintf("Enable %x in %x", label);
				return true;
			}
			else {
				result = typesafe_sprintf("Disable %x in %x", label);
				return true;
			}
		}
	}
	else if constexpr(std::is_arithmetic_v<T>) {
		if (drag(label, property)) { 
			result = typesafe_sprintf("Set %x to %x in %x", label, property);
			return true;
		}
	}
	else if constexpr(std::is_enum_v<T>) {
		if constexpr(std::is_same_v<T, faction_type>) {
			if (enum_combo_constrained<
				faction_type, 
				faction_type::RESISTANCE,
				faction_type::METROPOLIS,
				faction_type::ATLANTIS
			>(label, property)) { 
				result = typesafe_sprintf("Switch %x to %x in %x", label, property);
				return true;
			}
		}
		else {
			if (enum_combo(label, property)) { 
				result = typesafe_sprintf("Switch %x to %x in %x", label, property);
				return true;
			}
		}
	}
	else if constexpr(is_one_of_v<T, vec2, vec2i, vec2u>) {
		if (drag_vec2(label, property)) { 
			result = typesafe_sprintf("Set %x to %x in %x", label, property);
			return true;
		}
	}

	return false;
}

EDIT_FUNCTION(editor_sprite_node_editable& insp, T& es) {
	using namespace augs::imgui;

	bool last_result = false;
	std::string result;

	MULTIPROPERTY("Position", pos);
	MULTIPROPERTY("Rotation", rotation);

	MULTIPROPERTY("Flip horizontally", flip_horizontally);
	ImGui::SameLine();
	MULTIPROPERTY("Flip vertically", flip_vertically);

	MULTIPROPERTY("Colorize", colorize);

	MULTIPROPERTY("Resize", size.is_enabled);

	if (insp.size.is_enabled) {
		ImGui::SameLine();
		MULTIPROPERTY("##Overridden size", size.value);

		if (last_result) {
			result = "Resized %x";
		}
	}

	return result;
}

EDIT_FUNCTION(editor_sound_node_editable& insp, T& es) {
	using namespace augs::imgui;
	bool last_result = false;
	std::string result;

	MULTIPROPERTY("Position", pos);

	return result;
}

EDIT_FUNCTION(editor_firearm_node_editable& insp, T& es) {
	using namespace augs::imgui;
	bool last_result = false;
	std::string result;

	MULTIPROPERTY("Position", pos);
	MULTIPROPERTY("Rotation", rotation);

	return result;
}

EDIT_FUNCTION(editor_ammunition_node_editable& insp, T& es) {
	using namespace augs::imgui;
	bool last_result = false;
	std::string result;

	MULTIPROPERTY("Position", pos);
	MULTIPROPERTY("Rotation", rotation);

	return result;
}

EDIT_FUNCTION(editor_melee_node_editable& insp, T& es) {
	using namespace augs::imgui;
	bool last_result = false;
	std::string result;

	MULTIPROPERTY("Position", pos);
	MULTIPROPERTY("Rotation", rotation);

	return result;
}

EDIT_FUNCTION(editor_wandering_pixels_node_editable& insp, T& es) {
	using namespace augs::imgui;
	bool last_result = false;
	std::string result;

	MULTIPROPERTY("Position", pos);
	MULTIPROPERTY("Rotation", rotation);

	ImGui::Separator();

	MULTIPROPERTY("Colorize", colorize);
	MULTIPROPERTY("Num particles", num_particles);
	MULTIPROPERTY("Force particles within bounds", force_particles_within_bounds);
	MULTIPROPERTY("Illuminate", illuminate);

	return result;
}

static bool has_letter(const point_marker_type type) {
	switch (type) {
		default:
			return false;
	}
}

static bool has_team(const point_marker_type type) {
	switch (type) {
		case point_marker_type::TEAM_SPAWN:
			return true;
		default:
			return false;
	}
}

static bool has_letter(const area_marker_type type) {
	switch (type) {
		case area_marker_type::BOMBSITE:
			return true;
		default:
			return false;
	}
}

static bool has_team(const area_marker_type type) {
	switch (type) {
		case area_marker_type::BUY_AREA:
			return true;
		default:
			return false;
	}
}

EDIT_FUNCTION(editor_point_marker_node_editable& insp, T& es, const editor_point_marker_resource& resource) {
	using namespace augs::imgui;
	bool last_result = false;
	std::string result;

	const auto type = resource.editable.type;

	if (has_team(type)) {
		MULTIPROPERTY("Faction", faction);
	}

	if (has_letter(type)) {
		MULTIPROPERTY("Letter", letter);
	}

	return result;
}

EDIT_FUNCTION(editor_area_marker_node_editable& insp, T& es, const editor_area_marker_resource& resource) {
	using namespace augs::imgui;
	bool last_result = false;
	std::string result;

	const auto type = resource.editable.type;

	if (has_team(type)) {
		MULTIPROPERTY("Faction", faction);
	}

	if (has_letter(type)) {
		MULTIPROPERTY("Letter", letter);
	}

	return result;
}

EDIT_FUNCTION(editor_explosive_node_editable& insp, T& es) {
	using namespace augs::imgui;
	bool last_result = false;
	std::string result;

	MULTIPROPERTY("Position", pos);
	MULTIPROPERTY("Rotation", rotation);

	return result;
}

EDIT_FUNCTION(editor_light_node_editable& insp, T& es) {
	using namespace augs::imgui;
	bool last_result = false;
	std::string result;

	MULTIPROPERTY("Position", pos);
	MULTIPROPERTY("Colorize", colorize);
	MULTIPROPERTY("Scale intensity", scale_intensity);

	return result;
}

EDIT_FUNCTION(editor_particles_node_editable& insp, T& es) {
	using namespace augs::imgui;
	bool last_result = false;
	std::string result;

	MULTIPROPERTY("Position", pos);

	return result;
}

EDIT_FUNCTION(editor_layer_editable& insp, T& es) {
	using namespace augs::imgui;
	bool last_result = false;
	std::string result;

	MULTIPROPERTY("Selectable on scene", selectable_on_scene);

	if (ImGui::IsItemHovered()) {
		text_tooltip("Useful for e.g. ground/floor layers.\nYou might want to see the background objects without them being hoverable.\nThis way you can comfortably mass-select only some foreground objects.");
	}

	return result;
}

std::string perform_editable_gui(
	editor_sprite_resource_editable& e,
	const std::optional<vec2i> original_size,
	const image_color_picker_widget& picker
) {
	using namespace augs::imgui;

	std::string result;

	edit_property(result, "Domain", e.domain);

	//ImGui::Separator();

	const auto current_size = e.size;

	edit_property(result, "Color", e.color);
	//ImGui::Separator();
	edit_property(result, "Size", e.size);

	if (original_size != std::nullopt) {
		if (original_size != current_size) {
			ImGui::SameLine();

			if (ImGui::Button("Reset")) {
				e.size = *original_size;
				return "Reset %x size to original";
			}
		}
	}

	edit_property(result, "Stretch when resized", e.stretch_when_resized);

	if (e.domain == editor_sprite_domain::FOREGROUND) {
		edit_property(result, "Foreground glow", e.foreground_glow);
	}

	{
		if (edit_property(result, "##NeonMap", e.neon_map.is_enabled)) {
			if (e.neon_map.is_enabled) {
				result = "Enabled neon map in %x";
			}
			else {
				result = "Disabled neon map in %x";
			}
		}

		ImGui::SameLine();

		auto disabled = maybe_disabled_only_cols(!e.neon_map.is_enabled);

		auto scope = augs::imgui::scoped_tree_node_ex("Neon map");

		if (scope) {

			auto actually_disabled = maybe_disabled_cols(!e.neon_map.is_enabled);

			auto& v = e.neon_map.value;

			edit_property(result, "Colorize neon", e.neon_color);

			auto ic = 0;
			auto removed_i = -1;

			text("Click on the image to add neon light sources.");

			{
				if (picker.handle_prologue("##LightColorPicker", v.light_colors)) {
					result = "Picked new light color in %x";
				}
			}

			for (auto& col : v.light_colors) {
				auto id_col = typesafe_sprintf("Light %x", ++ic);
				auto id_but = typesafe_sprintf("-##Light%x", ic);

				if (ImGui::Button(id_but.c_str())) {
					removed_i = ic;
				}

				ImGui::SameLine();

				edit_property(result, id_col, col);
			}

			if (removed_i != -1) {
				v.light_colors.erase(v.light_colors.begin() + removed_i - 1);
				result = typesafe_sprintf("Removed Light %x in %x", removed_i);
			}

#if 0
			if (ImGui::Button("+##AddNewLightColor")) {
				v.light_colors.emplace_back(white);
				result = "Added new light color in %x";
			}

			text("Add new light source");
#endif

			if (auto scope = augs::imgui::scoped_tree_node_ex("Advanced neon map parameters")) {
				edit_property(result, "Standard deviation", v.standard_deviation);
				edit_property(result, "Radius", v.radius.x);
				v.radius.y = v.radius.x;
				edit_property(result, "Amplification", v.amplification);
			}
		}
	}

	if (e.domain == editor_sprite_domain::PHYSICAL) {
		ImGui::Separator();
		text_color("Physics", yellow);
		ImGui::Separator();

		{
			auto scope = augs::imgui::scoped_tree_node_ex("Edit collider shape");
		}

		edit_property(result, "Is see-through", e.is_see_through);

		if (ImGui::IsItemHovered()) {
			text_tooltip("If enabled, lets the light through.\nEnemies will be visible behind this object.\nUse it on walls of glass.");
		}

		edit_property(result, "Is body static", e.is_static);

		if (ImGui::IsItemHovered()) {
			text_tooltip("If enabled, will be permanently set in place.\nWon't move no matter what.\nUse it on layout-defining walls and objects.");
		}

		edit_property(result, "Density", e.density);
		edit_property(result, "Friction", e.friction);
		edit_property(result, "Restitution", e.restitution);
	}

	ImGui::Separator();

	return result;
}

std::string perform_editable_gui(editor_sound_resource_editable&) {
	using namespace augs::imgui;
	std::string result;

	return result;
}

std::string perform_editable_gui(editor_light_resource_editable& e) {
	using namespace augs::imgui;
	std::string result;

	edit_property(result, "##Defaultcolor", e.color);

	return result;
}

std::string perform_editable_gui(editor_particles_resource_editable&) {
	using namespace augs::imgui;
	std::string result;

	return result;
}

std::string perform_editable_gui(editor_material_resource_editable&) {
	using namespace augs::imgui;
	std::string result;

	return result;
}

std::string perform_editable_gui(editor_firearm_resource_editable&) {
	using namespace augs::imgui;
	std::string result;

	return result;
}

std::string perform_editable_gui(editor_ammunition_resource_editable&) {
	using namespace augs::imgui;
	std::string result;

	return result;
}

std::string perform_editable_gui(editor_melee_resource_editable&) {
	using namespace augs::imgui;
	std::string result;

	return result;
}

std::string perform_editable_gui(editor_wandering_pixels_resource_editable& e) {
	using namespace augs::imgui;
	std::string result;

	if (auto scope = augs::imgui::scoped_tree_node_ex("Node defaults")) {
		(void)e;
		// TODO: let this be edited
		/* if (const auto new_res = perform_editable_gui(e.node_defaults); !new_res.empty()) { */
		/* 	result = new_res; */
		/* } */
	}

	return result;
}

std::string perform_editable_gui(editor_point_marker_resource_editable& e) {
	using namespace augs::imgui;
	std::string result;

	(void)e;

	return result;
}

std::string perform_editable_gui(editor_area_marker_resource_editable& e) {
	using namespace augs::imgui;
	std::string result;

	(void)e;

	return result;
}

std::string perform_editable_gui(editor_explosive_resource_editable&) {
	using namespace augs::imgui;
	std::string result;

	return result;
}

void editor_inspector_gui::inspect(const inspected_variant inspected, bool wants_multiple) {
	auto different_found = [&]<typename T>(const T&) {
		return inspects_any_different_than<T>();
	};

	const bool different_type_found = std::visit(different_found, inspected);

	if (wants_multiple) {
		if (different_type_found) {
			return;
		}

		if (found_in(all_inspected, inspected)) {
			erase_element(all_inspected, inspected);
		}
		else {
			all_inspected.push_back(inspected);
			mark_last_inspected(inspected);
		}
	}
	else {
		const bool not_the_same = all_inspected != decltype(all_inspected) { inspected };

		if (not_the_same) {
			all_inspected = { inspected };
			mark_last_inspected(inspected);

			/*
				Commands will invoke "inspect" on undo/redo,
				even when executing them for the first time - directly by user interaction.

				This is why we need to prevent the tweaked widget from being reset if the inspected object does not change,
				because otherwise continuous commands wouldn't work.
			*/
			tweaked_widget.reset();
		}
	}
}

void editor_inspector_gui::perform(const editor_inspector_input in) {
	using namespace augs::imgui;

	(void)in;

	auto window = make_scoped_window();

	if (!window) {
		return;
	}

	auto get_object_category_name = [&]<typename T>(const T&) {
		if constexpr(std::is_same_v<T, editor_node_id>) {
			return "nodes";
		}
		else if constexpr(std::is_same_v<T, editor_resource_id>) {
			return "resources";
		}
		else if constexpr(std::is_same_v<T, editor_layer_id>) {
			return "layers";
		}
		else {
			static_assert(always_false_v<T>, "Non-exhaustive handler");
		}

		return "objects";
	};

	auto get_command_index = [&]() {
		return in.setup.get_last_command_index();
	};

	auto post_new_or_rewrite = [&]<typename T>(T&& cmd) {
		bool force_new = false;

		if (begins_with(cmd.describe(), "Added") || begins_with(cmd.describe(), "Picked") || begins_with(cmd.describe(), "Removed")) {
			force_new = true;
		}

		if (force_new || tweaked_widget.changed(get_command_index())) {
			in.setup.post_new_command(std::forward<T>(cmd));
			tweaked_widget.update(get_command_index());
		}
		else {
			in.setup.rewrite_last_command(std::forward<T>(cmd));
		}
	};

	auto resource_handler = [&]<typename R>(
		const R& resource, 
		const auto& resource_id, 
		std::optional<std::vector<inspected_variant>> override_inspector_state = std::nullopt
	) {
		auto name = resource.get_display_name();

		bool request_defaults = false;

		if (!resource_id.is_official) {
			const auto region_avail = ImGui::GetContentRegionAvail().x;

			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, region_avail - ImGui::CalcTextSize("Reset").x - ImGui::GetStyle().FramePadding.x);
		}

		if constexpr(is_pathed_resource_v<R>) {
			/* Preserve extension when displaying name */
			name = resource.external_file.path_in_project.filename().string();
			
			auto reveal_in_explorer_button = [this](const auto& path) {
				auto cursor = scoped_preserve_cursor();

				if (ImGui::Selectable("##RevealButton")) {
					reveal_in_explorer_once = path;
				}

				if (ImGui::IsItemHovered()) {
					text_tooltip("Reveal in explorer");
				}
			};

			const auto& path_in_project = resource.external_file.path_in_project;

			const auto full_path = 
				resource_id.is_official 
				? OFFICIAL_CONTENT_PATH / path_in_project
				: in.setup.resolve_project_path(path_in_project)
			;

			reveal_in_explorer_button(full_path);
		}

		text_color(std::string(resource.get_type_name()) + ": ", yellow);

		ImGui::SameLine();
		text(name);

		auto hypothetical_default = decltype(resource.editable)();

		if (!resource_id.is_official) {
			ImGui::NextColumn();

			if constexpr(std::is_same_v<R, editor_sprite_resource>) {
				if (auto ad_hoc = mapped_or_nullptr(in.ad_hoc_atlas, resource.thumbnail_id)) {
					hypothetical_default.size = ad_hoc->get_original_size();
				}
			}

			{
				const auto reset_bgs = std::array<rgba, 3> {
					rgba(0, 0, 0, 0),
					rgba(80, 20, 20, 255),
					rgba(150, 40, 40, 255)
				};

				const bool already_default = augs::introspective_equal(hypothetical_default, resource.editable);

				{
					auto cols = scoped_button_colors(reset_bgs);
					auto disabled = maybe_disabled_cols(already_default);

					if (ImGui::Button("Reset") && !already_default) {
						request_defaults = true;
					}
				}

				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
					if (already_default) {
						auto scope = scoped_tooltip();
						text_color("There's nothing to reset now.", green); text("The resource has default properties.");
					}
					else {
						auto scope = scoped_tooltip();
						text_color("Restore defaults to all properties below.", orange); text("Can safely be undone.");
					}
				}
			}

			ImGui::Columns(1);
		}

		//game_image(icon, scaled_icon_size, white, vec2::zero, atlas_type);

		ImGui::Separator();

		auto id = scoped_id(name.c_str());

		if (resource_id.is_official) {
			text_color("Official resources cannot be edited.", yellow);
		}
		else {
			auto edited_copy = resource.editable;
			auto changed = std::string();

			if constexpr(std::is_same_v<R, editor_sprite_resource>) {
				/* 
					It's probably better to leave the widgets themselves active
					in case someone wants to copy the values.
				*/

				auto disabled = maybe_disabled_only_cols(resource_id.is_official);

				const auto project_dir = in.setup.get_unofficial_content_dir();

				const auto picker = image_color_picker_widget {
					resource.scene_asset_id,
					in.game_atlas,
					in.setup.get_viewable_defs().image_definitions,
					neon_map_picker_preview,
					project_dir,
					false
				};

				changed = perform_editable_gui(edited_copy, hypothetical_default.size, picker);

			}
			else {
				auto disabled = maybe_disabled_only_cols(resource_id.is_official);
				changed = perform_editable_gui(edited_copy);
			}

			if (request_defaults) {
				edited_copy = hypothetical_default;
				changed = "Restored defaults to %x";
			}

			if (!changed.empty() && !resource_id.is_official) {
				edit_resource_command<R> cmd;
				cmd.resource_id = resource_id;
				cmd.after = edited_copy;
				cmd.built_description = typesafe_sprintf(changed, resource.get_display_name());
				cmd.override_inspector_state = std::move(override_inspector_state);

				post_new_or_rewrite(std::move(cmd)); 
			}
		}
	};

	auto node_handler = [&]<typename N>(const N& node, const auto& node_id) {
		auto id = scoped_id(node.unique_name.c_str());

		text_color(std::string(node.get_type_name()) + ": ", yellow);

		{
			ImGui::SameLine();

			auto edited_node_name = node.unique_name;

			if (input_text<100>("##NameInput", edited_node_name, ImGuiInputTextFlags_EnterReturnsTrue)) {
				if (edited_node_name != node.unique_name && !edited_node_name.empty()) {
					auto cmd = in.setup.make_command_from_selected_nodes<rename_node_command>("Renamed ");
					cmd.after = edited_node_name;

					if (cmd.size() == 1) {
						cmd.built_description = typesafe_sprintf("Renamed node to %x", cmd.after);
					}

					if (!cmd.empty()) {
						in.setup.post_new_command(std::move(cmd)); 
					}
				}
			}
		}
		ImGui::Separator();
		auto edited_copy = node.editable;
		auto changed = std::string();

		auto cmd = in.setup.make_command_from_selected_typed_nodes<edit_node_command<N>, N>("Edited ");

		for (auto& e : cmd.entries) {
			const auto node = in.setup.find_node(e.node_id);
			ensure(node != nullptr);
			e.after = node->editable;
		}

		(void)node_id;

		if constexpr(std::is_same_v<N, editor_sprite_node>) {
			const auto resource = in.setup.find_resource(node.resource_id);
			ensure(resource != nullptr);

			auto original_size = resource->editable.size;
			
			if (resource->official_tag) {
				std::visit(
					[&](const auto& tag) {
						const auto& flavour = in.setup.get_initial_scene().world.get_flavour(to_entity_flavour_id(tag));
						if (const auto sprite = flavour.template find<invariants::sprite>()) {
							original_size = sprite->get_size();
						}
					},
					*resource->official_tag
				);
			}

			changed = perform_editable_gui(edited_copy, cmd.entries);
		}
		else {
			if constexpr(is_one_of_v<N, editor_point_marker_node, editor_area_marker_node>) {
				const auto resource = in.setup.find_resource(node.resource_id);
				ensure(resource != nullptr);

				changed = perform_editable_gui(edited_copy, cmd.entries, *resource);
			}
			else {
				changed = perform_editable_gui(edited_copy, cmd.entries);
			}
		}

		if (!changed.empty() && !cmd.empty()) {
			if (cmd.size() == 1) {
				cmd.built_description = typesafe_sprintf(changed, node.get_display_name());
			}

			post_new_or_rewrite(std::move(cmd)); 
		}

		(void)node;
	};

	auto layer_handler = [&](editor_layer& layer, const editor_layer_id layer_id) {
		text_color("Layer: ", yellow);

		{
			ImGui::SameLine();

			{
				auto edited_layer_name = layer.unique_name;

				if (input_text<100>("##NameInput", edited_layer_name, ImGuiInputTextFlags_EnterReturnsTrue)) {
					if (edited_layer_name != layer.unique_name && !edited_layer_name.empty()) {
						auto cmd = in.setup.make_command_from_selected_layers<rename_layer_command>("Renamed ");
						cmd.after = edited_layer_name;

						if (cmd.size() == 1) {
							cmd.built_description = typesafe_sprintf("Renamed layer to %x", cmd.after);
						}

						if (!cmd.empty()) {
							in.setup.post_new_command(std::move(cmd)); 
						}

						in.setup.post_new_command(std::move(cmd)); 
					}
				}
			}

			{
				auto edited_copy = layer.editable;
				auto changed = std::string();

				auto cmd = in.setup.make_command_from_selected_layers<edit_layer_command>("Edited ");

				for (auto& e : cmd.entries) {
					const auto layer = in.setup.find_layer(e.layer_id);
					ensure(layer != nullptr);
					e.after = layer->editable;
				}

				(void)layer_id;
				changed = perform_editable_gui(edited_copy, cmd.entries);

				if (!changed.empty() && !cmd.empty()) {
					if (cmd.size() == 1) {
						cmd.built_description = typesafe_sprintf(changed, layer.get_display_name());
					}

					post_new_or_rewrite(std::move(cmd)); 
				}
			}
		}
	};

	auto edit_properties = [&]<typename T>(const T& inspected_id) {
		if constexpr(std::is_same_v<T, editor_node_id>) {
			simple_two_tabs(
				node_current_tab,
				inspected_node_tab_type::NODE,
				inspected_node_tab_type::RESOURCE,
				"Node",
				"Resource"
			);

			ImGui::Separator();

			if (node_current_tab == inspected_node_tab_type::NODE) {
				in.setup.on_node(inspected_id, node_handler);
			}
			else {
				in.setup.on_node(inspected_id, [&](const auto& typed_node, const auto id) {
					(void)id;

					if (const auto resource = in.setup.find_resource(typed_node.resource_id)) {
						resource_handler(*resource, typed_node.resource_id, in.setup.get_all_inspected());
					}
				});
			}
		}
		else if constexpr(std::is_same_v<T, editor_resource_id>) {
			in.setup.on_resource(inspected_id, resource_handler);
		}
		else if constexpr(std::is_same_v<T, editor_layer_id>) {
			if (auto layer = in.setup.find_layer(inspected_id)) {
				layer_handler(*layer, inspected_id);
			}

		}
		else {
			static_assert(always_false_v<T>, "Non-exhaustive handler");
		}

		(void)inspected_id;
	};

	if (all_inspected.size() == 0) {

	}
	else if (all_inspected.size() == 1) {
		std::visit(edit_properties, all_inspected[0]);
	}
	else {
		auto can_multi_edit = [&]<typename T>(const T& typed_inspected) -> bool { 
			if constexpr(std::is_same_v<T, editor_layer_id>) {
				return inspects_only<T>();
			}
			else if constexpr(std::is_same_v<T, editor_node_id>) {
				const auto same_type = in.setup.on_node(
					typed_inspected,
					[&]<typename E>(const E&, editor_typed_node_id<E> typed) {
						for (auto& e : all_inspected) {
							if (std::get<editor_node_id>(e).type_id != typed.get_type_id()) {
								return false;
							}
						}

						return true;
					}
				);

				return same_type.has_value() && same_type.value();
			}
			else if constexpr(std::is_same_v<T, editor_resource_id>) {
				const auto same_type = in.setup.on_resource(
					typed_inspected,
					[&]<typename E>(const E&, editor_typed_resource_id<E> typed) {
						for (auto& e : all_inspected) {
							if (std::get<editor_resource_id>(e).type_id != typed.get_type_id()) {
								return false;
							}
						}

						return true;
					}
				);

				return same_type.has_value() && same_type.value();
			}
			else {
				static_assert(always_false_v<T>, "Non-exhaustive");
			}
		};

		if (std::visit(can_multi_edit, all_inspected[0])) {
			std::visit(edit_properties, last_inspected_any);
		}
		else {
			text_color(typesafe_sprintf("Selected %x %x:", all_inspected.size(), std::visit(get_object_category_name, all_inspected[0])), yellow);

			ImGui::Separator();

			for (const auto& a : all_inspected) {
				const auto name = in.setup.get_name(a);

				if (name.size() > 0) {
					text(name);
				}
			}
		}
	}
}
