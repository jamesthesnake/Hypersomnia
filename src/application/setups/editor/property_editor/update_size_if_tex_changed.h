#pragma once
#include "game/assets/ids/asset_ids.h"

#include "application/setups/editor/editor_history.h"
#include "application/setups/editor/editor_command_input.h"
#include "application/setups/editor/property_editor/fae_tree_structs.h"
#include "application/setups/editor/commands/change_flavour_property_command.h"
#include "application/setups/editor/property_editor/component_field_eq_predicate.h"

template <class T, class C>
void update_size_if_tex_changed(
	const edit_invariant_input in,
	const T& invariant,
	const C& new_content
) {}

inline void update_size_if_tex_changed(
	const edit_invariant_input in,
	const invariants::sprite& invariant,
	const assets::image_id& new_content
) {
	const auto id = new_content;

	const auto fae_in = in.fae_in;
	const auto cpe_in = fae_in.cpe_in;
	const auto& image_caches = fae_in.image_caches;
	const auto& viewables = cpe_in.command_in.folder.work->viewables;

	image_cache cache;

	if (const auto c = mapped_or_nullptr(image_caches, id)) {
		cache = *c;
	}
	else {
		cache = { viewables.image_loadables[id], viewables.image_metas[id] };
	}

	auto& history = cpe_in.command_in.folder.history;

	{
		auto cmd = in.command;

		{
			const auto& size = invariant.size;
			cmd.property_id = flavour_property_id { in.invariant_id, make_field_address(size, invariant) };
		}

		{
			using T = decltype(invariants::sprite::size);
			const auto original_size = T(cache.original_image_size);

			cmd.value_after_change = augs::to_bytes(original_size);
		}

		cmd.built_description = "Set sprite size to image dimensions";
		cmd.common.has_parent = true;

		history.execute_new(std::move(cmd), cpe_in.command_in);
	}

	if (const auto invariant_id = in.shape_polygon_invariant_id) {
		auto cmd = in.command;

		{
			invariants::shape_polygon shape_invariant;
			const auto& shape = shape_invariant.shape;

			cmd.property_id = flavour_property_id { *invariant_id, make_field_address(shape, shape_invariant) };
		}

		{
			using T = decltype(invariants::shape_polygon::shape);

			const auto original_shape = T(cache.partitioned_shape);
			cmd.value_after_change = augs::to_bytes(original_shape);
		}

		cmd.built_description = "Update physics shape to match sprite size";
		cmd.common.has_parent = true;

		history.execute_new(std::move(cmd), cpe_in.command_in);
	}
}
