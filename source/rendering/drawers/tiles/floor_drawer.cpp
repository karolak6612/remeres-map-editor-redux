//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

// glut include removed

#include "rendering/drawers/tiles/floor_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/core/render_view.h"
#include "rendering/core/drawing_options.h"
#include "editor/editor.h"
#include "map/tile.h"
#include "map/map_region.h"
#include "game/items.h"

FloorDrawer::FloorDrawer() {
}

FloorDrawer::~FloorDrawer() {
}

void FloorDrawer::draw(SpriteBatch& sprite_batch, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, const RenderView& view, const DrawingOptions& options, Editor& editor) {

	// Draw "transparent higher floor"
	if (view.floor != 8 && view.floor != 0 && options.transparent_floors) {
		int map_z = view.floor - 1;

		int offset;
		if (map_z <= GROUND_LAYER) {
			offset = (GROUND_LAYER - map_z) * TileSize;
		} else {
			offset = TileSize * (view.floor - map_z);
		}

		// Pre-calculate base screen coordinates to avoid per-tile subtraction
		int base_screen_x = -view.view_scroll_x - offset;
		int base_screen_y = -view.view_scroll_y - offset;

		editor.map.visitLeaves(view.start_x, view.start_y, view.end_x + 1, view.end_y + 1, [&](MapNode* nd, int nd_map_x, int nd_map_y) {
			Floor* floor = nd->getFloor(map_z);
			if (!floor) {
				return;
			}

			for (int map_x = 0; map_x < 4; ++map_x) {
				for (int map_y = 0; map_y < 4; ++map_y) {
					TileLocation* loc = &floor->locs[map_x * 4 + map_y];
					Tile* tile = loc->get();
					if (!tile) {
						continue;
					}

					int global_x = nd_map_x + map_x;
					int global_y = nd_map_y + map_y;

					// Bounds check (visitLeaves might give nodes slightly outside if using viewport alignment)
					if (global_x < view.start_x || global_x > view.end_x || global_y < view.start_y || global_y > view.end_y) {
						continue;
					}

					int draw_x = (global_x * TileSize) + base_screen_x;
					int draw_y = (global_y * TileSize) + base_screen_y;

					if (tile->ground) {
						const ItemType& groundType = g_items[tile->ground->getID()];
						if (tile->isPZ()) {
							item_drawer->BlitItem(sprite_batch, sprite_drawer, creature_drawer, draw_x, draw_y, tile, tile->ground.get(), groundType, options, false, 128, 255, 128, 96);
						} else {
							item_drawer->BlitItem(sprite_batch, sprite_drawer, creature_drawer, draw_x, draw_y, tile, tile->ground.get(), groundType, options, false, 255, 255, 255, 96);
						}
					}
					if (view.zoom <= 10.0 || !options.hide_items_when_zoomed) {
						for (const auto& item : tile->items) {
							const ItemType& it = g_items[item->getID()];
							item_drawer->BlitItem(sprite_batch, sprite_drawer, creature_drawer, draw_x, draw_y, tile, item.get(), it, options, false, 255, 255, 255, 96);
						}
					}
				}
			}
		});
	}
}
