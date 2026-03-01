//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

// glut include removed

#include "rendering/drawers/overlays/brush_overlay_drawer.h"
#include "rendering/map_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/view_state.h"
#include "rendering/core/drawing_options.h"
#include "rendering/drawers/cursors/brush_cursor_drawer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/sprite_database.h"
#include "rendering/core/atlas_lifecycle.h"
#include "rendering/core/texture_gc.h"
#include "rendering/io/sprite_loader.h"
#include "editor/editor.h"
#include "ui/gui.h"
#include "game/sprites.h"
#include "app/settings.h"
#include "game/outfit.h"
#include "app/definitions.h"
#include "game/creatures.h"

#include "brushes/brush.h"

#include "rendering/ui/drawing_controller.h"
#include "brushes/doodad/doodad_brush.h"
#include "brushes/creature/creature_brush.h"
#include "brushes/house/house_exit_brush.h"
#include "brushes/house/house_brush.h"
#include "brushes/spawn/spawn_brush.h"
#include "brushes/wall/wall_brush.h"
#include "brushes/carpet/carpet_brush.h"
#include "brushes/raw/raw_brush.h"
#include "brushes/table/table_brush.h"
#include "brushes/waypoint/waypoint_brush.h"
#include "brushes/door/door_brush.h"
#include "brushes/flag/flag_brush.h"
#include "brushes/border/optional_border_brush.h"

#include "brushes/waypoint/waypoint_brush.h"

// Helper to get color from config
glm::vec4 BrushOverlayDrawer::get_brush_color(BrushColor color) {
	glm::vec4 c(1.0f);
	switch (color) {
		case COLOR_BRUSH:
			c = glm::vec4(
				g_settings.getInteger(Config::CURSOR_RED) / 255.0f,
				g_settings.getInteger(Config::CURSOR_GREEN) / 255.0f,
				g_settings.getInteger(Config::CURSOR_BLUE) / 255.0f,
				g_settings.getInteger(Config::CURSOR_ALPHA) / 255.0f
			);
			break;

		case COLOR_FLAG_BRUSH:
		case COLOR_HOUSE_BRUSH:
			c = glm::vec4(
				g_settings.getInteger(Config::CURSOR_ALT_RED) / 255.0f,
				g_settings.getInteger(Config::CURSOR_ALT_GREEN) / 255.0f,
				g_settings.getInteger(Config::CURSOR_ALT_BLUE) / 255.0f,
				g_settings.getInteger(Config::CURSOR_ALT_ALPHA) / 255.0f
			);
			break;

		case COLOR_SPAWN_BRUSH:
		case COLOR_ERASER:
		case COLOR_INVALID:
			c = glm::vec4(166.0f / 255.0f, 0.0f, 0.0f, 128.0f / 255.0f);
			break;

		case COLOR_VALID:
			c = glm::vec4(0.0f, 166.0f / 255.0f, 0.0f, 128.0f / 255.0f);
			break;

		default:
			c = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);
			break;
	}
	return c;
}

glm::vec4 BrushOverlayDrawer::get_check_color(Brush* brush, Editor& editor, const Position& pos) {
	if (brush->canDraw(&editor.map, pos)) {
		return get_brush_color(COLOR_VALID);
	} else {
		return get_brush_color(COLOR_INVALID);
	}
}

BrushOverlayDrawer::BrushOverlayDrawer() {
}

BrushOverlayDrawer::~BrushOverlayDrawer() {
}

void BrushOverlayDrawer::draw(const DrawContext& ctx, MapDrawer* drawer, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, Editor& editor) {
	if (!g_gui.IsDrawingMode()) {
		return;
	}
	if (!g_gui.GetCurrentBrush()) {
		return;
	}
	if (ctx.options.ingame) {
		return;
	}

	Brush* brush = g_gui.GetCurrentBrush();

	BrushColor brushColorType = COLOR_BLANK;
	if (brush->is<TerrainBrush>() || brush->is<TableBrush>() || brush->is<CarpetBrush>()) {
		brushColorType = COLOR_BRUSH;
	} else if (brush->is<HouseBrush>()) {
		brushColorType = COLOR_HOUSE_BRUSH;
	} else if (brush->is<FlagBrush>()) {
		brushColorType = COLOR_FLAG_BRUSH;
	} else if (brush->is<SpawnBrush>()) {
		brushColorType = COLOR_SPAWN_BRUSH;
	} else if (brush->is<EraserBrush>()) {
		brushColorType = COLOR_ERASER;
	}

	glm::vec4 brushColor = get_brush_color(brushColorType);

	if (!g_gui.atlas.ensureAtlasManager()) {
		return;
	}
	const AtlasManager& atlas = *g_gui.atlas.getAtlasManager();

	if (drawer->canvas.drawing_controller->IsDraggingDraw()) {
		ASSERT(brush->canDrag());

		if (brush->is<WallBrush>()) {
			int last_click_start_map_x = std::min(drawer->canvas.last_click_map_x, ctx.view.mouse_map_x);
			int last_click_start_map_y = std::min(drawer->canvas.last_click_map_y, ctx.view.mouse_map_y);
			int last_click_end_map_x = std::max(drawer->canvas.last_click_map_x, ctx.view.mouse_map_x) + 1;
			int last_click_end_map_y = std::max(drawer->canvas.last_click_map_y, ctx.view.mouse_map_y) + 1;

			int last_click_start_sx = last_click_start_map_x * TILE_SIZE - ctx.view.view_scroll_x - ctx.view.getFloorAdjustment();
			int last_click_start_sy = last_click_start_map_y * TILE_SIZE - ctx.view.view_scroll_y - ctx.view.getFloorAdjustment();
			int last_click_end_sx = last_click_end_map_x * TILE_SIZE - ctx.view.view_scroll_x - ctx.view.getFloorAdjustment();
			int last_click_end_sy = last_click_end_map_y * TILE_SIZE - ctx.view.view_scroll_y - ctx.view.getFloorAdjustment();

			int delta_x = last_click_end_sx - last_click_start_sx;
			int delta_y = last_click_end_sy - last_click_start_sy;

			// Top
			ctx.sprite_batch.drawRect(static_cast<float>(last_click_start_sx), static_cast<float>(last_click_start_sy), static_cast<float>(last_click_end_sx - last_click_start_sx), static_cast<float>(TILE_SIZE), brushColor, atlas);

			// Bottom
			if (delta_y > TILE_SIZE) {
				ctx.sprite_batch.drawRect(static_cast<float>(last_click_start_sx), static_cast<float>(last_click_end_sy - TILE_SIZE), static_cast<float>(last_click_end_sx - last_click_start_sx), static_cast<float>(TILE_SIZE), brushColor, atlas);
			}

			// Right
			if (delta_x > TILE_SIZE && delta_y > TILE_SIZE) {
				float h = (last_click_end_sy - TILE_SIZE) - (last_click_start_sy + TILE_SIZE);
				ctx.sprite_batch.drawRect(static_cast<float>(last_click_end_sx - TILE_SIZE), static_cast<float>(last_click_start_sy + TILE_SIZE), static_cast<float>(TILE_SIZE), h, brushColor, atlas);
			}

			// Left
			if (delta_y > TILE_SIZE) {
				float h = (last_click_end_sy - TILE_SIZE) - (last_click_start_sy + TILE_SIZE);
				ctx.sprite_batch.drawRect(static_cast<float>(last_click_start_sx), static_cast<float>(last_click_start_sy + TILE_SIZE), static_cast<float>(TILE_SIZE), h, brushColor, atlas);
			}
		} else {
			// if (brush->is<RAWBrush>()) { glEnable(GL_TEXTURE_2D); } -> handled by DrawRawBrush or BatchRenderer

			if (g_gui.GetBrushShape() == BRUSHSHAPE_SQUARE || brush->is<SpawnBrush>()) {
				if (brush->is<RAWBrush>() || brush->is<OptionalBorderBrush>()) {
					const int click_x = drawer->canvas.last_click_map_x;
					const int click_y = drawer->canvas.last_click_map_y;
					const int mouse_x = ctx.view.mouse_map_x;
					const int mouse_y = ctx.view.mouse_map_y;

					int start_x = std::min(click_x, mouse_x);
					int end_x = std::max(click_x, mouse_x);
					int start_y = std::min(click_y, mouse_y);
					int end_y = std::max(click_y, mouse_y);

					RAWBrush* raw_brush = nullptr;
					if (brush->is<RAWBrush>()) {
						raw_brush = brush->as<RAWBrush>();
					}

					for (int y = start_y; y <= end_y; y++) {
						int cy = y * TILE_SIZE - ctx.view.view_scroll_y - ctx.view.getFloorAdjustment();
						for (int x = start_x; x <= end_x; x++) {
							int cx = x * TILE_SIZE - ctx.view.view_scroll_x - ctx.view.getFloorAdjustment();
							if (brush->is<OptionalBorderBrush>()) {
								ctx.sprite_batch.drawRect(static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE), get_check_color(brush, editor, Position(x, y, ctx.view.floor)), atlas);
							} else if (brush->is<RAWBrush>()) {
								item_drawer->DrawRawBrush(ctx, sprite_drawer, cx, cy, raw_brush->getItemType(), 160, 160, 160, 160);
							} else {
								DrawColor color = DrawColor(128, 128, 255, 64);
								sprite_drawer->glDrawBox(ctx, cx, cy, TILE_SIZE, TILE_SIZE, color);
							}
						}
					}
				} else {
					int last_click_start_map_x = std::min(drawer->canvas.last_click_map_x, ctx.view.mouse_map_x);
					int last_click_start_map_y = std::min(drawer->canvas.last_click_map_y, ctx.view.mouse_map_y);
					int last_click_end_map_x = std::max(drawer->canvas.last_click_map_x, ctx.view.mouse_map_x) + 1;
					int last_click_end_map_y = std::max(drawer->canvas.last_click_map_y, ctx.view.mouse_map_y) + 1;

					int last_click_start_sx = last_click_start_map_x * TILE_SIZE - ctx.view.view_scroll_x - ctx.view.getFloorAdjustment();
					int last_click_start_sy = last_click_start_map_y * TILE_SIZE - ctx.view.view_scroll_y - ctx.view.getFloorAdjustment();
					int last_click_end_sx = last_click_end_map_x * TILE_SIZE - ctx.view.view_scroll_x - ctx.view.getFloorAdjustment();
					int last_click_end_sy = last_click_end_map_y * TILE_SIZE - ctx.view.view_scroll_y - ctx.view.getFloorAdjustment();

					float w = last_click_end_sx - last_click_start_sx;
					float h = last_click_end_sy - last_click_start_sy;
					bool autoborder_active = g_settings.getInteger(Config::USE_AUTOMAGIC) && brush->needBorders();
					if (autoborder_active) {
						// Draw outline only
						float thickness = 1.0f; // Thin border

						// Top
						ctx.sprite_batch.drawRect(static_cast<float>(last_click_start_sx), static_cast<float>(last_click_start_sy), w, thickness, brushColor, atlas);
						// Bottom
						ctx.sprite_batch.drawRect(static_cast<float>(last_click_start_sx), static_cast<float>(last_click_start_sy + h - thickness), w, thickness, brushColor, atlas);
						// Left
						ctx.sprite_batch.drawRect(static_cast<float>(last_click_start_sx), static_cast<float>(last_click_start_sy + thickness), thickness, h - 2 * thickness, brushColor, atlas);
						// Right
						ctx.sprite_batch.drawRect(static_cast<float>(last_click_start_sx + w - thickness), static_cast<float>(last_click_start_sy + thickness), thickness, h - 2 * thickness, brushColor, atlas);
					} else {
						ctx.sprite_batch.drawRect(static_cast<float>(last_click_start_sx), static_cast<float>(last_click_start_sy), w, h, brushColor, atlas);
					}
				}
			} else if (g_gui.GetBrushShape() == BRUSHSHAPE_CIRCLE) {
				// Calculate drawing offsets
				int start_x, end_x;
				int start_y, end_y;
				int width = std::max(
					std::abs(std::max(ctx.view.mouse_map_y, drawer->canvas.last_click_map_y) - std::min(ctx.view.mouse_map_y, drawer->canvas.last_click_map_y)),
					std::abs(std::max(ctx.view.mouse_map_x, drawer->canvas.last_click_map_x) - std::min(ctx.view.mouse_map_x, drawer->canvas.last_click_map_x))
				);

				if (ctx.view.mouse_map_x < drawer->canvas.last_click_map_x) {
					start_x = drawer->canvas.last_click_map_x - width;
					end_x = drawer->canvas.last_click_map_x;
				} else {
					start_x = drawer->canvas.last_click_map_x;
					end_x = drawer->canvas.last_click_map_x + width;
				}

				if (ctx.view.mouse_map_y < drawer->canvas.last_click_map_y) {
					start_y = drawer->canvas.last_click_map_y - width;
					end_y = drawer->canvas.last_click_map_y;
				} else {
					start_y = drawer->canvas.last_click_map_y;
					end_y = drawer->canvas.last_click_map_y + width;
				}

				int center_x = start_x + (end_x - start_x) / 2;
				int center_y = start_y + (end_y - start_y) / 2;
				float radii = width / 2.0f + 0.005f;

				RAWBrush* raw_brush = nullptr;
				if (brush->is<RAWBrush>()) {
					raw_brush = brush->as<RAWBrush>();
				}

				for (int y = start_y - 1; y <= end_y + 1; y++) {
					int cy = y * TILE_SIZE - ctx.view.view_scroll_y - ctx.view.getFloorAdjustment();
					float dy = center_y - y;
					for (int x = start_x - 1; x <= end_x + 1; x++) {
						int cx = x * TILE_SIZE - ctx.view.view_scroll_x - ctx.view.getFloorAdjustment();

						float dx = center_x - x;
						float distance = sqrt(dx * dx + dy * dy);
						if (distance < radii) {
							if (brush->is<RAWBrush>()) {
								item_drawer->DrawRawBrush(ctx, sprite_drawer, cx, cy, raw_brush->getItemType(), 160, 160, 160, 160);
							} else {
								ctx.sprite_batch.drawRect(static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE), brushColor, atlas);
							}
						}
					}
				}
			}

			// if (brush->is<RAWBrush>()) { glDisable(GL_TEXTURE_2D); }
		}
	} else {
		if (brush->is<WallBrush>()) {
			int start_map_x = ctx.view.mouse_map_x - g_gui.GetBrushSize();
			int start_map_y = ctx.view.mouse_map_y - g_gui.GetBrushSize();
			int end_map_x = ctx.view.mouse_map_x + g_gui.GetBrushSize() + 1;
			int end_map_y = ctx.view.mouse_map_y + g_gui.GetBrushSize() + 1;

			int start_sx = start_map_x * TILE_SIZE - ctx.view.view_scroll_x - ctx.view.getFloorAdjustment();
			int start_sy = start_map_y * TILE_SIZE - ctx.view.view_scroll_y - ctx.view.getFloorAdjustment();
			int end_sx = end_map_x * TILE_SIZE - ctx.view.view_scroll_x - ctx.view.getFloorAdjustment();
			int end_sy = end_map_y * TILE_SIZE - ctx.view.view_scroll_y - ctx.view.getFloorAdjustment();

			int delta_x = end_sx - start_sx;
			int delta_y = end_sy - start_sy;

			// Top
			ctx.sprite_batch.drawRect(static_cast<float>(start_sx), static_cast<float>(start_sy), static_cast<float>(end_sx - start_sx), static_cast<float>(TILE_SIZE), brushColor, atlas);

			// Bottom
			if (delta_y > TILE_SIZE) {
				ctx.sprite_batch.drawRect(static_cast<float>(start_sx), static_cast<float>(end_sy - TILE_SIZE), static_cast<float>(end_sx - start_sx), static_cast<float>(TILE_SIZE), brushColor, atlas);
			}

			// Right
			if (delta_x > TILE_SIZE && delta_y > TILE_SIZE) {
				float h = static_cast<float>(end_sy - start_sy - 2 * TILE_SIZE);
				ctx.sprite_batch.drawRect(static_cast<float>(end_sx - TILE_SIZE), static_cast<float>(start_sy + TILE_SIZE), static_cast<float>(TILE_SIZE), h, brushColor, atlas);
			}

			// Left
			if (delta_y > TILE_SIZE) {
				float h = static_cast<float>(end_sy - start_sy - 2 * TILE_SIZE);
				ctx.sprite_batch.drawRect(static_cast<float>(start_sx), static_cast<float>(start_sy + TILE_SIZE), static_cast<float>(TILE_SIZE), h, brushColor, atlas);
			}
		} else if (brush->is<DoorBrush>()) {
			int cx = (ctx.view.mouse_map_x) * TILE_SIZE - ctx.view.view_scroll_x - ctx.view.getFloorAdjustment();
			int cy = (ctx.view.mouse_map_y) * TILE_SIZE - ctx.view.view_scroll_y - ctx.view.getFloorAdjustment();

			ctx.sprite_batch.drawRect(static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE), get_check_color(brush, editor, Position(ctx.view.mouse_map_x, ctx.view.mouse_map_y, ctx.view.floor)), atlas);
		} else if (brush->is<CreatureBrush>()) {
			// glEnable(GL_TEXTURE_2D);
			int cy = (ctx.view.mouse_map_y) * TILE_SIZE - ctx.view.view_scroll_y - ctx.view.getFloorAdjustment();
			int cx = (ctx.view.mouse_map_x) * TILE_SIZE - ctx.view.view_scroll_x - ctx.view.getFloorAdjustment();
			CreatureBrush* creature_brush = brush->as<CreatureBrush>();
			if (creature_brush->canDraw(&editor.map, Position(ctx.view.mouse_map_x, ctx.view.mouse_map_y, ctx.view.floor))) {
				creature_drawer->BlitCreature(ctx, sprite_drawer, cx, cy, creature_brush->getType()->outfit, SOUTH, CreatureDrawOptions { .color = DrawColor(255, 255, 255, 160) });
			} else {
				DrawColor color = DrawColor(128, 128, 255, 64);
				sprite_drawer->glDrawBox(ctx, cx, cy, TILE_SIZE, TILE_SIZE, color);
			}
			// glDisable(GL_TEXTURE_2D);
		} else if (!brush->is<DoodadBrush>()) {
			RAWBrush* raw_brush = nullptr;
			if (brush->is<RAWBrush>()) { // Textured brush
				// glEnable(GL_TEXTURE_2D);
				raw_brush = brush->as<RAWBrush>();
			}

			for (int y = -g_gui.GetBrushSize() - 1; y <= g_gui.GetBrushSize() + 1; y++) {
				int cy = (ctx.view.mouse_map_y + y) * TILE_SIZE - ctx.view.view_scroll_y - ctx.view.getFloorAdjustment();
				for (int x = -g_gui.GetBrushSize() - 1; x <= g_gui.GetBrushSize() + 1; x++) {
					int cx = (ctx.view.mouse_map_x + x) * TILE_SIZE - ctx.view.view_scroll_x - ctx.view.getFloorAdjustment();
					if (g_gui.GetBrushShape() == BRUSHSHAPE_SQUARE) {
						if (x >= -g_gui.GetBrushSize() && x <= g_gui.GetBrushSize() && y >= -g_gui.GetBrushSize() && y <= g_gui.GetBrushSize()) {
							if (brush->is<RAWBrush>()) {
								item_drawer->DrawRawBrush(ctx, sprite_drawer, cx, cy, raw_brush->getItemType(), 160, 160, 160, 160);
							} else {
								if (brush->is<WaypointBrush>()) {
									uint8_t r, g, b;
									get_color(brush, editor, Position(ctx.view.mouse_map_x + x, ctx.view.mouse_map_y + y, ctx.view.floor), r, g, b);
									drawer->brush_cursor_drawer->draw(ctx, cx, cy, brush, r, g, b);
								} else {
									glm::vec4 c = brushColor;
									if (brush->is<HouseExitBrush>() || brush->is<OptionalBorderBrush>()) {
										c = get_check_color(brush, editor, Position(ctx.view.mouse_map_x + x, ctx.view.mouse_map_y + y, ctx.view.floor));
									}
									ctx.sprite_batch.drawRect(static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE), c, atlas);
								}
							}
						}
					} else if (g_gui.GetBrushShape() == BRUSHSHAPE_CIRCLE) {
						double distance = sqrt(double(x * x) + double(y * y));
						if (distance < g_gui.GetBrushSize() + 0.005) {
							if (brush->is<RAWBrush>()) {
								item_drawer->DrawRawBrush(ctx, sprite_drawer, cx, cy, raw_brush->getItemType(), 160, 160, 160, 160);
							} else {
								if (brush->is<WaypointBrush>()) {
									uint8_t r, g, b;
									get_color(brush, editor, Position(ctx.view.mouse_map_x + x, ctx.view.mouse_map_y + y, ctx.view.floor), r, g, b);
									drawer->brush_cursor_drawer->draw(ctx, cx, cy, brush, r, g, b);
								} else {
									glm::vec4 c = brushColor;
									if (brush->is<HouseExitBrush>() || brush->is<OptionalBorderBrush>()) {
										c = get_check_color(brush, editor, Position(ctx.view.mouse_map_x + x, ctx.view.mouse_map_y + y, ctx.view.floor));
									}
									ctx.sprite_batch.drawRect(static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE), c, atlas);
								}
							}
						}
					}
				}
			}

			// if (brush->is<RAWBrush>()) { // Textured brush
			// 	glDisable(GL_TEXTURE_2D);
			// }
		}
	}
}

void BrushOverlayDrawer::get_color(Brush* brush, Editor& editor, const Position& position, uint8_t& r, uint8_t& g, uint8_t& b) {
	if (brush->canDraw(&editor.map, position)) {
		if (brush->is<WaypointBrush>()) {
			r = 0x00;
			g = 0xff, b = 0x00;
		} else {
			r = 0x00;
			g = 0x00, b = 0xff;
		}
	} else {
		r = 0xff;
		g = 0x00, b = 0x00;
	}
}
