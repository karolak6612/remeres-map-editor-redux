#include "app/main.h"

#include "rendering/drawers/overlays/brush/generic_overlay_drawer.h"

#include "brushes/border/optional_border_brush.h"
#include "brushes/house/house_exit_brush.h"
#include "brushes/raw/raw_brush.h"
#include "brushes/waypoint/waypoint_brush.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/render_view.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/drawers/cursors/brush_cursor_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/overlays/brush/brush_overlay_common.h"
#include "rendering/drawers/overlays/brush_overlay_drawer.h"

#include <cmath>

void GenericOverlayDrawer::draw(const DrawContext& ctx, const BrushOverlayContext& overlay, const glm::vec4& brush_color) {
	auto& sprite_batch = ctx.sprite_batch;
	auto& primitive_renderer = ctx.primitive_renderer;
	const auto& view = ctx.view;
	const auto& atlas = ctx.atlas;
	const auto& visual = *overlay.visual;
	auto& map_access = *overlay.map_access;
	Brush* brush = overlay.current_brush;
	auto* raw_brush = brush->is<RAWBrush>() ? brush->as<RAWBrush>() : nullptr;

	for (int y = -overlay.brush_size - 1; y <= overlay.brush_size + 1; ++y) {
		const int cy = (view.mouse_map_y + y) * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();
		for (int x = -overlay.brush_size - 1; x <= overlay.brush_size + 1; ++x) {
			const bool inside_square = x >= -overlay.brush_size && x <= overlay.brush_size && y >= -overlay.brush_size && y <= overlay.brush_size;
			const bool inside_circle = std::sqrt(double(x * x) + double(y * y)) < overlay.brush_size + 0.005;
			if ((overlay.brush_shape == BRUSHSHAPE_SQUARE && !inside_square) || (overlay.brush_shape == BRUSHSHAPE_CIRCLE && !inside_circle)) {
				continue;
			}

			const int cx = (view.mouse_map_x + x) * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
			const Position pos(view.mouse_map_x + x, view.mouse_map_y + y, view.floor);
			if (brush->is<RAWBrush>()) {
				overlay.item_drawer->DrawRawBrush(sprite_batch, overlay.sprite_drawer, cx, cy, raw_brush->getItemID(), 160, 160, 160, 160);
				continue;
			}

			if (brush->is<WaypointBrush>()) {
				uint8_t r = 0;
				uint8_t g = 0;
				uint8_t b = 0;
				getOverlayTileColor(brush, map_access, pos, r, g, b);
				overlay.brush_cursor_drawer->draw(sprite_batch, primitive_renderer, atlas, cx, cy, brush, r, g, b);
				continue;
			}

			glm::vec4 color = brush_color;
			if (brush->is<HouseExitBrush>() || brush->is<OptionalBorderBrush>()) {
				color = getOverlayCheckColor(brush, map_access, pos, visual);
			}
			sprite_batch.drawRect((float)cx, (float)cy, (float)TILE_SIZE, (float)TILE_SIZE, color, atlas);
		}
	}
}
