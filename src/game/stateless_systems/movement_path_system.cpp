#include "augs/misc/randomization.h"
#include "augs/drawing/make_sprite_points.h"
#include "augs/math/steering.h"

#include "game/transcendental/cosmos.h"
#include "game/transcendental/entity_handle.h"
#include "game/transcendental/logic_step.h"

#include "game/messages/interpolation_correction_request.h"
#include "game/messages/queue_destruction.h"
#include "game/messages/will_soon_be_deleted.h"
#include "game/detail/visible_entities.h"

#include "game/stateless_systems/movement_path_system.h"

void movement_path_system::advance_paths(const logic_step step) const {
	auto& cosm = step.get_cosmos();
	const auto delta = step.get_delta();

	auto& npo = cosm.get_solvable_inferred({}).tree_of_npo;

	cosm.for_each_having<components::movement_path>(
		[&](const auto subject) {
			const auto& movement_path_def = subject.template get<invariants::movement_path>();

			auto& transform = subject.template get<components::transform>();

			const auto& rotation_speed = movement_path_def.continuous_rotation_speed;

			if (!augs::is_epsilon(rotation_speed)) {
				transform.rotation += rotation_speed * delta.in_seconds();
			}

			if (movement_path_def.fish_movement.is_enabled) {
				auto& movement_path = subject.template get<components::movement_path>();

				const auto& pos = transform.pos;
				const auto tip_pos = *subject.find_logical_tip();

				const auto& def = movement_path_def.fish_movement.value;
				const auto size = def.rect_size;

				const auto origin = movement_path.origin;
				const auto bound = xywh::center_and_size(origin.pos, size);

				const auto global_time = cosm.get_total_seconds_passed() + real32(subject.get_guid());
				const auto global_time_sine = std::sin(global_time * 2);

				const auto max_speed_boost = def.sine_speed_boost;
				const auto boost_mult = static_cast<real32>(global_time_sine * global_time_sine);
				const auto speed_boost = boost_mult * max_speed_boost;

				const auto fov_half_degrees = real32((360 - 90) / 2);
				const auto max_avoidance_speed = 20 + speed_boost / 2;
				const auto max_startle_speed = 250 + 4*speed_boost;
				const auto max_lighter_startle_speed = 200 + 4*speed_boost;

				const auto cohesion_mult = 0.05f;
				const auto alignment_mult = 0.08f;

				const auto base_speed = def.base_speed;

				const auto min_speed = base_speed + speed_boost;
				const auto max_speed = base_speed + max_speed_boost;

				const auto current_dir = vec2::from_degrees(transform.rotation);

				const float comfort_zone_radius = 50.f;
				const float cohesion_zone_radius = 60.f;

				auto for_each_neighbor_within = [&](const auto radius, auto callback) {
					thread_local visible_entities neighbors;

					neighbors.clear();
					neighbors.acquire_non_physical({
						cosm,
						camera_cone(camera_eye(tip_pos), vec2::square(radius * 2)),
						false
					});

					for (const auto& a : neighbors.all) {
						cosm[a].dispatch_on_having<components::movement_path>([&](const auto typed_neighbor) {
							if (typed_neighbor.get_id() != subject.get_id()) {
								const auto neighbor_tip = *typed_neighbor.find_logical_tip();
								const auto offset = neighbor_tip - tip_pos;

								const auto facing = current_dir.degrees_between(offset);

								if (facing < fov_half_degrees) {
									callback(typed_neighbor);
								}
							}
							/* Otherwise, don't measure against itself */
						});
					}
				};

				auto velocity = current_dir * min_speed;

				real32 total_startle_applied = 0.f;

				auto do_startle = [&](const auto type, const auto damping, const auto steer_mult, const auto max_speed) {
					auto& startle = movement_path.startle[type];
					//const auto desired_vel = vec2(startle).trim_length(max_speed);
					const auto desired_vel = startle;
					const auto total_steering = vec2((desired_vel - velocity) * steer_mult * (0.02f + (0.16f * boost_mult))).trim_length(max_speed);

					total_startle_applied += total_steering.length() / velocity.length();

					velocity += total_steering;

					startle.damp(delta.in_seconds(), vec2::square(damping));
				};

				do_startle(startle_type::LIGHTER, 0.2f, 0.1f, max_lighter_startle_speed);
				do_startle(startle_type::IMMEDIATE, 5.f, 1.f, max_startle_speed);

				vec2 average_pos;
				vec2 average_vel;

				unsigned counted_neighbors = 0;

				{
					auto greatest_avoidance = vec2::zero;

					const auto subject_layer = subject.template get<invariants::render>().layer;

					for_each_neighbor_within(comfort_zone_radius, [&](const auto typed_neighbor) {
						const auto neighbor_layer = typed_neighbor.template get<invariants::render>().layer;

						if (int(subject_layer) > int(neighbor_layer)) {
							/* Don't avoid smaller species. */
							return;
						}

						const auto& neighbor_path_def = typed_neighbor.template get<invariants::movement_path>();
						const auto& neighbor_path = typed_neighbor.template get<components::movement_path>();

						if (neighbor_path_def.fish_movement.is_enabled) {
							const auto neighbor_transform = typed_neighbor.get_logic_transform();
							const auto neighbor_vel = vec2::from_degrees(neighbor_transform.rotation) * neighbor_path.last_speed;
							const auto neighbor_tip = *typed_neighbor.find_logical_tip();

							const auto avoidance = augs::immediate_avoidance(
								tip_pos,
								vec2(velocity).set_length(movement_path.last_speed),
								neighbor_tip,
								neighbor_vel,
								comfort_zone_radius,
								max_avoidance_speed * neighbor_path.last_speed / max_speed
							);

							greatest_avoidance = std::max(avoidance, greatest_avoidance);

							if (typed_neighbor.get_flavour_id() == subject.get_flavour_id()) {
								average_pos += neighbor_transform.pos;
								average_vel += vec2::from_degrees(neighbor_transform.rotation) * neighbor_path.last_speed;
								++counted_neighbors;
							}
						}
					});

					velocity += greatest_avoidance;
				}

				if (counted_neighbors) {
					average_pos /= counted_neighbors;
					average_vel /= counted_neighbors;

					if (cohesion_mult != 0.f) {
						const auto total_cohesion = cohesion_mult * total_startle_applied;

						velocity += augs::arrive(
							velocity,
							pos,
							average_pos,
							velocity.length(),
							cohesion_zone_radius
						) * total_cohesion;
					}

					if (alignment_mult != 0.f) {
						const auto desired_vel = average_vel.set_length(velocity.length());
						const auto steering = desired_vel - velocity;

						velocity += steering * alignment_mult;
					}
				}

				const auto total_speed = velocity.length();

				const auto bound_avoidance = augs::steer_to_avoid_bounds(
					velocity,
					tip_pos,
					bound,
					40.f,
					0.2f
				);

				velocity += bound_avoidance;

				for (auto& startle : movement_path.startle) {
					/* 
						Decrease startle vectors when nearing the bounds,
						to avoid a glitch where fish is conflicted about where to go.
					*/

					if (startle + bound_avoidance * 6 < startle) {
						startle += bound_avoidance * 6;
					}
				}

				movement_path.last_speed = total_speed;

				transform.pos += velocity * delta.in_seconds();
				transform.rotation = velocity.degrees();//augs::interp(transform.rotation, velocity.degrees(), 50.f * delta.in_seconds());

				const auto speed_mult = total_speed / max_speed;
				const auto elapsed_anim_ms = delta.in_milliseconds() * speed_mult;

				const auto& bubble_effect = def.bubble_effect;

				if (bubble_effect.id.is_set()) {
					/* Resolve bubbles and bubble intervals */

					auto& next_in_ms = movement_path.next_bubble_in_ms;
					auto rng = cosm.get_rng_for(subject);
					
					auto choose_new_interval = [&rng, &next_in_ms, &def]() {
						const auto interval = def.base_bubble_interval_ms;
						const auto h = interval / 1.5f;

						next_in_ms = rng.randval(interval - h, interval + h);
					};

					if (next_in_ms < 0.f) {
						choose_new_interval();
					}
					else {
						next_in_ms -= elapsed_anim_ms;

						if (next_in_ms < 0.f) {
							bubble_effect.start(
								step,
								particle_effect_start_input::fire_and_forget(tip_pos)
							);
						}
					}
				}

				auto& anim_state = subject.template get<components::animation>().state;
				anim_state.frame_elapsed_ms += elapsed_anim_ms;
			}

			npo.infer_cache_for(subject);
		}
	);
}
