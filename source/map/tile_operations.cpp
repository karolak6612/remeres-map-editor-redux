//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "map/tile_operations.h"
#include "map/tile.h"
#include "map/map_region.h"
#include "map/map_allocator.h"
#include "map/basemap.h"
#include "game/item.h"
#include "game/spawn.h"
#include "game/creature.h"
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

	void merge(Tile* tile, Tile* other) {
		if (other->isPZ()) {
			tile->setPZ(true);
		}
		if (other->house_id) {
			tile->house_id = other->house_id;
		}

		if (other->ground) {
			tile->ground = std::move(other->ground);
		}

		if (other->creature) {
			tile->creature = std::move(other->creature);
		}

		if (other->spawn) {
			tile->spawn = std::move(other->spawn);
		}

		tile->items.reserve(tile->items.size() + other->items.size());
		for (auto& item : other->items) {
			tile->addItem(std::move(item));
		}
		other->items.clear();
		tile->update();
	}

	bool isContentEqual(const Tile* tile, const Tile* other) {
		if (!other || !tile) {
			return false;
		}

		// Compare ground
		if (tile->ground != nullptr && other->ground != nullptr) {
			if (tile->ground->getID() != other->ground->getID() || tile->ground->getSubtype() != other->ground->getSubtype()) {
				return false;
			}
		} else if (tile->ground != other->ground) {
			return false;
		}

		// Compare items
		return std::ranges::equal(tile->items, other->items, [](const std::unique_ptr<Item>& it1, const std::unique_ptr<Item>& it2) {
			return it1->getID() == it2->getID() && it1->getSubtype() == it2->getSubtype();
		});
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

		// Ensure minimapColor and other derived flags are updated
		copy->update();

		return copy;
	}

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

	void addBorderItem(Tile* tile, std::unique_ptr<Item> item) {
		if (!item) {
			return;
		}
		ASSERT(item->isBorder());
		tile->items.insert(tile->items.begin(), std::move(item));
		tile->update();
	}

	void addWallItem(Tile* tile, std::unique_ptr<Item> item) {
		if (!item) {
			return;
		}
		ASSERT(item->isWall());

		tile->addItem(std::move(item));
	}

} // namespace TileOperations
