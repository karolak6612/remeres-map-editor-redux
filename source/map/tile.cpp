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
#include "map/tile_operations.h"
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
#include <numeric>

Tile::Tile(int x, int y, int z) :
	location(nullptr),
	position(x, y, z),
	ground(nullptr),
	house_id(0),
	mapflags(0),
	statflags(0),
	minimapColor(INVALID_MINIMAP_COLOR) {
	////
}

Tile::Tile(TileLocation& loc) :
	location(&loc),
	position(loc.getPosition()),
	ground(nullptr),
	house_id(0),
	mapflags(0),
	statflags(0),
	minimapColor(INVALID_MINIMAP_COLOR) {
	////
}

Position Tile::getPosition() {
	return position;
}

const Position Tile::getPosition() const {
	return position;
}

int Tile::getX() const {
	return position.x;
}

int Tile::getY() const {
	return position.y;
}

int Tile::getZ() const {
	return position.z;
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

uint32_t Tile::memsize() const {
	uint32_t mem = sizeof(*this);
	if (ground) {
		mem += ground->memsize();
	}

	for (const auto& item : items) {
		mem += item->memsize();
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
		TileOperations::update(this);
		return;
	}

	uint16_t gid = item->getGroundEquivalent();
	auto it = items.begin();

	if (gid != 0) {
		ground = Item::Create(gid);
		TileOperations::update(this);
		return;
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

	items.insert(it, std::move(item));
	TileOperations::update(this);
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

void Tile::setHouse(House* _house) {
	house_id = (_house ? _house->getID() : 0);
}

void Tile::setHouseID(uint32_t newHouseId) {
	house_id = newHouseId;
}

bool Tile::isTownExit(Map& map) const {
	return location->getTownCount() > 0;
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
	return std::ranges::equal(items, other->items, [](const std::unique_ptr<Item>& it1, const std::unique_ptr<Item>& it2) {
		return it1->getID() == it2->getID() && it1->getSubtype() == it2->getSubtype();
	});
}
