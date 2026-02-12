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

#include <bit>
#include <algorithm>

#include "map/map_region.h"
#include "map/basemap.h"
#include "map/position.h"
#include "map/tile.h"
#include "map/spatial_hash_grid.h"

//**************** Tile Location **********************

TileLocation::TileLocation() :
	position(0, 0, 0),
	spawn_count(0),
	waypoint_count(0),
	town_count(0) {
	////
}

TileLocation::~TileLocation() {
	// std::unique_ptr cleans up automatically
}

int TileLocation::size() const {
	if (tile) {
		return tile->size();
	}
	return spawn_count + waypoint_count + (house_exits ? 1 : 0);
}

bool TileLocation::empty() const {
	return size() == 0;
}

//**************** Floor **********************

Floor::Floor(int sx, int sy, int z) {
	sx = sx & ~3;
	sy = sy & ~3;

	for (int i = 0; i < MAP_LAYERS; ++i) {
		locs[i].position.x = sx + (i >> 2);
		locs[i].position.y = sy + (i & 3);
		locs[i].position.z = z;
	}
}

//**************** MapNode **********************

MapNode::MapNode(BaseMap& map) :
	map(map) {
	// std::array<std::unique_ptr> initializes to nullptr automatically
}

MapNode::~MapNode() {
	// std::unique_ptr cleans up automatically
}

Floor* MapNode::createFloor(int x, int y, int z) {
	if (!array[z]) {
		array[z] = std::make_unique<Floor>(x, y, z);
	}
	return array[z].get();
}

std::vector<SpatialHashGrid::SortedGridCell> SpatialHashGrid::getSortedCells() const {
	std::vector<SortedGridCell> sorted_cells;
	sorted_cells.reserve(cells.size());
	for (const auto& pair : cells) {
		sorted_cells.emplace_back(pair.first, pair.second.get());
	}
	std::sort(sorted_cells.begin(), sorted_cells.end(), [](const auto& a, const auto& b) {
		return a.key < b.key;
	});
	return sorted_cells;
}

bool MapNode::hasFloor(uint32_t z) {
	return array[z] != nullptr;
}

TileLocation* MapNode::getTile(int x, int y, int z) {
	Floor* f = array[z].get();
	if (!f) {
		return nullptr;
	}
	return &f->locs[(x & 3) * 4 + (y & 3)];
}

TileLocation* MapNode::createTile(int x, int y, int z) {
	Floor* f = createFloor(x, y, z);
	return &f->locs[(x & 3) * 4 + (y & 3)];
}

std::unique_ptr<Tile> MapNode::setTile(int x, int y, int z, std::unique_ptr<Tile> newtile) {
	Floor* f = createFloor(x, y, z);

	int offset_x = x & 3;
	int offset_y = y & 3;

	TileLocation* tmp = &f->locs[offset_x * 4 + offset_y];
	if (newtile) {
		newtile->setLocation(tmp);
	}
	std::unique_ptr<Tile> oldtile = std::move(tmp->tile);
	tmp->tile = std::move(newtile);

	if (tmp->tile && !oldtile) {
		++map.tilecount;
	} else if (oldtile && !tmp->tile) {
		--map.tilecount;
	}

	return oldtile;
}

void MapNode::clearTile(int x, int y, int z) {
	Floor* f = createFloor(x, y, z);

	int offset_x = x & 3;
	int offset_y = y & 3;

	TileLocation* tmp = &f->locs[offset_x * 4 + offset_y];
	tmp->tile = map.allocator(tmp);
}

//**************** SpatialHashGrid **********************

SpatialHashGrid::GridCell::GridCell() {
	// std::unique_ptr default constructor initializes to nullptr
}

SpatialHashGrid::GridCell::~GridCell() {
	// std::unique_ptr handles cleanup automatically
}

SpatialHashGrid::SpatialHashGrid(BaseMap& map) : map(map) {
	//
}

SpatialHashGrid::~SpatialHashGrid() {
	clear();
}

void SpatialHashGrid::clear() {
	cells.clear();
}

MapNode* SpatialHashGrid::getLeaf(int x, int y) {
	uint64_t key = makeKey(x, y);
	auto it = cells.find(key);
	if (it == cells.end()) {
		return nullptr;
	}

	int nx = (x >> NODE_SHIFT) & (NODES_PER_CELL - 1);
	int ny = (y >> NODE_SHIFT) & (NODES_PER_CELL - 1);
	return it->second->nodes[ny * NODES_PER_CELL + nx].get();
}

MapNode* SpatialHashGrid::getLeafForce(int x, int y) {
	uint64_t key = makeKey(x, y);
	auto& cell = cells[key];
	if (!cell) {
		cell = std::make_unique<GridCell>();
	}

	int nx = (x >> NODE_SHIFT) & (NODES_PER_CELL - 1);
	int ny = (y >> NODE_SHIFT) & (NODES_PER_CELL - 1);
	auto& node = cell->nodes[ny * NODES_PER_CELL + nx];
	if (!node) {
		node = std::make_unique<MapNode>(map);
	}
	return node.get();
}
