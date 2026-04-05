//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "brushes/ground/terrain_placement.h"

#include "game/item.h"
#include "map/tile.h"

namespace TerrainPlacement {

void placeBrushItem(Tile& tile, std::unique_ptr<Item> item) {
	if (!item) {
		return;
	}

	if (item->isGroundTile()) {
		tile.setGround(std::move(item));
		return;
	}

	const uint16_t border_base_ground_id = item->borderBaseGroundId();
	if (border_base_ground_id != 0) {
		tile.setGround(Item::Create(border_base_ground_id));
		return;
	}

	tile.addItem(std::move(item));
}

void placeBrushItem(Tile& tile, uint16_t item_id) {
	placeBrushItem(tile, Item::Create(item_id));
}

} // namespace TerrainPlacement
