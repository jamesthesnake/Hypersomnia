#pragma once
#include "augs/misc/machine_entropy.h"
#include "game/cosmic_entropy.h"

#include "misc/step_player.h"

class entropy_player {
public:
	augs::machine_entropy total_buffered_entropy;

	augs::step_player<decltype(augs::machine_entropy::local)> local_entropy_player;

	void buffer_entropy_for_next_step(augs::machine_entropy);
	cosmic_entropy obtain_cosmic_entropy_for_next_step(const cosmos&);

	void record_and_save_this_session(std::string folder, std::string filename);
	
	bool try_to_load_and_replay_recording(std::string filename);
	bool is_replaying() const;
};