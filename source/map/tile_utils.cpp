//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "map/tile_utils.h"

#include "brushes/carpet/carpet_brush.h"
#include "brushes/ground/ground_brush.h"
#include "brushes/table/table_brush.h"
#include "brushes/wall/wall_brush.h"
#include "map/tile.h"
#include <algorithm>

void TileUtils::borderize(BaseMap* parent, Tile* tile) {
	GroundBrush::doBorders(parent, tile);
}

void TileUtils::cleanBorders(Tile* tile) {
	auto& items = tile->items;
	auto first_to_remove = std::stable_partition(items.begin(), items.end(), [](Item* item) {
		return !item->isBorder();
	});

	for (auto it = first_to_remove; it != items.end(); ++it) {
		delete *it;
	}
	items.erase(first_to_remove, items.end());
}

void TileUtils::wallize(BaseMap* parent, Tile* tile) {
	WallBrush::doWalls(parent, tile);
}

void TileUtils::cleanWalls(Tile* tile, bool dontdelete) {
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

void TileUtils::cleanWalls(Tile* tile, WallBrush* wb) {
	auto& items = tile->items;
	auto first_to_remove = std::stable_partition(items.begin(), items.end(), [wb](Item* item) {
		return !(item->isWall() && wb->hasWall(item));
	});

	for (auto it = first_to_remove; it != items.end(); ++it) {
		delete *it;
	}
	items.erase(first_to_remove, items.end());
}

void TileUtils::tableize(BaseMap* parent, Tile* tile) {
	TableBrush::doTables(parent, tile);
}

void TileUtils::cleanTables(Tile* tile, bool dontdelete) {
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

void TileUtils::carpetize(BaseMap* parent, Tile* tile) {
	CarpetBrush::doCarpets(parent, tile);
}
