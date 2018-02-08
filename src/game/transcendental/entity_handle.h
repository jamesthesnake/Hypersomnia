#pragma once
#include "3rdparty/sol2/sol/forward.hpp"

#include "augs/build_settings/platform_defines.h"
#include "augs/templates/get_by_dynamic_id.h"

#include "augs/templates/maybe_const.h"
#include "augs/templates/component_traits.h"
#include "augs/templates/type_matching_and_indexing.h"

#include "game/detail/inventory/inventory_slot_handle_declaration.h"
#include "game/transcendental/entity_handle_declaration.h"
#include "game/transcendental/cosmos_solvable_access.h"
#include "game/transcendental/entity_id.h"
#include "game/transcendental/entity_solvable.h"

#include "game/organization/all_components_declaration.h"

#include "game/detail/entity_handle_mixins/all_handle_mixins.h"
#include "game/enums/entity_flag.h"

#include "game/transcendental/step_declaration.h"
#include "game/components/flags_component.h"

class cosmos;

template <class, class>
class component_synchronizer;

template <bool is_const>
class basic_entity_handle :
	public misc_mixin<basic_entity_handle<is_const>>,
	public inventory_mixin<basic_entity_handle<is_const>>,
	public physics_mixin<basic_entity_handle<is_const>>,
	public relations_mixin<basic_entity_handle<is_const>>,
	public spatial_properties_mixin<basic_entity_handle<is_const>>
{
	using owner_reference = maybe_const_ref_t<is_const, cosmos>;
	using entity_ptr = maybe_const_ptr_t<is_const, void>;

	using this_handle_type = basic_entity_handle<is_const>;
	using misc_base = misc_mixin<this_handle_type>;

	entity_ptr ptr;
	owner_reference owner;
	entity_id raw_id;

	template <typename T>
	static void check_component_type() {
		static_assert(is_one_of_list_v<T, component_list_t<type_list>>, "Unknown component type!");
	}

	template <typename T>
	static void check_invariant_type() {
		static_assert(is_one_of_list_v<T, invariant_list_t<type_list>>, "Unknown invariant type!");
	}

	auto& pool_provider() const {
		return owner.get_solvable({});
	}

	basic_entity_handle(
		const entity_ptr ptr,
		owner_reference owner,
		const entity_id raw_id
	) :
		ptr(ptr),
		owner(owner),
		raw_id(raw_id)
	{}

	template <class T>
	auto* find_ptr() const {
		return dispatch([](const auto typed_handle) { 
			return typed_handle.template find<T>(); 
		});
	}

	static auto dereference(
		owner_reference owner, 
		const entity_id raw_id
	) {
		return owner.get_solvable({}).on_entity(raw_id, [&](auto& agg) {
			return reinterpret_cast<entity_ptr>(std::addressof(agg));	
		});
	}

public:
	using const_type = basic_entity_handle<!is_const>;
	using misc_base::get_raw_flavour_id;
	friend const_type;

	basic_entity_handle(
		owner_reference owner, 
		const entity_id raw_id
	) : basic_entity_handle(
		dereference(owner, raw_id),
		owner, 
		raw_id 
	) {
	}

	const auto& get_meta() const {
		return *reinterpret_cast<entity_solvable_meta*>(ptr);
	};

	auto& get(cosmos_solvable_access) const {
		return *ptr;
	}

	const auto& get() const {
		return *ptr;
	}

	auto get_id() const {
		return raw_id;
	}

	auto get_type_id() const {
		return raw_id.type_id;
	}

	bool alive() const {
		return ptr != nullptr;
	}

	bool dead() const {
		return !alive();
	}

	auto& get_cosmos() const {
		return owner;
	}

	bool operator==(const entity_id id) const {
		return raw_id == id;
	}

	bool operator!=(const entity_id id) const {
		return !operator==(id);
	}

	template <bool C = !is_const, class = std::enable_if_t<C>>
	operator const_entity_handle() const {
		return const_entity_handle(ptr, owner, raw_id);
	}

	operator entity_id() const {
		return raw_id;
	}

	operator child_entity_id() const {
		return raw_id;
	}

	operator unversioned_entity_id() const {
		return raw_id;
	}

	operator bool() const {
		return alive();
	}

	template <class F>
	decltype(auto) dispatch(F&& callback) {
		ensure(alive());

		return get_by_dynamic_id(
			all_entity_types(),
			raw_id.type_id,
			[&](auto t) {
				using entity_type = decltype(t);
				using handle_type = basic_typed_entity_handle<is_const, entity_type>;

				auto& specific_ref = 
					*reinterpret_cast<
						maybe_const_ptr_t<is_const>(entity_solvable<entity_type>)
					>(ptr)
				;
					
				return callback(handle_type(specific_ref, owner, get_id()));
			}
		);
	}

	template <class T>
	bool has() const {
		return dispatch([](const auto typed_handle) { 
			return typed_handle.template has<T>(); 
		});
	}

	template<class T>
	decltype(auto) find() const {
		if constexpr(is_synchronized_v<T>) {
			return component_synchronizer<this_handle_type, T>(find_ptr<T>(), *this);
		}
		else {
			return find_ptr<T>();
		}
	}

	template<class T>
	decltype(auto) get() const {
		if constexpr(is_synchronized_v<T>) {
			return component_synchronizer<this_handle_type, T>(find_ptr<T>(), *this);
		}
		else {
			return *find_ptr<T>();
		}
	}

	template <class F>
	void for_each_component(F&& callback) const {
		dispatch([&](const auto typed_handle) {
			typed_handle.for_each_component(std::forward<F>(callback));
		});
	}
};

inline std::ostream& operator<<(std::ostream& out, const entity_handle &x) {
	return out << typesafe_sprintf("%x %x", to_string(x.get_name()), x.get_id());
}

inline std::ostream& operator<<(std::ostream& out, const const_entity_handle &x) {
	return out << typesafe_sprintf("%x %x", to_string(x.get_name()), x.get_id());
}
