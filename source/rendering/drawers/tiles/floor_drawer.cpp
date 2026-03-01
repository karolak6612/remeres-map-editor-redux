//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

// glut include removed

#include "rendering/drawers/tiles/floor_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/floor_view_params.h"
#include "rendering/core/view_state.h"
#include "rendering/core/drawing_options.h"
#include "editor/editor.h"
#include "map/tile.h"

FloorDrawer::FloorDrawer() {
}

FloorDrawer::~FloorDrawer() {
}

void FloorDrawer::draw(const DrawContext& ctx, const FloorViewParams& floor_params, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, Editor& editor) {

	// Draw "transparent higher floor"
	if (ctx.view.floor != 8 && ctx.view.floor != 0 && ctx.options.transparent_floors) {
		int map_z = ctx.view.floor - 1;
		for (int map_x = floor_params.start_x; map_x <= floor_params.end_x; map_x++) {
			for (int map_y = floor_params.start_y; map_y <= floor_params.end_y; map_y++) {
				Tile* tile = editor.map.getTile(map_x, map_y, map_z);
				if (tile) {
					int offset;
					if (map_z <= GROUND_LAYER) {
						offset = (GROUND_LAYER - map_z) * TILE_SIZE;
					} else {
						offset = TILE_SIZE * (ctx.view.floor - map_z);
					}

					int draw_x = ((map_x * TILE_SIZE) - ctx.view.view_scroll_x) - offset;
					int draw_y = ((map_y * TILE_SIZE) - ctx.view.view_scroll_y) - offset;

					// Position pos = tile->getPosition();

					if (tile->ground) {
						BlitItemParams params(tile, tile->ground.get(), ctx.options);
						params.alpha = 96;
						if (tile->isPZ()) {
							params.red = 128;
							params.green = 255;
							params.blue = 128;
						}
						item_drawer->BlitItem(ctx, sprite_drawer, creature_drawer, draw_x, draw_y, params);
					}
					if (ctx.view.zoom <= 10.0 || !ctx.options.hide_items_when_zoomed) {
						for (const auto& item : tile->items) {
							BlitItemParams params(tile, item.get(), ctx.options);
							params.alpha = 96;
							item_drawer->BlitItem(ctx, sprite_drawer, creature_drawer, draw_x, draw_y, params);
						}
					}
				}
			}
		}
	}
}
