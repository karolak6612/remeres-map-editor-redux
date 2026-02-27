//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "map/tile_operations.h"
#include "map/tile.h"
#include "map/map.h"
#include "map/map_region.h"
#include "game/item.h"
#include "game/creature.h"
#include "game/spawn.h"
#include "brushes/ground/ground_brush.h"
#include "brushes/wall/wall_brush.h"
#include "brushes/table/table_brush.h"
#include "brushes/carpet/carpet_brush.h"
#include <algorithm>
#include <functional>
#include <ranges>

namespace TileOperations {

	namespace {

		template <typename Predicate>
		void cleanItems(Tile* tile, Predicate p, bool delete_items) {
			auto& items = tile->items;
			auto first_to_remove = std::stable_partition(items.begin(), items.end(), std::not_fn(p));

			if (delete_items) {
				// With unique_ptr, elements are automatically deleted when the vector is erased or items are replaced.
				// We don't need to manually delete them here.
			}
			items.erase(first_to_remove, items.end());
		}

	} // anonymous namespace

	void borderize(Tile* tile, BaseMap* map) {
		GroundBrush::doBorders(map, tile);
	}

	void cleanBorders(Tile* tile) {
		cleanItems(tile, [](const auto& item) { return item->isBorder(); }, true);
	}

	void wallize(Tile* tile, BaseMap* map) {
		WallBrush::doWalls(map, tile);
	}

	void cleanWalls(Tile* tile, bool dontdelete) {
		cleanItems(tile, [](const auto& item) { return item->isWall(); }, !dontdelete);
	}

	void cleanWalls(Tile* tile, WallBrush* wb) {
		cleanItems(tile, [wb](const auto& item) { return item->isWall() && wb->hasWall(item.get()); }, true);
	}

	void tableize(Tile* tile, BaseMap* map) {
		TableBrush::doTables(map, tile);
	}

	void cleanTables(Tile* tile, bool dontdelete) {
		cleanItems(tile, [](const auto& item) { return item->isTable(); }, !dontdelete);
	}

	void carpetize(Tile* tile, BaseMap* map) {
		CarpetBrush::doCarpets(map, tile);
	}

	std::unique_ptr<Tile> deepCopy(const Tile* tile, BaseMap& map) {
		// Use the destination map's TileLocation at the same position
		TileLocation* dest_location = map.getTileL(tile->getX(), tile->getY(), tile->getZ());
		if (!dest_location) {
			dest_location = map.createTileL(tile->getX(), tile->getY(), tile->getZ());
		}
		std::unique_ptr<Tile> copy(map.allocator.allocateTile(dest_location));
		copy->setMapFlags(tile->getMapFlags());
		copy->setStatFlags(tile->getStatFlags());
		copy->setHouseID(tile->getHouseID());

		if (tile->spawn) {
			copy->spawn = tile->spawn->deepCopy();
		}
		if (tile->creature) {
			copy->creature = tile->creature->deepCopy();
		}
		// Spawncount & exits are not transferred on copy!
		if (tile->ground) {
			copy->ground = tile->ground->deepCopy();
		}

		copy->items.reserve(tile->items.size());
		for (const auto& item : tile->items) {
			copy->items.push_back(std::unique_ptr<Item>(item->deepCopy()));
		}

		return copy;
	}

	void merge(Tile* dest, const Tile* src) {
		if (src->isPZ()) {
			dest->setPZ(true);
		}
		if (src->getHouseID()) {
			dest->setHouseID(src->getHouseID());
		}

		if (src->ground) {
			dest->ground = src->ground->deepCopy();
		}

		if (src->creature) {
			dest->creature = src->creature->deepCopy();
		}

		if (src->spawn) {
			dest->spawn = src->spawn->deepCopy();
		}

		dest->items.reserve(dest->items.size() + src->items.size());
		for (const auto& item : src->items) {
			dest->addItem(item->deepCopy());
		}
		dest->update();
	}

	bool isContentEqual(const Tile* a, const Tile* b) {
		if (!a || !b) {
			return false;
		}
		return a->isContentEqual(b);
	}

	GroundBrush* getGroundBrush(const Tile* tile) {
		if (tile->ground) {
			if (tile->ground->getGroundBrush()) {
				return tile->ground->getGroundBrush();
			}
		}
		return nullptr;
	}

	Item* getWall(const Tile* tile) {
		auto it = std::ranges::find_if(tile->items, [](const std::unique_ptr<Item>& i) {
			return i->isWall();
		});
		return (it != tile->items.end()) ? it->get() : nullptr;
	}

	bool hasWall(const Tile* tile) {
		return getWall(tile) != nullptr;
	}

	void addWallItem(Tile* tile, std::unique_ptr<Item> item) {
		if (!item) {
			return;
		}
		ASSERT(item->isWall());

		tile->addItem(std::move(item));
	}

	Item* getCarpet(const Tile* tile) {
		auto it = std::ranges::find_if(tile->items, [](const std::unique_ptr<Item>& i) {
			return i->isCarpet();
		});
		return (it != tile->items.end()) ? it->get() : nullptr;
	}

	bool hasCarpet(const Tile* tile) {
		return getCarpet(tile) != nullptr;
	}

	Item* getTable(const Tile* tile) {
		auto it = std::ranges::find_if(tile->items, [](const std::unique_ptr<Item>& i) {
			return i->isTable();
		});
		return (it != tile->items.end()) ? it->get() : nullptr;
	}

	bool hasTable(const Tile* tile) {
		return getTable(tile) != nullptr;
	}

	void addBorderItem(Tile* tile, std::unique_ptr<Item> item) {
		if (!item) {
			return;
		}
		ASSERT(item->isBorder());
		tile->items.insert(tile->items.begin(), std::move(item));
		tile->update();
	}

	void select(Tile* tile) {
		if (tile->empty()) {
			return;
		}
		if (tile->ground) {
			tile->ground->select();
		}
		if (tile->spawn) {
			tile->spawn->select();
		}
		if (tile->creature) {
			tile->creature->select();
		}

		std::ranges::for_each(tile->items, [](const auto& i) {
			i->select();
		});

		tile->setStatFlags(tile->getStatFlags() | TILESTATE_SELECTED);
	}

	void deselect(Tile* tile) {
		if (tile->ground) {
			tile->ground->deselect();
		}
		if (tile->spawn) {
			tile->spawn->deselect();
		}
		if (tile->creature) {
			tile->creature->deselect();
		}

		std::ranges::for_each(tile->items, [](const auto& i) {
			i->deselect();
		});

		tile->unsetStatFlags(TILESTATE_SELECTED);
	}

	void selectGround(Tile* tile) {
		bool selected_ = false;
		if (tile->ground) {
			tile->ground->select();
			selected_ = true;
		}

		for (const auto& i : tile->items | std::views::take_while([](const auto& item) { return item->isBorder(); })) {
			i->select();
			selected_ = true;
		}

		if (selected_) {
			tile->setStatFlags(tile->getStatFlags() | TILESTATE_SELECTED);
		}
	}

	void deselectGround(Tile* tile) {
		if (tile->ground) {
			tile->ground->deselect();
		}

		for (const auto& i : tile->items | std::views::take_while([](const auto& item) { return item->isBorder(); })) {
			i->deselect();
		}
	}

	Item* getTopSelectedItem(Tile* tile) {
		auto view = std::ranges::reverse_view(tile->items);
		auto it = std::ranges::find_if(view, [](const std::unique_ptr<Item>& i) {
			return i->isSelected() && !i->isMetaItem();
		});

		if (it != view.end()) {
			return it->get();
		}

		if (tile->ground && tile->ground->isSelected() && !tile->ground->isMetaItem()) {
			return tile->ground.get();
		}
		return nullptr;
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

} // namespace TileOperations
