#pragma once
#include <array>
#include "augs/gui/rect.h"
#include "augs/gui/rect_world.h"
#include "game/transcendental/entity_id.h"
#include "game/detail/gui/drag_and_drop_target_drop_item.h"
#include "game/detail/gui/hotbar_button.h"
#include "augs/gui/dereferenced_location.h"
#include "game/enums/slot_function.h"
#include "augs/ensure.h"

#include "augs/misc/constant_size_vector.h"
#include "game/container_sizes.h"

#include "game/detail/gui/game_gui_context.h"
//#include <map>
//
//#include "game/detail/inventory_slot_id.h"
//#include "augs/gui/appearance_detector.h"
//
//
//#include "game/detail/augs/gui/immediate_hud.h"
//

class viewing_step;
#include "game/transcendental/step_declaration.h"

namespace components {
	struct gui_element {
		game_gui_rect_world rect_world;
		int dragged_charges = 0;

		bool is_gui_look_enabled = false;
		bool preview_due_to_item_picking_request = false;
		bool draw_free_space_inside_container_icons = true;
		padding_byte pad;

		drag_and_drop_target_drop_item drop_item_icon = augs::gui::material(assets::texture_id::DROP_HAND_ICON, red);

		std::array<hotbar_button, 10> hotbar_buttons;

		vec2i get_screen_size() const;
		vec2 get_gui_crosshair_position() const;

		static rects::xywh<float> get_rectangle_for_slot_function(const slot_function);
		
		vec2i get_initial_position_for(const drag_and_drop_target_drop_item&) const;
		vec2 initial_inventory_root_position() const;
		
		void draw_cursor_and_tooltip(vertex_triangle_buffer& output_buffer, const viewing_gui_context&) const;
		
		static entity_id get_hovered_world_entity(const cosmos& cosm, const vec2& world_cursor_position);
		static void draw_complete_gui_for_camera_rendering_request(vertex_triangle_buffer& output_buffer, const const_entity_handle& handle, viewing_step&);
	};
}