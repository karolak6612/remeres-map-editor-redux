#include "app/main.h"

// glut include removed

#include "rendering/drawers/overlays/preview_drawer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/ui/map_display.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "ui/gui.h"
#include "brushes/brush.h"
#include "editor/copybuffer.h"
#include "map/map.h"

PreviewDrawer::PreviewDrawer() {
}

PreviewDrawer::~PreviewDrawer() {
}

void PreviewDrawer::draw(SpriteBatch& sprite_batch, PrimitiveRenderer& primitive_renderer, MapCanvas* canvas, const RenderView& view, int map_z, const DrawingOptions& options, Editor& editor, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, uint32_t current_house_id) {
	if (g_gui.secondary_map != nullptr && !options.ingame) {
		Brush* brush = g_gui.GetCurrentBrush();

		Position normalPos;
		Position to(view.mouse_map_x, view.mouse_map_y, view.floor);

		if (canvas->isPasting()) {
			normalPos = editor.copybuffer.getPosition();
		} else if (brush && brush->isDoodad()) {
			normalPos = Position(0x8000, 0x8000, 0x8);
		}

		// PROFILER: Iterate secondary map tiles instead of viewport pixels for performance
		MapIterator it = g_gui.secondary_map->begin();
		MapIterator end = g_gui.secondary_map->end();
		for (; it != end; ++it) {
			Tile* tile = (*it)->get();
			if (!tile) {
				continue;
			}

			Position pos = tile->getPosition();
			// Calculate where this tile would land on the screen
			// final = pos - normalPos + to
			// (derived from pos = normalPos + final - to)
			Position final = pos - normalPos + to;

			// Check if this tile belongs to the current layer being drawn
			if (final.z != map_z) {
				continue;
			}

			int map_x = final.x;
			int map_y = final.y;

			// Culling
			if (map_x < view.start_x || map_x > view.end_x || map_y < view.start_y || map_y > view.end_y) {
				continue;
			}

			{
				// Compensate for underground/overground
				int offset;
				if (map_z <= GROUND_LAYER) {
					offset = (GROUND_LAYER - map_z) * TileSize;
				} else {
					offset = TileSize * (view.floor - map_z);
				}

				int draw_x = ((map_x * TileSize) - view.view_scroll_x) - offset;
				int draw_y = ((map_y * TileSize) - view.view_scroll_y) - offset;

				// Draw ground
				uint8_t r = 160, g = 160, b = 160;
				if (tile->ground) {
					if (tile->isBlocking() && options.show_blocking) {
						g = g / 3 * 2;
						b = b / 3 * 2;
					}
					if (tile->isHouseTile() && options.show_houses) {
						if ((int)tile->getHouseID() == current_house_id) {
							r /= 2;
						} else {
							r /= 2;
							g /= 2;
						}
					} else if (options.show_special_tiles && tile->isPZ()) {
						r /= 2;
						b /= 2;
					}
					if (options.show_special_tiles && tile->getMapFlags() & TILESTATE_PVPZONE) {
						r = r / 3 * 2;
						b = r / 3 * 2;
					}
					if (options.show_special_tiles && tile->getMapFlags() & TILESTATE_NOLOGOUT) {
						b /= 2;
					}
					if (options.show_special_tiles && tile->getMapFlags() & TILESTATE_NOPVP) {
						g /= 2;
					}
					if (tile->ground) {
						item_drawer->BlitItem(sprite_batch, primitive_renderer, sprite_drawer, creature_drawer, draw_x, draw_y, tile, tile->ground, options, true, r, g, b, 160);
					}
				}

				// Draw items on the tile
				if (view.zoom <= 10.0 || !options.hide_items_when_zoomed) {
					ItemVector::iterator it;
					for (it = tile->items.begin(); it != tile->items.end(); it++) {
						if ((*it)->isBorder()) {
							item_drawer->BlitItem(sprite_batch, primitive_renderer, sprite_drawer, creature_drawer, draw_x, draw_y, tile, *it, options, true, 160, r, g, b);
						} else {
							item_drawer->BlitItem(sprite_batch, primitive_renderer, sprite_drawer, creature_drawer, draw_x, draw_y, tile, *it, options, true, 160, 160, 160, 160);
						}
					}
					if (tile->creature && options.show_creatures) {
						creature_drawer->BlitCreature(sprite_batch, sprite_drawer, draw_x, draw_y, tile->creature);
					}
				}
			}
		}
	}
}
