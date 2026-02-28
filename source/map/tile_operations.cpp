//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "map/tile_operations.h"
#include "map/tile.h"
#include "map/basemap.h"
#include "game/item.h"
#include "game/house.h"
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
		void cleanItems(Tile* tile, Predicate p) {
			auto& items = tile->items;
			auto first_to_remove = std::stable_partition(items.begin(), items.end(), std::not_fn(p));

			items.erase(first_to_remove, items.end());
		}

		void UpdateItemFlags(const Item* i, uint16_t& statflags, uint8_t& minimapColor) {
			if (i->isSelected()) {
				statflags |= TILESTATE_SELECTED;
			}
			if (i->getUniqueID() != 0) {
				statflags |= TILESTATE_UNIQUE;
			}
			if (i->getMiniMapColor() != 0) {
				minimapColor = i->getMiniMapColor();
			}

			const ItemType& it_type = g_items[i->getID()];
			if (it_type.unpassable) {
				statflags |= TILESTATE_BLOCKING;
			}
			if (it_type.isOptionalBorder) {
				statflags |= TILESTATE_OP_BORDER;
			}
			if (it_type.isTable) {
				statflags |= TILESTATE_HAS_TABLE;
			}
			if (it_type.isCarpet) {
				statflags |= TILESTATE_HAS_CARPET;
			}
			if (it_type.hookSouth) {
				statflags |= TILESTATE_HOOK_SOUTH;
			}
			if (it_type.hookEast) {
				statflags |= TILESTATE_HOOK_EAST;
			}
			if (i->hasLight()) {
				statflags |= TILESTATE_HAS_LIGHT;
			}
		}

	} // anonymous namespace

	void borderize(Tile* tile, BaseMap* map) {
		GroundBrush::doBorders(map, tile);
	}

	void cleanBorders(Tile* tile) {
		cleanItems(tile, [](const auto& item) { return item->isBorder(); });
	}

	void wallize(Tile* tile, BaseMap* map) {
		WallBrush::doWalls(map, tile);
	}

	void cleanWalls(Tile* tile) {
		cleanItems(tile, [](const auto& item) { return item->isWall(); });
	}

	void cleanWalls(Tile* tile, WallBrush* wb) {
		cleanItems(tile, [wb](const auto& item) { return item->isWall() && wb->hasWall(item.get()); });
	}

	void tableize(Tile* tile, BaseMap* map) {
		TableBrush::doTables(map, tile);
	}

	void cleanTables(Tile* tile) {
		cleanItems(tile, [](const auto& item) { return item->isTable(); });
	}

	void carpetize(Tile* tile, BaseMap* map) {
		CarpetBrush::doCarpets(map, tile);
	}

	// Implementation of moved methods

	std::unique_ptr<Tile> deepCopy(const Tile* tile, BaseMap& map) {
		// Use the destination map's TileLocation at the same position
		TileLocation* dest_location = map.getTileL(tile->getX(), tile->getY(), tile->getZ());
		if (!dest_location) {
			dest_location = map.createTileL(tile->getX(), tile->getY(), tile->getZ());
		}
		std::unique_ptr<Tile> copy(map.allocator.allocateTile(dest_location));
		copy->mapflags = tile->mapflags;
		copy->statflags = tile->statflags;
		copy->minimapColor = tile->minimapColor;
		copy->house_id = tile->house_id;
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
		std::ranges::transform(tile->items, std::back_inserter(copy->items), [](const auto& item) {
			return item->deepCopy();
		});

		return copy;
	}

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
		TileOperations::update(dest);
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

		tile->statflags |= TILESTATE_SELECTED;
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

		tile->statflags &= ~TILESTATE_SELECTED;
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
			tile->statflags |= TILESTATE_SELECTED;
		}
	}

	void deselectGround(Tile* tile) {
		if (tile->ground) {
			tile->ground->deselect();
		}

		for (const auto& i : tile->items | std::views::take_while([](const auto& item) { return item->isBorder(); })) {
			i->deselect();
		}

		TileOperations::update(tile);
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

		TileOperations::update(tile);
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

	void addBorderItem(Tile* tile, std::unique_ptr<Item> item) {
		if (!item) {
			return;
		}
		ASSERT(item->isBorder());
		tile->items.insert(tile->items.begin(), std::move(item));
		TileOperations::update(tile);
	}

	void addWallItem(Tile* tile, std::unique_ptr<Item> item) {
		if (!item) {
			return;
		}
		ASSERT(item->isWall());
		tile->addItem(std::move(item));
	}

	void addHouseExit(Tile* tile, House* h) {
		if (!h) {
			return;
		}
		HouseExitList* house_exits = tile->location->createHouseExits();
		house_exits->push_back(h->getID());
	}

	void removeHouseExit(Tile* tile, House* h) {
		if (!h) {
			return;
		}

		HouseExitList* house_exits = tile->location->getHouseExits();
		if (!house_exits) {
			return;
		}

		std::erase(*house_exits, h->getID());
	}

	void update(Tile* tile) {
		tile->statflags &= TILESTATE_MODIFIED;

		if (tile->spawn && tile->spawn->isSelected()) {
			tile->statflags |= TILESTATE_SELECTED;
		}
		if (tile->creature && tile->creature->isSelected()) {
			tile->statflags |= TILESTATE_SELECTED;
		}

		tile->minimapColor = 0; // Reset to "no color" (valid)

		if (tile->ground) {
			if (tile->ground->isSelected()) {
				tile->statflags |= TILESTATE_SELECTED;
			}
			if (tile->ground->isBlocking()) {
				tile->statflags |= TILESTATE_BLOCKING;
			}
			if (tile->ground->getUniqueID() != 0) {
				tile->statflags |= TILESTATE_UNIQUE;
			}
			if (tile->ground->getMiniMapColor() != 0) {
				tile->minimapColor = tile->ground->getMiniMapColor();
			}
			if (tile->ground->hasLight()) {
				tile->statflags |= TILESTATE_HAS_LIGHT;
			}
		}

		std::ranges::for_each(tile->items, [&](const auto& i) {
			UpdateItemFlags(i.get(), tile->statflags, tile->minimapColor);
		});

		if ((tile->statflags & TILESTATE_BLOCKING) == 0) {
			if (tile->ground == nullptr && tile->items.empty()) {
				tile->statflags |= TILESTATE_BLOCKING;
			}
		}
	}

} // namespace TileOperations
