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

void Tile::setHouse(House* _house) {
	house_id = (_house ? _house->getID() : 0);
}

void Tile::setHouseID(uint32_t newHouseId) {
	house_id = newHouseId;
}

bool Tile::isTownExit(Map& map) const {
	return location->getTownCount() > 0;
}
