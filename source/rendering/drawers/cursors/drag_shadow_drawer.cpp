//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "map/tile_operations.h"
#include "app/main.h"

// glut include removed

#include "rendering/drawers/cursors/drag_shadow_drawer.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/view_state.h"
#include "rendering/core/drawing_options.h"
#include "editor/editor.h"
#include "rendering/ui/map_display.h"
#include "map/tile.h"
#include "game/sprites.h"

#include "game/item.h"
#include "game/creature.h"
#include "game/spawn.h"
#include "rendering/map_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/ui/selection_controller.h"

DragShadowDrawer::DragShadowDrawer() {
}

DragShadowDrawer::~DragShadowDrawer() {
}

#include "rendering/core/primitive_renderer.h"

void DragShadowDrawer::draw(const DrawContext& ctx, MapDrawer* drawer, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer) {
	if (!drawer || !drawer->canvas) {
		return;
	}

	// Draw dragging shadow
	if (!drawer->editor.selection.isBusy() && ctx.options.dragging && !ctx.options.ingame) {
		for (auto tit = drawer->editor.selection.begin(); tit != drawer->editor.selection.end(); tit++) {
			Tile* tile = *tit;
			Position pos = tile->getPosition();

			int move_x, move_y, move_z;
			Position drag_start = drawer->canvas->selection_controller->GetDragStartPosition();
			move_x = drag_start.x - ctx.view.mouse_map_x;
			move_y = drag_start.y - ctx.view.mouse_map_y;
			move_z = drag_start.z - ctx.view.floor;

			pos.x -= move_x;
			pos.y -= move_y;
			pos.z -= move_z;

			if (pos.z < 0 || pos.z >= MAP_LAYERS) {
				continue;
			}

			// On screen and dragging?
			if (pos.x + 2 > ctx.view.camera_start_x && pos.x < ctx.view.camera_end_x && pos.y + 2 > ctx.view.camera_start_y && pos.y < ctx.view.camera_end_y && (move_x != 0 || move_y != 0 || move_z != 0)) {
				int draw_x, draw_y;
				ctx.view.getScreenPosition(pos.x, pos.y, pos.z, draw_x, draw_y);

				// save performance when moving large chunks unzoomed
				ItemVector toRender = TileOperations::getSelectedItems(tile, ctx.view.zoom > 3.0);
				Tile* desttile = drawer->editor.map.getTile(pos);
				for (const auto& item : toRender) {
					if (desttile) {
						BlitItemParams params(desttile, item, ctx.options);
						params.ephemeral = true;
						params.red = 160;
						params.green = 160;
						params.blue = 160;
						params.alpha = 160;
						item_drawer->BlitItem(ctx, sprite_drawer, creature_drawer, draw_x, draw_y, params);
					} else {
						BlitItemParams params(pos, item, ctx.options);
						params.ephemeral = true;
						params.red = 160;
						params.green = 160;
						params.blue = 160;
						params.alpha = 160;
						item_drawer->BlitItem(ctx, sprite_drawer, creature_drawer, draw_x, draw_y, params);
					}
				}

				// save performance when moving large chunks unzoomed
				if (ctx.view.zoom <= 3.0) {
					if (tile->creature && tile->creature->isSelected() && ctx.options.show_creatures) {
						creature_drawer->BlitCreature(ctx, sprite_drawer, draw_x, draw_y, tile->creature.get(), CreatureDrawOptions { .color = DrawColor(160, 160, 160, 160) });
					}
					if (tile->spawn && tile->spawn->isSelected()) {
						sprite_drawer->BlitSprite(ctx, draw_x, draw_y, SPRITE_SPAWN, DrawColor(160, 160, 160, 160));
					}
				}
			}
		}
	}
}
