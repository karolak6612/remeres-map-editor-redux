#include "app/main.h"
#include "map/tile_operations.h"
#include "map/tile.h"
#include "game/item.h"
#include "brushes/ground/ground_brush.h"
#include "brushes/wall/wall_brush.h"
#include "brushes/carpet/carpet_brush.h"
#include "brushes/table/table_brush.h"

#include <algorithm>

namespace TileOperations {

void borderize(BaseMap* map, Tile* tile) {
	if (!tile) return;
	GroundBrush::doBorders(map, tile);
}

void cleanBorders(Tile* tile) {
	if (!tile) return;
	auto first_to_remove = std::stable_partition(tile->items.begin(), tile->items.end(), [](Item* item) {
		return !item->isBorder();
	});

	for (auto it = first_to_remove; it != tile->items.end(); ++it) {
		delete *it;
	}
	tile->items.erase(first_to_remove, tile->items.end());
}

void addBorderItem(Tile* tile, Item* item) {
	if (!tile || !item) return;
	ASSERT(item->isBorder());
	tile->items.insert(tile->items.begin(), item);
}

void wallize(BaseMap* map, Tile* tile) {
	if (!tile) return;
	WallBrush::doWalls(map, tile);
}

void cleanWalls(Tile* tile, bool dontdelete) {
	if (!tile) return;
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
	if (!tile || !wb) return;
	auto first_to_remove = std::stable_partition(tile->items.begin(), tile->items.end(), [wb](Item* item) {
		return !(item->isWall() && wb->hasWall(item));
	});

	for (auto it = first_to_remove; it != tile->items.end(); ++it) {
		delete *it;
	}
	tile->items.erase(first_to_remove, tile->items.end());
}

void addWallItem(Tile* tile, Item* item) {
	if (!tile || !item) return;
	ASSERT(item->isWall());
	tile->addItem(item);
}

void tableize(BaseMap* map, Tile* tile) {
	if (!tile) return;
	TableBrush::doTables(map, tile);
}

void cleanTables(Tile* tile, bool dontdelete) {
	if (!tile) return;
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

void carpetize(BaseMap* map, Tile* tile) {
	if (!tile) return;
	CarpetBrush::doCarpets(map, tile);
}

} // namespace TileOperations
