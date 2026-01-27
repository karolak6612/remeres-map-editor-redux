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
#include "map/operations/tile_operations.h"

#include "map/tile.h"
#include "map/basemap.h"
#include "game/item.h"

#include "brushes/wall_brush.h"
#include "brushes/ground_brush.h"
#include "brushes/table_brush.h"
#include "brushes/carpet_brush.h"

#include <algorithm>

namespace TileOperations {

void wallize(Tile* tile, BaseMap* map) {
	WallBrush::doWalls(map, tile);
}

void borderize(Tile* tile, BaseMap* map) {
	GroundBrush::doBorders(map, tile);
}

void tableize(Tile* tile, BaseMap* map) {
	TableBrush::doTables(map, tile);
}

void carpetize(Tile* tile, BaseMap* map) {
	CarpetBrush::doCarpets(map, tile);
}

void cleanWalls(Tile* tile, bool dontdelete) {
	auto& items = tile->items;
	auto first_to_remove = std::stable_partition(items.begin(), items.end(), [](Item* item) {
		return !item->isWall();
	});

	if (!dontdelete) {
		for (auto it = first_to_remove; it != items.end(); ++it) {
			delete *it;
		}
	}
	items.erase(first_to_remove, items.end());
}

void cleanWalls(Tile* tile, WallBrush* wb) {
	auto& items = tile->items;
	auto first_to_remove = std::stable_partition(items.begin(), items.end(), [wb](Item* item) {
		return !(item->isWall() && wb->hasWall(item));
	});

	for (auto it = first_to_remove; it != items.end(); ++it) {
		delete *it;
	}
	items.erase(first_to_remove, items.end());
}

void cleanBorders(Tile* tile) {
	auto& items = tile->items;
	auto first_to_remove = std::stable_partition(items.begin(), items.end(), [](Item* item) {
		return !item->isBorder();
	});

	for (auto it = first_to_remove; it != items.end(); ++it) {
		delete *it;
	}
	items.erase(first_to_remove, items.end());
}

void cleanTables(Tile* tile, bool dontdelete) {
	auto& items = tile->items;
	auto first_to_remove = std::stable_partition(items.begin(), items.end(), [](Item* item) {
		return !item->isTable();
	});

	if (!dontdelete) {
		for (auto it = first_to_remove; it != items.end(); ++it) {
			delete *it;
		}
	}
	items.erase(first_to_remove, items.end());
}

void addBorderItem(Tile* tile, Item* item) {
	if (!item) {
		return;
	}
	ASSERT(item->isBorder());
	tile->items.insert(tile->items.begin(), item);
}

void addWallItem(Tile* tile, Item* item) {
	if (!item) {
		return;
	}
	ASSERT(item->isWall());
	tile->addItem(item);
}

} // namespace TileOperations
