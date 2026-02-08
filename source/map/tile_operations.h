#ifndef RME_MAP_TILE_OPERATIONS_H_
#define RME_MAP_TILE_OPERATIONS_H_

class BaseMap;
class Tile;
class Item;
class WallBrush;

namespace TileOperations {

void borderize(BaseMap* map, Tile* tile);
void cleanBorders(Tile* tile);
void addBorderItem(Tile* tile, Item* item);

void wallize(BaseMap* map, Tile* tile);
void cleanWalls(Tile* tile, bool dontdelete = false);
void cleanWalls(Tile* tile, WallBrush* wb);
void addWallItem(Tile* tile, Item* item);

void tableize(BaseMap* map, Tile* tile);
void cleanTables(Tile* tile, bool dontdelete = false);

void carpetize(BaseMap* map, Tile* tile);

} // namespace TileOperations

#endif
