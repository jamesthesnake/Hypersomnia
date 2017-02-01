#include "cosmic_entropy.h"
#include "game/transcendental/cosmos.h"

#include "augs/misc/machine_entropy.h"
#include "game/global/input_context.h"

#include "game/detail/inventory_utils.h"

template <class key>
void basic_cosmic_entropy<key>::override_transfers_leaving_other_entities(
	const cosmos& cosm,
	std::vector<item_slot_transfer_request_data> new_transfers
) {
	erase_remove(transfer_requests, [&](const item_slot_transfer_request_data o) {
		const auto overridden_transfer = match_transfer_capabilities(cosm[o]);
		
		ensure(overridden_transfer.is_legal());

		for (const auto n : new_transfers) {
			const auto new_transfer = cosm[n];
			
			if (
				match_transfer_capabilities(new_transfer).authorized_capability
				== overridden_transfer.authorized_capability
			) {
				return true;
			}
		}

		return false;
	});

	concatenate(transfer_requests, new_transfers);
}

template <class key>
size_t basic_cosmic_entropy<key>::length() const {
	size_t total = 0;

	for (const auto& ent : entropy_per_entity) {
		total += ent.second.size();
	}

	total += transfer_requests.size();

	return total;
}

template <class key>
basic_cosmic_entropy<key>& basic_cosmic_entropy<key>::operator+=(const basic_cosmic_entropy& b) {
	for (const auto& new_entropy : b.entropy_per_entity) {
		concatenate(entropy_per_entity[new_entropy.first], new_entropy.second);
	}

	concatenate(transfer_requests, b.transfer_requests);

	return *this;
}

guid_mapped_entropy::guid_mapped_entropy(const cosmic_entropy& b, const cosmos& mapper) {
	for (const auto& entry : b.entropy_per_entity) {
		entropy_per_entity[mapper[entry.first].get_guid()] = entry.second;
	}
}

bool guid_mapped_entropy::operator!=(const guid_mapped_entropy& b) const {
	if (entropy_per_entity.size() != b.entropy_per_entity.size())
		return true;

	for (const auto& entry : b.entropy_per_entity) {
		auto found = entropy_per_entity.find(entry.first);
	
		if (found == entropy_per_entity.end())
			return true;
	
		if (entry.second != (*found).second)
			return true;
	}

	return false;
}

cosmic_entropy::cosmic_entropy(const guid_mapped_entropy& b, const cosmos& mapper) {
	for (const auto& entry : b.entropy_per_entity) {
		entropy_per_entity[mapper.get_entity_by_guid(entry.first).get_id()] = entry.second;
	}
}

cosmic_entropy::cosmic_entropy(
	const const_entity_handle controlled_entity,
	const std::vector<key_and_mouse_intent>& intents
) {
	entropy_per_entity[controlled_entity] = intents;
}

template struct basic_cosmic_entropy<unsigned>;
template struct basic_cosmic_entropy<entity_id>;