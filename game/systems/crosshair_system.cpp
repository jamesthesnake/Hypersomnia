#include "crosshair_system.h"
#include "game/cosmos.h"

#include "game/messages/intent_message.h"
#include "game/messages/crosshair_intent_message.h"

#include "game/components/sprite_component.h"
#include "game/components/camera_component.h"

#include "game/components/crosshair_component.h"
#include "game/components/transform_component.h"
#include "game/messages/intent_message.h"

#include "texture_baker/texture_baker.h"
#include "game/entity_handle.h"
#include "game/step.h"

void crosshair_system::generate_crosshair_intents(fixed_step& step) {
	step.messages.get_queue<messages::crosshair_intent_message>().clear();
	auto events = step.messages.get_queue<messages::intent_message>();

	for (auto& it : events) {
		messages::crosshair_intent_message crosshair_intent;
		crosshair_intent.messages::intent_message::operator=(it);

		if (it.intent == intent_type::MOVE_CROSSHAIR ||
			it.intent == intent_type::CROSSHAIR_PRIMARY_ACTION ||
			it.intent == intent_type::CROSSHAIR_SECONDARY_ACTION
			) {
			auto subject = cosmos[it.subject];
			auto crosshair = subject.find<components::crosshair>();

			if (!crosshair)
				continue;

			vec2 delta = vec2(vec2(it.state.mouse.rel) * crosshair->sensitivity).rotate(crosshair->rotation_offset, vec2());

			vec2& base_offset = crosshair->base_offset;
			vec2 old_base_offset = base_offset;
			vec2 old_pos = position(subject);

			base_offset += delta;
			base_offset.clamp_rotated(crosshair->bounds_for_base_offset, crosshair->rotation_offset);

			crosshair_intent.crosshair_base_offset_rel = base_offset - old_base_offset;
			crosshair_intent.crosshair_base_offset = base_offset;
			crosshair_intent.crosshair_world_pos = base_offset + cosmos[crosshair->character_entity_to_chase].get<components::transform>().pos;

			step.messages.post(crosshair_intent);
		}
	}
}
void crosshair_system::apply_crosshair_intents_to_base_offsets(fixed_step& step) {
	auto& events = step.messages.get_queue<messages::crosshair_intent_message>();

	for (auto& it : events)
		cosmos[it.subject].get<components::crosshair>().base_offset = it.crosshair_base_offset;
}

void crosshair_system::apply_base_offsets_to_crosshair_transforms(fixed_step& step) {
	for (auto& it : cosmos.get(processing_subjects::WITH_CROSSHAIR)) {
		auto player_id = cosmos[it.get<components::crosshair>().character_entity_to_chase];

		if (player_id.alive()) {
			it.get<components::transform>().pos = 
				components::crosshair::calculate_aiming_displacement(it, true) + position(player_id);
		}
	}
}