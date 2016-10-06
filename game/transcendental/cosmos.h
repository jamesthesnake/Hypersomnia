#pragma once
#include <thread>
#include "game/build_settings.h"
#include "augs/misc/streams.h"
#include "game/transcendental/cosmic_entropy.h"

#include "augs/entity_system/storage_for_components_and_aggregates.h"
#include "augs/entity_system/storage_for_message_queues.h"
#include "augs/entity_system/storage_for_systems.h"

#include "game/transcendental/types_specification/all_components_declaration.h"
#include "game/transcendental/types_specification/all_messages_declaration.h"
#include "game/transcendental/types_specification/all_systems_declaration.h"

#include "game/systems_insignificant/interpolation_system.h"
#include "game/systems_insignificant/past_infection_system.h"
#include "game/systems_temporary/dynamic_tree_system.h"
#include "game/systems_temporary/physics_system.h"
#include "game/systems_temporary/processing_lists_system.h"

#include "augs/misc/delta.h"
#include "augs/misc/pool_handlizer.h"

#include "game/transcendental/entity_id.h"
#include "game/detail/inventory_slot_id.h"
#include "game/transcendental/entity_handle_declaration.h"
#include "game/detail/inventory_slot_handle_declaration.h"
#include "game/global/all_settings.h"
#include "game/transcendental/cosmic_profiler.h"

class cosmic_delta;

class cosmos : private storage_for_all_components_and_aggregates, public augs::pool_handlizer<cosmos>
{
	void advance_deterministic_schemata(fixed_step& step_state);

#if COSMOS_TRACKS_GUIDS
	friend class cosmic_delta;

	std::map<unsigned, entity_id> guid_map_for_transport;

	void assign_next_guid(entity_handle);
	void clear_guid(entity_handle);
	unsigned get_guid(const_entity_handle) const;
public:
	void remap_guids();
private:
	entity_handle create_entity_with_specific_guid(std::string debug_name, unsigned specific_guid);
#endif

	void destroy_substance_completely();
	void create_substance_completely();

public:
	storage_for_all_systems_temporary systems_temporary;
	mutable storage_for_all_systems_insignificant systems_insignificant;

	mutable cosmic_profiler profiler;
	augs::stream reserved_memory_for_serialization;

	class significant_state {
	public:
		class metadata {
			friend class cosmos;

			augs::fixed_delta delta;
			unsigned total_steps_passed = 0;

#if COSMOS_TRACKS_GUIDS
			unsigned next_entity_guid = 1;
#endif
		public:
			all_settings settings;

		} meta;

		aggregate_pool_type pool_for_aggregates;
		component_pools_type pools_for_components;

		template <class Archive>
		void serialize(Archive& ar) {
			ar(
				CEREAL_NVP(settings),

				CEREAL_NVP(delta),

				CEREAL_NVP(pool_for_aggregates),
				CEREAL_NVP(pools_for_components)
			);
		}

		bool operator==(const significant_state&) const;
		bool operator!=(const significant_state&) const;

	} significant;

	template <class Archive>
	void serialize(Archive& ar) {
		ar(CEREAL_NVP(significant));
		complete_resubstantiation();
	}

	cosmos(unsigned reserved_entities = 0);
	cosmos(const cosmos&);

	cosmos& operator=(const cosmos&);
	cosmos& operator=(const significant_state&);

	bool operator==(const cosmos&) const;
	bool operator!=(const cosmos&) const;

	template<class Pre, class Post>
	void advance_deterministic_schemata(cosmic_entropy input, Pre pre_solve, Post post_solve) {
		fixed_step step(*this, input);

		pre_solve(step);
		advance_deterministic_schemata(step);
		post_solve(step);
	}

	void integrate_interpolated_transforms(const float seconds) const;
	void advance_deterministic_schemata(cosmic_entropy input);

	void reserve_storage_for_entities(size_t);

	entity_handle create_entity(std::string debug_name);
	entity_handle clone_entity(entity_id);
	void delete_entity(entity_id);

	void refresh_for_new_significant_state();

	void complete_resubstantiation();
	void complete_resubstantiation(const_entity_handle);
	void destroy_substance_for_entity(const_entity_handle);
	void create_substance_for_entity(const_entity_handle);

	template<class System>
	void partial_resubstantiation(entity_handle handle) {
		auto& sys = systems_temporary.get<System>();

		sys.destruct(handle);

		if (handle.has<components::substance>())
			sys.construct(handle);
	}

	template<class element_type>
	auto get_handle(augs::pool_id<element_type> id) {
		return get_pool(id)[id];
	}

	template<class element_type>
	auto get_handle(augs::pool_id<element_type> id) const {
		return get_pool(id)[id];
	}

#if COSMOS_TRACKS_GUIDS
	entity_handle get_entity_by_guid(unsigned);
	const_entity_handle get_entity_by_guid(unsigned) const;
	bool entity_exists_with_guid(unsigned) const;
#endif

	entity_handle get_handle(entity_id);
	const_entity_handle get_handle(entity_id) const;
	inventory_slot_handle get_handle(inventory_slot_id);
	const_inventory_slot_handle get_handle(inventory_slot_id) const;

	randomization get_rng_for(entity_id) const;

	std::vector<entity_handle> get(processing_subjects);
	std::vector<const_entity_handle> get(processing_subjects) const;

	size_t entities_count() const;
	size_t get_maximum_entities() const;
	std::wstring summary() const;

	float get_total_time_passed_in_seconds(float view_interpolation_ratio) const;
	float get_total_time_passed_in_seconds() const;
	unsigned get_total_steps_passed() const;

	augs::stepped_timestamp get_timestamp() const;

	const augs::fixed_delta& get_fixed_delta() const;
	void set_fixed_delta(const augs::fixed_delta&);

	void save_to_file(std::string);
	bool load_from_file(std::string);

	template <class D>
	void for_each_entity_id(D pred) {
		significant.pool_for_aggregates.for_each_id(pred);
	}

	template <class D>
	void for_each_entity_id(D pred) const {
		significant.pool_for_aggregates.for_each_id(pred);
	}

	template<class T>
	auto& get_pool(augs::pool_id<T>) {
		return std::get<augs::pool<T>>(significant.pools_for_components);
	}

	template<class T>
	const auto& get_pool(augs::pool_id<T>) const {
		return std::get<augs::pool<T>>(significant.pools_for_components);
	}

	auto& get_pool(entity_id) {
		return significant.pool_for_aggregates;
	}

	const auto& get_pool(entity_id) const {
		return significant.pool_for_aggregates;
	}
};

namespace augs {
	template<class A>
	bool read_object(A& ar, cosmos::significant_state& significant) {
		return read_object(ar, significant.meta) &&
		read_object(ar, significant.pools_for_components) &&
		read_object(ar, significant.pool_for_aggregates);
	}

	template<class A>
	void write_object(A& ar, const cosmos::significant_state& significant) {
		write_object(ar, significant.meta);
		write_object(ar, significant.pools_for_components);
		write_object(ar, significant.pool_for_aggregates);
	}
}