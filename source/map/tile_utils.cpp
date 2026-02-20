//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "map/tile_utils.h"
#include "map/tile.h"
#include "game/item.h"
#include "game/creature.h"
#include "game/spawn.h"

#include <algorithm>
#include <ranges>
#include <iterator>

namespace TileUtils {

void merge(Tile* dest, Tile* src) {
	if (src->isPZ()) {
		dest->setPZ(true);
	}
	if (src->house_id) {
		dest->house_id = src->house_id;
	}

	if (src->ground) {
		dest->ground = std::move(src->ground);
	}

	if (src->creature) {
		dest->creature = std::move(src->creature);
	}

	if (src->spawn) {
		dest->spawn = std::move(src->spawn);
	}

	dest->items.reserve(dest->items.size() + src->items.size());
	for (auto& item : src->items) {
		dest->addItem(std::move(item));
	}
	src->items.clear();
	dest->update();
}

bool isContentEqual(const Tile* a, const Tile* b) {
	if (!a || !b) {
		return false;
	}

	// Compare ground
	if (a->ground != nullptr && b->ground != nullptr) {
		if (a->ground->getID() != b->ground->getID() || a->ground->getSubtype() != b->ground->getSubtype()) {
			return false;
		}
	} else if (a->ground != b->ground) {
		return false;
	}

	// Compare items
	if (a->items.size() != b->items.size()) {
		return false;
	}

	return std::equal(a->items.begin(), a->items.end(), b->items.begin(), b->items.end(), [](const std::unique_ptr<Item>& it1, const std::unique_ptr<Item>& it2) {
		return it1->getID() == it2->getID() && it1->getSubtype() == it2->getSubtype();
	});
}

std::vector<std::unique_ptr<Item>> popSelectedItems(Tile* tile, bool ignoreTileSelected) {
	std::vector<std::unique_ptr<Item>> pop_items;

	if (!ignoreTileSelected && !tile->isSelected()) {
		return pop_items;
	}

	if (tile->ground && tile->ground->isSelected()) {
		pop_items.push_back(std::move(tile->ground));
	}

	auto split_point = std::stable_partition(tile->items.begin(), tile->items.end(), [](const std::unique_ptr<Item>& i) { return !i->isSelected(); });
	std::move(split_point, tile->items.end(), std::back_inserter(pop_items));
	tile->items.erase(split_point, tile->items.end());

	tile->unsetStatFlags(TILESTATE_SELECTED);
	return pop_items;
}

ItemVector getSelectedItems(Tile* tile, bool unzoomed) {
	ItemVector selected_items;

	if (!tile->isSelected()) {
		return selected_items;
	}

	if (tile->ground && tile->ground->isSelected()) {
		selected_items.push_back(tile->ground.get());
	}

	// save performance when zoomed out
	if (!unzoomed) {
		std::ranges::for_each(tile->items, [&](const auto& item) {
			if (item->isSelected()) {
				selected_items.push_back(item.get());
			}
		});
	}

	return selected_items;
}

} // namespace TileUtils
