//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "map/tile_operations.h"
#include "app/main.h"

// glut include removed

#include "rendering/drawers/cursors/drag_shadow_drawer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/map_access.h"
#include "rendering/core/render_view.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/frame_options.h"
#include "editor/selection.h"
#include "map/map.h"
#include "map/tile.h"
#include "game/sprites.h"

#include "game/item.h"
#include "game/creature.h"
#include "game/spawn.h"

DragShadowDrawer::DragShadowDrawer() {
}

DragShadowDrawer::~DragShadowDrawer() {
}

#include "rendering/core/primitive_renderer.h"

void DragShadowDrawer::draw(
	const DrawContext& ctx, IMapAccess& map_access, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer,
	const Position& drag_start
) {
	auto& sprite_batch = ctx.sprite_batch;
	const auto& view = ctx.view;
	const auto& settings = ctx.settings;
	const auto& frame = ctx.frame;

	// Draw dragging shadow
	if (!map_access.getSelection().isBusy() && frame.dragging && !settings.ingame) {
		for (auto tit = map_access.getSelection().begin(); tit != map_access.getSelection().end(); tit++) {
			Tile* tile = *tit;
			Position pos = tile->getPosition();

			int move_x, move_y, move_z;
			move_x = drag_start.x - view.mouse_map_x;
			move_y = drag_start.y - view.mouse_map_y;
			move_z = drag_start.z - view.floor;

			pos.x -= move_x;
			pos.y -= move_y;
			pos.z -= move_z;

			if (pos.z < 0 || pos.z >= MAP_LAYERS) {
				continue;
			}

			// On screen and dragging?
			if (pos.x + 2 > view.start_x && pos.x < view.end_x && pos.y + 2 > view.start_y && pos.y < view.end_y && (move_x != 0 || move_y != 0 || move_z != 0)) {
				int offset;
				if (pos.z <= GROUND_LAYER) {
					offset = (GROUND_LAYER - pos.z) * TILE_SIZE;
				} else {
					offset = TILE_SIZE * (view.floor - pos.z);
				}

				int draw_x = ((pos.x * TILE_SIZE) - view.view_scroll_x) - offset;
				int draw_y = ((pos.y * TILE_SIZE) - view.view_scroll_y) - offset;

				// save performance when moving large chunks unzoomed
				ItemVector toRender = TileOperations::getSelectedItems(tile, view.zoom > 3.0);
				Tile* desttile = map_access.getMap().getTile(pos);
				for (const auto& item : toRender) {
					if (desttile) {
						BlitItemParams params(desttile, item, settings, frame);
						params.ephemeral = true;
						params.red = 160;
						params.green = 160;
						params.blue = 160;
						params.alpha = 160;
						item_drawer->BlitItem(sprite_batch, ctx.atlas, sprite_drawer, creature_drawer, draw_x, draw_y, params);
					} else {
						BlitItemParams params(pos, item, settings, frame);
						params.ephemeral = true;
						params.red = 160;
						params.green = 160;
						params.blue = 160;
						params.alpha = 160;
						item_drawer->BlitItem(sprite_batch, ctx.atlas, sprite_drawer, creature_drawer, draw_x, draw_y, params);
					}
				}

				// save performance when moving large chunks unzoomed
				if (view.zoom <= 3.0) {
					if (tile->creature && tile->creature->isSelected() && settings.show_creatures) {
						creature_drawer->BlitCreature(sprite_batch, sprite_drawer, draw_x, draw_y, tile->creature.get(), CreatureDrawOptions { .color = DrawColor(160, 160, 160, 160) });
					}
					if (tile->spawn && tile->spawn->isSelected()) {
						sprite_drawer->BlitSprite(sprite_batch, draw_x, draw_y, SPRITE_SPAWN, DrawColor(160, 160, 160, 160));
					}
				}
			}
		}
	}
}
