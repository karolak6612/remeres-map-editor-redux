//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "brushes/brush.h"

#include "map/tile.h"
#include "game/creature.h"
#include "game/house.h"
#include "map/basemap.h"
#include "map/map_region.h"
#include "game/spawn.h"
#include "brushes/ground/ground_brush.h"
#include "brushes/wall/wall_brush.h"
#include "brushes/carpet/carpet_brush.h"
#include "brushes/table/table_brush.h"
#include "game/town.h"
#include "map/map.h"

#include <ranges>
#include <algorithm>
#include <iterator>
#include <memory>

Tile::Tile(int x, int y, int z) :
	location(nullptr),
	ground(nullptr),
	house_id(0),
	mapflags(0),
	statflags(0),
	minimapColor(INVALID_MINIMAP_COLOR) {
	////
}

Tile::Tile(TileLocation& loc) :
	location(&loc),
	ground(nullptr),
	house_id(0),
	mapflags(0),
	statflags(0),
	minimapColor(INVALID_MINIMAP_COLOR) {
	////
}

Position Tile::getPosition() {
	return location->getPosition();
}

const Position Tile::getPosition() const {
	return location->getPosition();
}

int Tile::getX() const {
	return location->getPosition().x;
}

int Tile::getY() const {
	return location->getPosition().y;
}

int Tile::getZ() const {
	return location->getPosition().z;
}

HouseExitList* Tile::getHouseExits() {
	return location ? location->getHouseExits() : nullptr;
}

const HouseExitList* Tile::getHouseExits() const {
	return location ? location->getHouseExits() : nullptr;
}

bool Tile::isHouseExit() const {
	const HouseExitList* house_exits = getHouseExits();
	return house_exits && !house_exits->empty();
}

bool Tile::hasHouseExit(uint32_t exit) const {
	const HouseExitList* house_exits = getHouseExits();
	if (house_exits) {
		return std::ranges::any_of(*house_exits, [exit](uint32_t current_exit) { return current_exit == exit; });
	}
	return false;
}

Tile::~Tile() {
	// Smart pointers handle deletion
}

std::unique_ptr<Tile> Tile::deepCopy(BaseMap& map) {
	// Use the destination map's TileLocation at the same position
	TileLocation* dest_location = map.getTileL(getX(), getY(), getZ());
	if (!dest_location) {
		dest_location = map.createTileL(getX(), getY(), getZ());
	}
	std::unique_ptr<Tile> copy(map.allocator.allocateTile(dest_location));
	copy->mapflags = mapflags;
	copy->statflags = statflags;
	copy->minimapColor = minimapColor;
	copy->house_id = house_id;
	if (spawn) {
		copy->spawn = spawn->deepCopy();
	}
	if (creature) {
		copy->creature = creature->deepCopy();
	}
	// Spawncount & exits are not transferred on copy!
	if (ground) {
		copy->ground = ground->deepCopy();
	}

	copy->items.reserve(items.size());
	for (const auto& item : items) {
		copy->items.push_back(std::unique_ptr<Item>(item->deepCopy()));
	}

	return copy;
}

uint32_t Tile::memsize() const {
	uint32_t mem = sizeof(*this);
	if (ground) {
		mem += ground->memsize();
	}

	for (const auto& i : items) {
		mem += i->memsize();
	}

	mem += sizeof(std::unique_ptr<Item>) * items.capacity();

	return mem;
}

int Tile::size() const {
	int sz = 0;
	if (ground) {
		++sz;
	}
	sz += items.size();
	if (creature) {
		++sz;
	}
	if (spawn) {
		++sz;
	}
	if (house_id != 0) {
		++sz;
	}
	if (mapflags) {
		++sz;
	}
	if (location) {
		if (location->getHouseExits()) {
			++sz;
		}
		if (location->getSpawnCount()) {
			++sz;
		}
		if (location->getWaypointCount()) {
			++sz;
		}
	}
	return sz;
}

void Tile::merge(Tile* other) {
	if (other->isPZ()) {
		setPZ(true);
	}
	if (other->house_id) {
		house_id = other->house_id;
	}

	if (other->ground) {
		ground = std::move(other->ground);
	}

	if (other->creature) {
		creature = std::move(other->creature);
	}

	if (other->spawn) {
		spawn = std::move(other->spawn);
	}

	items.reserve(items.size() + other->items.size());
	for (auto& item : other->items) {
		addItem(std::move(item));
	}
	other->items.clear();
	update();
}

bool Tile::hasProperty(enum ITEMPROPERTY prop) const {
	if (prop == PROTECTIONZONE && isPZ()) {
		return true;
	}

	if (prop == BLOCKSOLID) {
		// Optimization: Use cached blocking state
		// Note: isBlocking() returns true for empty tiles (void), but hasProperty checks if *content* has property.
		return isBlocking() && (ground || !items.empty());
	}

	if (ground && ground->hasProperty(prop)) {
		return true;
	}

	return std::ranges::any_of(items, [prop](const auto& i) {
		return i->hasProperty(prop);
	});
}

int Tile::getIndexOf(Item* item) const {
	if (!item) {
		return wxNOT_FOUND;
	}

	int index = 0;
	if (ground) {
		if (ground.get() == item) {
			return index;
		}
		index++;
	}

	if (!items.empty()) {
		if (auto it = std::ranges::find_if(items, [item](const std::unique_ptr<Item>& i) { return i.get() == item; }); it != items.end()) {
			index += std::distance(items.begin(), it);
			return index;
		}
	}
	return wxNOT_FOUND;
}

Item* Tile::getTopItem() const {
	if (!items.empty() && !items.back()->isMetaItem()) {
		return items.back().get();
	}
	if (ground && !ground->isMetaItem()) {
		return ground.get();
	}
	return nullptr;
}

Item* Tile::getItemAt(int index) const {
	if (index < 0) {
		return nullptr;
	}
	if (ground) {
		if (index == 0) {
			return ground.get();
		}
		index--;
	}
	if (index >= 0 && index < (int)items.size()) {
		return items.at(index).get();
	}
	return nullptr;
}

void Tile::addItem(std::unique_ptr<Item> item) {
	if (!item) {
		return;
	}
	if (item->isGroundTile()) {
		ground = std::move(item);
		return;
	}

	uint16_t gid = item->getGroundEquivalent();
	auto it = items.begin();

	if (gid != 0) {
		ground = Item::Create(gid);
		// At the very bottom!
	} else if (item->isAlwaysOnBottom()) {
		// Find insertion point for always-on-bottom items
		// They are sorted by TopOrder, and come before normal items.
		it = std::ranges::find_if(items, [&](const std::unique_ptr<Item>& i) {
			if (!i->isAlwaysOnBottom()) {
				return true; // Found a normal item, insert before it
			}
			return item->getTopOrder() < i->getTopOrder(); // Found a bottom item with higher order
		});
	} else {
		it = items.end();
	}

	if (item->isSelected()) {
		statflags |= TILESTATE_SELECTED;
	}
	items.insert(it, std::move(item));
	update();
}

void Tile::select() {
	if (empty()) {
		return;
	}
	if (ground) {
		ground->select();
	}
	if (spawn) {
		spawn->select();
	}
	if (creature) {
		creature->select();
	}

	std::ranges::for_each(items, [](const auto& i) {
		i->select();
	});

	statflags |= TILESTATE_SELECTED;
}

void Tile::deselect() {
	if (ground) {
		ground->deselect();
	}
	if (spawn) {
		spawn->deselect();
	}
	if (creature) {
		creature->deselect();
	}

	std::ranges::for_each(items, [](const auto& i) {
		i->deselect();
	});

	statflags &= ~TILESTATE_SELECTED;
}

Item* Tile::getTopSelectedItem() {
	auto view = std::ranges::reverse_view(items);
	auto it = std::ranges::find_if(view, [](const std::unique_ptr<Item>& i) {
		return i->isSelected() && !i->isMetaItem();
	});

	if (it != view.end()) {
		return it->get();
	}

	if (ground && ground->isSelected() && !ground->isMetaItem()) {
		return ground.get();
	}
	return nullptr;
}

std::vector<std::unique_ptr<Item>> Tile::popSelectedItems(bool ignoreTileSelected) {
	std::vector<std::unique_ptr<Item>> pop_items;

	if (!ignoreTileSelected && !isSelected()) {
		return pop_items;
	}

	if (ground && ground->isSelected()) {
		pop_items.push_back(std::move(ground));
	}

	auto split_point = std::stable_partition(items.begin(), items.end(), [](const std::unique_ptr<Item>& i) { return !i->isSelected(); });
	std::move(split_point, items.end(), std::back_inserter(pop_items));
	items.erase(split_point, items.end());

	statflags &= ~TILESTATE_SELECTED;
	return pop_items;
}

ItemVector Tile::getSelectedItems(bool unzoomed) {
	ItemVector selected_items;

	if (!isSelected()) {
		return selected_items;
	}

	if (ground && ground->isSelected()) {
		selected_items.push_back(ground.get());
	}

	// save performance when zoomed out
	if (!unzoomed) {
		std::ranges::for_each(items, [&](const auto& item) {
			if (item->isSelected()) {
				selected_items.push_back(item.get());
			}
		});
	}

	return selected_items;
}

uint8_t Tile::getMiniMapColor() const {
	if (minimapColor != INVALID_MINIMAP_COLOR) {
		return minimapColor;
	}

	auto view = std::ranges::reverse_view(items);
	auto it = std::ranges::find_if(view, [](const std::unique_ptr<Item>& i) {
		return i->getMiniMapColor() != 0;
	});

	if (it != view.end()) {
		return it->get()->getMiniMapColor();
	}

	// check ground too
	if (hasGround()) {
		return ground->getMiniMapColor();
	}

	return 0;
}

bool tilePositionLessThan(const Tile* a, const Tile* b) {
	return a->getPosition() < b->getPosition();
}

bool tilePositionVisualLessThan(const Tile* a, const Tile* b) {
	Position pa = a->getPosition();
	Position pb = b->getPosition();

	if (pa.z > pb.z) {
		return true;
	}
	if (pa.z < pb.z) {
		return false;
	}

	if (pa.y < pb.y) {
		return true;
	}
	if (pa.y > pb.y) {
		return false;
	}

	if (pa.x < pb.x) {
		return true;
	}

	return false;
}

void Tile::update() {
	statflags &= TILESTATE_MODIFIED;

	if (spawn && spawn->isSelected()) {
		statflags |= TILESTATE_SELECTED;
	}
	if (creature && creature->isSelected()) {
		statflags |= TILESTATE_SELECTED;
	}

	minimapColor = 0; // Reset to "no color" (valid)

	if (ground) {
		if (ground->isSelected()) {
			statflags |= TILESTATE_SELECTED;
		}
		if (ground->isBlocking()) {
			statflags |= TILESTATE_BLOCKING;
		}
		if (ground->getUniqueID() != 0) {
			statflags |= TILESTATE_UNIQUE;
		}
		if (ground->getMiniMapColor() != 0) {
			minimapColor = ground->getMiniMapColor();
		}
	}

	std::ranges::for_each(items, [&](const auto& i) {
		if (i->isSelected()) {
			statflags |= TILESTATE_SELECTED;
		}
		if (i->getUniqueID() != 0) {
			statflags |= TILESTATE_UNIQUE;
		}
		if (i->getMiniMapColor() != 0) {
			minimapColor = i->getMiniMapColor();
		}

		ItemType& it_type = g_items[i->getID()];
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
	});

	if ((statflags & TILESTATE_BLOCKING) == 0) {
		if (ground == nullptr && items.empty()) {
			statflags |= TILESTATE_BLOCKING;
		}
	}
}

void Tile::addBorderItem(std::unique_ptr<Item> item) {
	if (!item) {
		return;
	}
	ASSERT(item->isBorder());
	items.insert(items.begin(), std::move(item));
	update();
}

GroundBrush* Tile::getGroundBrush() const {
	if (ground) {
		if (ground->getGroundBrush()) {
			return ground->getGroundBrush();
		}
	}
	return nullptr;
}

Item* Tile::getWall() const {
	auto it = std::ranges::find_if(items, [](const std::unique_ptr<Item>& i) {
		return i->isWall();
	});
	return (it != items.end()) ? it->get() : nullptr;
}

Item* Tile::getCarpet() const {
	auto it = std::ranges::find_if(items, [](const std::unique_ptr<Item>& i) {
		return i->isCarpet();
	});
	return (it != items.end()) ? it->get() : nullptr;
}

Item* Tile::getTable() const {
	auto it = std::ranges::find_if(items, [](const std::unique_ptr<Item>& i) {
		return i->isTable();
	});
	return (it != items.end()) ? it->get() : nullptr;
}

void Tile::addWallItem(std::unique_ptr<Item> item) {
	if (!item) {
		return;
	}
	ASSERT(item->isWall());

	addItem(std::move(item));
}

void Tile::selectGround() {
	bool selected_ = false;
	if (ground) {
		ground->select();
		selected_ = true;
	}

	for (const auto& i : items) {
		if (i->isBorder()) {
			i->select();
			selected_ = true;
		} else {
			break;
		}
	}

	if (selected_) {
		statflags |= TILESTATE_SELECTED;
	}
}

void Tile::deselectGround() {
	if (ground) {
		ground->deselect();
	}

	for (const auto& i : items) {
		if (i->isBorder()) {
			i->deselect();
		} else {
			break;
		}
	}
}

void Tile::setHouse(House* _house) {
	house_id = (_house ? _house->getID() : 0);
}

void Tile::setHouseID(uint32_t newHouseId) {
	house_id = newHouseId;
}

bool Tile::isTownExit(Map& map) const {
	return location->getTownCount() > 0;
}

void Tile::addHouseExit(House* h) {
	if (!h) {
		return;
	}
	HouseExitList* house_exits = location->createHouseExits();
	house_exits->push_back(h->getID());
}

void Tile::removeHouseExit(House* h) {
	if (!h) {
		return;
	}

	HouseExitList* house_exits = location->getHouseExits();
	if (!house_exits) {
		return;
	}

	std::erase(*house_exits, h->getID());
}

bool Tile::isContentEqual(const Tile* other) const {
	if (!other) {
		return false;
	}

	// Compare ground
	if (ground != nullptr && other->ground != nullptr) {
		if (ground->getID() != other->ground->getID() || ground->getSubtype() != other->ground->getSubtype()) {
			return false;
		}
	} else if (ground != other->ground) {
		return false;
	}

	// Compare items
	if (items.size() != other->items.size()) {
		return false;
	}

	return std::equal(items.begin(), items.end(), other->items.begin(), other->items.end(), [](const std::unique_ptr<Item>& it1, const std::unique_ptr<Item>& it2) {
		return it1->getID() == it2->getID() && it1->getSubtype() == it2->getSubtype();
	});
}
