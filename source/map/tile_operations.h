//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_TILE_OPERATIONS_H
#define RME_TILE_OPERATIONS_H

#include "game/item.h"
#include <memory>
#include <vector>

class Tile;
class BaseMap;
class WallBrush;
class GroundBrush;

namespace TileOperations {
	void borderize(Tile* tile, BaseMap* map);
	void cleanBorders(Tile* tile);

	void wallize(Tile* tile, BaseMap* map);
	void cleanWalls(Tile* tile, bool dontdelete = false);
	void cleanWalls(Tile* tile, WallBrush* wb);

	void tableize(Tile* tile, BaseMap* map);
	void cleanTables(Tile* tile, bool dontdelete = false);

	void carpetize(Tile* tile, BaseMap* map);

	std::unique_ptr<Tile> deepCopy(const Tile* tile, BaseMap& map);
	void merge(Tile* dest, const Tile* src);
	bool isContentEqual(const Tile* a, const Tile* b);

	GroundBrush* getGroundBrush(const Tile* tile);
	Item* getWall(const Tile* tile);
	bool hasWall(const Tile* tile);
	void addWallItem(Tile* tile, std::unique_ptr<Item> item);

	Item* getCarpet(const Tile* tile);
	bool hasCarpet(const Tile* tile);
	Item* getTable(const Tile* tile);
	bool hasTable(const Tile* tile);

	void addBorderItem(Tile* tile, std::unique_ptr<Item> item);

	void select(Tile* tile);
	void deselect(Tile* tile);
	void selectGround(Tile* tile);
	void deselectGround(Tile* tile);

	Item* getTopSelectedItem(Tile* tile);
	std::vector<std::unique_ptr<Item>> popSelectedItems(Tile* tile, bool ignoreTileSelected = false);
	ItemVector getSelectedItems(Tile* tile, bool unzoomed = false);
}

#endif
