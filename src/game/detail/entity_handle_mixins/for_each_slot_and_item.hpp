#pragma once
#include "game/detail/entity_handle_mixins/inventory_mixin.h"
#include "game/detail/inventory/direct_attachment_offset.h"

template <class E>
template <class S, class I>
callback_result inventory_mixin<E>::for_each_contained_slot_and_item_recursive(
	S slot_callback, 
	I item_callback
) const {
	const auto this_container = *static_cast<const E*>(this);
	auto& cosm = this_container.get_cosmos();

	if (const auto container = this_container.template find<invariants::container>()) {
		for (const auto& s : container->slots) {
			const auto this_slot_id = inventory_slot_id(s.first, this_container.get_id());
			const auto slot_callback_result = slot_callback(cosm[this_slot_id]);

			if (slot_callback_result == recursive_callback_result::ABORT) {
				return callback_result::ABORT;
			}
			else if (slot_callback_result == recursive_callback_result::CONTINUE_DONT_RECURSE) {
				continue;
			}
			else if (slot_callback_result == recursive_callback_result::CONTINUE_AND_RECURSE) {
				for (const auto& id : get_items_inside(this_container, s.first)) {
					const auto child_item_handle = cosm[id];
					const auto item_callback_result = item_callback(child_item_handle);

					if (item_callback_result == recursive_callback_result::ABORT) {
						return callback_result::ABORT;
					}
					else if (item_callback_result == recursive_callback_result::CONTINUE_DONT_RECURSE) {
						continue;
					}
					else if (item_callback_result == recursive_callback_result::CONTINUE_AND_RECURSE) {
						if (child_item_handle.for_each_contained_slot_and_item_recursive(
							slot_callback,
							item_callback
						) == callback_result::ABORT) {
							return callback_result::ABORT;
						}
					}
					else {
						ensure(false && "bad recursive_callback_result");
					}
				}
			}
			else {
				ensure(false && "bad recursive_callback_result");
			}
		}
	}

	return callback_result::CONTINUE;
}

template <class E>
template <class A, class G>
void inventory_mixin<E>::for_each_attachment_recursive(
	A attachment_callback,
	G get_offsets_by_torso,
	const attachment_offset_settings& settings
) const {
	struct node {
		inventory_slot_id parent;
		entity_id child;
		transformr offset;
	};

	thread_local std::vector<node> container_stack;
	container_stack.clear();

	const auto root_container = *static_cast<const E*>(this);
	auto& cosm = root_container.get_cosmos();

	container_stack.push_back({ {}, root_container, transformr() });

	while (!container_stack.empty()) {
		const auto it = container_stack.back();
		container_stack.pop_back();

		cosm[it.child].dispatch(
			[&](const auto this_attachment) {
				auto current_offset = it.offset;

				if (it.parent.container_entity.is_set()) {
					/* Don't do it for the root */
					const auto direct_offset = direct_attachment_offset(
						cosm[it.parent.container_entity],
						this_attachment,
						get_offsets_by_torso,
						settings,
						it.parent.type
					);

					current_offset *= direct_offset;
					attachment_callback(this_attachment, current_offset);
				}

				const auto& this_container = this_attachment;

				if (const auto container = this_container.template find<invariants::container>()) {
					for (const auto& s : container->slots) {
						const auto type = s.first;

						if (s.second.makes_physical_connection()) {
							const auto this_container_id = this_container.get_id();

							for (const auto& id : get_items_inside(this_container, type)) {
								container_stack.push_back({ { s.first, this_container_id }, id, current_offset });
							}
						}
					}
				}
			}
		);
	}
}

template <class E>
template <class I>
void inventory_mixin<E>::for_each_contained_item_recursive(I&& item_callback) const {
	for_each_contained_slot_and_item_recursive(
		[](auto) { return recursive_callback_result::CONTINUE_AND_RECURSE; }, 
		std::forward<I>(item_callback)
	);
}

template <class E>
template <class S>
void inventory_mixin<E>::for_each_contained_slot_recursive(S&& slot_callback) const {
	for_each_contained_slot_and_item_recursive(
		std::forward<S>(slot_callback), 
		[](auto...) { return recursive_callback_result::CONTINUE_AND_RECURSE; }
	);
}

template <class E>
template <class G>
ltrb inventory_mixin<E>::calc_attachments_aabb(G&& get_offsets_by_torso) const {
	ltrb result;

	for_each_attachment_recursive(
		[&result](
			const auto attachment_entity,
			const auto attachment_offset
		) {
			result.contain(attachment_entity.get_aabb(attachment_offset));
		},
		std::forward<G>(get_offsets_by_torso),
		attachment_offset_settings::for_logic()
	);

	return result;
}