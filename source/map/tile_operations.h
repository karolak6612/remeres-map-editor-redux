//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_TILE_OPERATIONS_H
#define RME_TILE_OPERATIONS_H

#include <memory>

class Tile;
class BaseMap;
class WallBrush;
class Item;

namespace TileOperations {
	// Absorb the other tile into this tile
	void merge(Tile* tile, Tile* other);

	// Compare the content (ground and items) of two tiles
	bool isContentEqual(const Tile* tile, const Tile* other);

	// Argument is a the map to allocate the tile from
	std::unique_ptr<Tile> deepCopy(const Tile* tile, BaseMap& map);

	void borderize(Tile* tile, BaseMap* map);
	void cleanBorders(Tile* tile);

	void wallize(Tile* tile, BaseMap* map);
	void cleanWalls(Tile* tile, bool dontdelete = false);
	void cleanWalls(Tile* tile, WallBrush* wb);

	void tableize(Tile* tile, BaseMap* map);
	void cleanTables(Tile* tile, bool dontdelete = false);

	void carpetize(Tile* tile, BaseMap* map);

	// Add a border item (added at the bottom of all items)
	void addBorderItem(Tile* tile, std::unique_ptr<Item> item);

	// Add a wall item (same as just addItem, but an additional check to verify that it is a wall)
	void addWallItem(Tile* tile, std::unique_ptr<Item> item);
}

#endif
