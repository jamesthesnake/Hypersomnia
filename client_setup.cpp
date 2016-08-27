#include <thread>
#include "game/bindings/bind_game_and_augs.h"
#include "augs/global_libraries.h"
#include "game/game_window.h"

#include "game/resources/manager.h"

#include "game/scene_managers/testbed.h"
#include "game/scene_managers/resource_setups/all.h"

#include "game/transcendental/types_specification/all_component_includes.h"
#include "game/transcendental/viewing_session.h"
#include "game/transcendental/simulation_receiver.h"
#include "game/transcendental/cosmos.h"

#include "game/transcendental/step_and_entropy_unpacker.h"

#include "game/transcendental/network_commands.h"
#include "game/transcendental/cosmic_delta.h"

#include "augs/filesystem/file.h"

#include "augs/network/network_client.h"

#include "augs/misc/templated_readwrite.h"
#include "setups.h"

void client_setup::process(game_window& window) {
	const vec2i screen_size = vec2i(window.window.get_screen_rect());

	cosmos hypersomnia(3000);

	cosmos initial_hypersomnia(3000);
	scene_managers::testbed().populate_world_with_entities(initial_hypersomnia);

	step_and_entropy_unpacker input_unpacker;
	scene_managers::testbed testbed;

	if (!hypersomnia.load_from_file("save.state"))
		testbed.populate_world_with_entities(hypersomnia);

	input_unpacker.try_to_load_or_save_new_session("sessions/", "recorded.inputs");

	viewing_session session;
	session.camera.configure_size(screen_size);

	testbed.configure_view(session);

	augs::network::client client;
	simulation_receiver receiver;

	cosmos hypersomnia_last_snapshot(3000);
	cosmos extrapolated_hypersomnia(3000);

	bool should_quit = false;

	bool last_stepped_was_extrapolated = false;

	if (client.connect(window.get_config_string("server_address"), static_cast<unsigned short>(window.get_config_number("server_port")), 2000)) {
		LOG("Connected successfully");
		
		while (!should_quit) {
			augs::machine_entropy new_entropy;

			new_entropy.local = window.collect_entropy();
			new_entropy.remote = client.collect_entropy();

			for (auto& n : new_entropy.local) {
				if (n.key == augs::window::event::keys::ESC && n.key_event == augs::window::event::key_changed::PRESSED) {
					should_quit = true;
				}
			}

			input_unpacker.control(new_entropy);

			auto steps = input_unpacker.unpack_steps(hypersomnia.get_fixed_delta());

			for (auto& s : steps) {
				for (auto& net_event : s.total_entropy.remote) {
					if (net_event.message_type == augs::network::message::type::RECEIVE) {
						auto& stream = net_event.payload;

						while (stream.get_unread_bytes() > 0) {
							const auto command = static_cast<network_command>(stream.peek<unsigned char>());
							
							switch (command) {
							case network_command::COMPLETE_STATE:
								network_command read_command;

								augs::read_object(stream, read_command);
								ensure_eq(int(network_command::COMPLETE_STATE), int(read_command));

								cosmic_delta::decode(initial_hypersomnia, stream);
								hypersomnia = initial_hypersomnia;

								break;

							case network_command::ENTROPY_FOR_NEXT_STEP:
								receiver.read_entropy_for_next_step(stream);
								break;

							case network_command::ENTROPY_WITH_HEARTBEAT_FOR_NEXT_STEP:
								receiver.read_entropy_with_heartbeat_for_next_step(stream);
								break;
							}
						}
					}
				}

				const auto local_cosmic_entropy_for_this_step = testbed.make_cosmic_entropy(s.total_entropy.local, session.input, hypersomnia);

				simulation_exchange::packaged_step net_step;
				net_step.step_type = simulation_exchange::packaged_step::type::NEW_ENTROPY;
				net_step.entropy = local_cosmic_entropy_for_this_step;

				augs::stream serialized_step;
				simulation_exchange::write_packaged_step_to_stream(serialized_step, net_step, hypersomnia);

				client.post_redundant(serialized_step);
				client.send_pending_redundant();

				auto deterministic_steps = receiver.unpack_deterministic_steps(hypersomnia, extrapolated_hypersomnia, hypersomnia_last_snapshot);

				if (deterministic_steps.use_extrapolated_cosmos) {
					testbed.step_with_callbacks(cosmic_entropy(), extrapolated_hypersomnia);
					renderer::get_current().clear_logic_lines();
					last_stepped_was_extrapolated = true;
				}
				else {
					last_stepped_was_extrapolated = false;

					ensure(deterministic_steps.has_next_entropy());

					while (deterministic_steps.has_next_entropy()) {
						const auto cosmic_entropy_for_this_step = deterministic_steps.unpack_next_entropy(hypersomnia);
						testbed.step_with_callbacks(cosmic_entropy_for_this_step, hypersomnia);
						renderer::get_current().clear_logic_lines();
					}
				}
			}

			testbed.view(last_stepped_was_extrapolated ? hypersomnia : extrapolated_hypersomnia, 
				window, session, session.frame_timer.extract_variable_delta(hypersomnia.get_fixed_delta(), input_unpacker.timer));
		}
	}
	else {
		LOG("Connection failed.");
	}
}