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
#include "brushes/ground/ground_brush.h"
#include "brushes/wall/wall_brush.h"
#include "brushes/table/table_brush.h"
#include "brushes/carpet/carpet_brush.h"

#include <algorithm>
#include <vector>

namespace TileOperations {

	void borderize(Tile* tile, BaseMap* parent) {
		GroundBrush::doBorders(parent, tile);
	}

	void wallize(Tile* tile, BaseMap* parent) {
		WallBrush::doWalls(parent, tile);
	}

	void tableize(Tile* tile, BaseMap* parent) {
		TableBrush::doTables(parent, tile);
	}

	void carpetize(Tile* tile, BaseMap* parent) {
		CarpetBrush::doCarpets(parent, tile);
	}

	void cleanBorders(Tile* tile) {
		auto first_to_remove = std::stable_partition(tile->items.begin(), tile->items.end(), [](Item* item) {
			return !item->isBorder();
		});

		for (auto it = first_to_remove; it != tile->items.end(); ++it) {
			delete *it;
		}
		tile->items.erase(first_to_remove, tile->items.end());
	}

	void cleanWalls(Tile* tile, bool dontdelete) {
		auto first_to_remove = std::stable_partition(tile->items.begin(), tile->items.end(), [](Item* item) {
			return !item->isWall();
		});

		if (!dontdelete) {
			for (auto it = first_to_remove; it != tile->items.end(); ++it) {
				delete *it;
			}
		}
		tile->items.erase(first_to_remove, tile->items.end());
	}

	void cleanWalls(Tile* tile, WallBrush* wb) {
		auto first_to_remove = std::stable_partition(tile->items.begin(), tile->items.end(), [wb](Item* item) {
			return !(item->isWall() && wb->hasWall(item));
		});

		for (auto it = first_to_remove; it != tile->items.end(); ++it) {
			delete *it;
		}
		tile->items.erase(first_to_remove, tile->items.end());
	}

	void cleanTables(Tile* tile, bool dontdelete) {
		auto first_to_remove = std::stable_partition(tile->items.begin(), tile->items.end(), [](Item* item) {
			return !item->isTable();
		});

		if (!dontdelete) {
			for (auto it = first_to_remove; it != tile->items.end(); ++it) {
				delete *it;
			}
		}
		tile->items.erase(first_to_remove, tile->items.end());
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

}
