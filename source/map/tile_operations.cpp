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
#include "game/complexitem.h"
#include "game/creature.h"
#include "brushes/ground/ground_brush.h"
#include "brushes/wall/wall_brush.h"
#include "brushes/table/table_brush.h"
#include "brushes/carpet/carpet_brush.h"
#include <algorithm>
#include <functional>
#include <ranges>
#include <queue>

namespace TileOperations {

	namespace {

		template <typename Predicate>
		void cleanItems(Tile* tile, Predicate p) {
			auto& items = tile->items;
			auto first_to_remove = std::stable_partition(items.begin(), items.end(), std::not_fn(p));

			items.erase(first_to_remove, items.end());
		}

		void UpdateItemFlags(const Item* i, StatFlags& statflags, uint8_t& minimapColor) {
			if (i->isSelected()) {
				statflags |= StatFlags::Selected;
			}
			if (i->getUniqueID() != 0) {
				statflags |= StatFlags::Unique;
			}
			if (i->getMiniMapColor() != 0) {
				minimapColor = i->getMiniMapColor();
			}

			const ItemType& it_type = g_items[i->getID()];
			if (it_type.unpassable) {
				statflags |= StatFlags::Blocking;
			}
			if (it_type.isOptionalBorder) {
				statflags |= StatFlags::OpBorder;
			}
			if (it_type.isTable) {
				statflags |= StatFlags::HasTable;
			}
			if (it_type.isCarpet) {
				statflags |= StatFlags::HasCarpet;
			}
			if (it_type.hookSouth) {
				statflags |= StatFlags::HookSouth;
			}
			if (it_type.hookEast) {
				statflags |= StatFlags::HookEast;
			}
			if (i->hasLight()) {
				statflags |= StatFlags::HasLight;
			}
		}

	} // anonymous namespace

	void borderize(Tile* tile, BaseMap* map) {
		GroundBrush::doBorders(map, tile);
	}

	void cleanBorders(Tile* tile) {
		cleanItems(tile, [](const auto& item) { return item->isBorder(); });
		TileOperations::update(tile);
	}

	void wallize(Tile* tile, BaseMap* map) {
		WallBrush::doWalls(map, tile);
	}

	void cleanWalls(Tile* tile) {
		cleanItems(tile, [](const auto& item) { return item->isWall(); });
		TileOperations::update(tile);
	}

	void cleanWalls(Tile* tile, WallBrush* wb) {
		cleanItems(tile, [wb](const auto& item) { return item->isWall() && wb->hasWall(item.get()); });
		TileOperations::update(tile);
	}

	void tableize(Tile* tile, BaseMap* map) {
		TableBrush::doTables(map, tile);
	}

	void cleanTables(Tile* tile) {
		cleanItems(tile, [](const auto& item) { return item->isTable(); });
		TileOperations::update(tile);
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

	Item* transformItem(Item* old_item, uint16_t new_id, Tile* parent) {
		if (old_item == nullptr) {
			return nullptr;
		}

		const uint16_t old_id = old_item->getID();
		old_item->setID(new_id);
		// Through the magic of deepCopy, this will now be a pointer to an item of the correct type.
		std::unique_ptr<Item> new_item_ptr = old_item->deepCopy();
		Item* new_item = new_item_ptr.get();

		if (parent) {
			// Find the old item and remove it from the tile, insert this one instead!
			if (old_item == parent->ground.get()) {
				parent->ground = std::move(new_item_ptr);
				TileOperations::update(parent);
				return new_item;
			}

			std::queue<Container*> containers;
			for (auto& item_ptr : parent->items) {
				if (item_ptr.get() == old_item) {
					item_ptr = std::move(new_item_ptr);
					TileOperations::update(parent);
					return new_item;
				}

				if (Container* c = item_ptr->asContainer()) {
					containers.push(c);
				}
			}

			while (!containers.empty()) {
				Container* container = containers.front();
				containers.pop();

				for (auto& item_ptr : container->getVector()) {
					if (item_ptr.get() == old_item) {
						// Found it!
						item_ptr = std::move(new_item_ptr);
						TileOperations::update(parent);
						return new_item;
					}

					if (Container* c = item_ptr->asContainer()) {
						containers.push(c);
					}
				}
			}
		}

		// If we reached here, the item was not found in the parent or parent was null.
		// Restore the old ID to keep the object consistent.
		old_item->setID(old_id);
		return nullptr;
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

		tile->statflags |= StatFlags::Selected;
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

		tile->statflags &= ~StatFlags::Selected;
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
			tile->statflags |= StatFlags::Selected;
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
		tile->statflags &= StatFlags::Modified;

		if (tile->spawn && tile->spawn->isSelected()) {
			tile->statflags |= StatFlags::Selected;
		}
		if (tile->creature && tile->creature->isSelected()) {
			tile->statflags |= StatFlags::Selected;
		}

		tile->minimapColor = 0; // Reset to "no color" (valid)

		if (tile->ground) {
			if (tile->ground->isSelected()) {
				tile->statflags |= StatFlags::Selected;
			}
			if (tile->ground->isBlocking()) {
				tile->statflags |= StatFlags::Blocking;
			}
			if (tile->ground->getUniqueID() != 0) {
				tile->statflags |= StatFlags::Unique;
			}
			if (tile->ground->getMiniMapColor() != 0) {
				tile->minimapColor = tile->ground->getMiniMapColor();
			}
			if (tile->ground->hasLight()) {
				tile->statflags |= StatFlags::HasLight;
			}
		} else {
			tile->mapflags = MapFlags::None;
			tile->house_id = 0;
		}

		std::ranges::for_each(tile->items, [&](const auto& i) {
			UpdateItemFlags(i.get(), tile->statflags, tile->minimapColor);
		});

		if (!testFlags(tile->statflags, StatFlags::Blocking)) {
			if (tile->ground == nullptr && tile->items.empty()) {
				tile->statflags |= StatFlags::Blocking;
			}
		}
	}

} // namespace TileOperations
