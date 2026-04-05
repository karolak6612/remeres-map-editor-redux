//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_TERRAIN_PLACEMENT_H
#define RME_TERRAIN_PLACEMENT_H

#include <cstdint>
#include <memory>

class Item;
class Tile;

namespace TerrainPlacement {

// Terrain tools are the only subsystem allowed to interpret terrain base metadata.
void placeBrushItem(Tile& tile, std::unique_ptr<Item> item);
void placeBrushItem(Tile& tile, uint16_t item_id);

} // namespace TerrainPlacement

#endif
