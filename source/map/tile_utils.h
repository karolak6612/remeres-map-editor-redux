//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_TILE_UTILS_H_
#define RME_TILE_UTILS_H_

#include <vector>
#include <memory>

class Tile;
class Item;

using ItemVector = std::vector<Item*>;

namespace TileUtils {
	void merge(Tile* dest, Tile* src);
	bool isContentEqual(const Tile* a, const Tile* b);
	std::vector<std::unique_ptr<Item>> popSelectedItems(Tile* tile, bool ignoreTileSelected = false);
	ItemVector getSelectedItems(Tile* tile, bool unzoomed = false);
}

#endif
