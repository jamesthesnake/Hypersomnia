#include "melee_system.h"
#include "entity_system/world.h"
#include "entity_system/entity.h"

#include "game/messages/intent_message.h"
#include "game/messages/melee_swing_response.h"
#include "game/messages/rebuild_physics_message.h"

#include "game/detail/combat/melee_animation.h"

void melee_system::consume_melee_intents() {
	auto& events = parent_world.get_message_queue<messages::intent_message>();

	for (auto it : events) {
		/* 
			we search the whole intent message queue for events of interest;
			here we can even find gunshot requests for some distant enemy AI,
			therefore we need to filter out events we're interested in, and that would be
			melee-related intents and only these applied to an entity with a melee component
		*/
		auto* maybe_melee = it.subject->find<components::melee>();
		
		if (maybe_melee == nullptr) 
			continue;

		auto& melee = *maybe_melee;

		if (it.intent == intent_type::MELEE_PRIMARY_MOVE) {
			melee.primary_move_flag = it.pressed_flag;
		}

		if (it.intent == intent_type::MELEE_SECONDARY_MOVE) {
			melee.secondary_move_flag = it.pressed_flag;
		}
		
		if (it.intent == intent_type::MELEE_TERTIARY_MOVE) {
			melee.tertiary_move_flag = it.pressed_flag;
		}
	}
}

void melee_system::initiate_and_update_moves() {
	/* 
	- fixed delta timestep if it's a logic procedure 
	- variable frame time if it is a rendering-time procedure 
	*/
	auto dt = delta_milliseconds();

	/* clear melee swing response queue because this is the only place where we send them */
	parent_world.get_message_queue<messages::melee_swing_response>().clear();

	/* all entities in the "targets" vector are guaranteed to have both melee and damage components
		therefore we use get instead of find
	*/
	for (auto& t : targets) {
		auto& melee = t->get<components::melee>();
		auto& damage = t->get<components::damage>();

//		 LOG("P: %x, S: %x, T: %x CDT: %x MVT: %x STATE: %x", melee.primary_move_flag, melee.secondary_move_flag, melee.tertiary_move_flag,melee.swing_current_cooldown_time,melee.swing_current_time,melee.state);

		damage.damage_upon_collision = melee.primary_move_flag;

		messages::rebuild_physics_message pos_response;
		pos_response.subject = t;
		auto new_definition = t->get<components::physics_definition>();

		if (melee.primary_move_flag && melee.state == components::MELEE_FREE) {
			/* send a response message so that the rest of the game knows that a swing has occured;
			the message could be read by particles system, audio system and possibly animation system
			to apply each their own effects.
			*/
			melee.state = components::MELEE_PRIMARY;
			messages::melee_swing_response response;
			response.subject = t;
			response.origin_transform = t->get<components::transform>();
			parent_world.post_message(response);
		}
		else if (melee.state == components::MELEE_ONCOOLDOWN) {
//			LOG("ON CD (LEFT: %x )",melee.swing_cooldown_ms - melee.swing_current_cooldown_time);
			melee.swing_current_cooldown_time += dt;

			if (melee.swing_current_cooldown_time >= melee.swing_cooldown_ms) {
				melee.swing_current_cooldown_time = 0;
				melee.state = components::MELEE_FREE;
			}
		}
		else if (melee.state == components::MELEE_PRIMARY) {
			LOG("MELEE PRIMARY (LEFT: %x)", melee.swing_duration_ms - melee.swing_current_time);

			std::vector<vec2> swing = melee.offset_positions;
			std::reverse(std::begin(swing),std::end(swing));

			melee_animation animation(swing);

			if (melee.swing_current_time >= melee.swing_duration_ms) {
				melee.state = components::MELEE_BACKSWING_PRIMARY;
				melee.swing_current_time = 0;
			}
			else
				new_definition.offsets_for_created_shapes[components::physics_definition::SPECIAL_MOVE_DISPLACEMENT] = animation.update(melee.swing_current_time / melee.swing_duration_ms);
			
			pos_response.new_definition = new_definition;
			parent_world.post_message(pos_response);
			melee.swing_current_time += dt * melee.swing_acceleration;
		}
		else if (melee.state == components::MELEE_BACKSWING_PRIMARY) {
			LOG("MELEE BACKSWING PRIMARY (LEFT: %x)", melee.swing_duration_ms - melee.swing_current_time);
			melee_animation animation(melee.offset_positions);
			if (melee.swing_current_time >= melee.swing_duration_ms) {
				melee.state = components::MELEE_ONCOOLDOWN;
				melee.swing_current_time = 0;
			}
			else
				new_definition.offsets_for_created_shapes[components::physics_definition::SPECIAL_MOVE_DISPLACEMENT] = animation.update(melee.swing_current_time / melee.swing_duration_ms);
			pos_response.new_definition = new_definition;
			parent_world.post_message(pos_response);
			melee.swing_current_time += dt * melee.swing_acceleration;
		}
	}
}