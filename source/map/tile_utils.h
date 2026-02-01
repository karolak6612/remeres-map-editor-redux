//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_TILE_UTILS_H_
#define RME_TILE_UTILS_H_

class Tile;
class BaseMap;
class WallBrush;

class TileUtils {
public:
	static void borderize(BaseMap* parent, Tile* tile);
	static void cleanBorders(Tile* tile);

	static void wallize(BaseMap* parent, Tile* tile);
	static void cleanWalls(Tile* tile, bool dontdelete = false);
	static void cleanWalls(Tile* tile, WallBrush* wb);

	static void tableize(BaseMap* parent, Tile* tile);
	static void cleanTables(Tile* tile, bool dontdelete = false);

	static void carpetize(BaseMap* parent, Tile* tile);
};

#endif
