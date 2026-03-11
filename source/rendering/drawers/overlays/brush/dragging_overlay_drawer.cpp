#include "app/main.h"

#include "rendering/drawers/overlays/brush/dragging_overlay_drawer.h"

#include "brushes/border/optional_border_brush.h"
#include "brushes/raw/raw_brush.h"
#include "brushes/spawn/spawn_brush.h"
#include "brushes/wall/wall_brush.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/render_view.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/overlays/brush/brush_overlay_common.h"
#include "rendering/drawers/overlays/brush_overlay_drawer.h"

#include <algorithm>
#include <cmath>

void DraggingOverlayDrawer::draw(const DrawContext& ctx, const BrushOverlayContext& overlay, const glm::vec4& brush_color) {
	auto& sprite_batch = ctx.sprite_batch;
	const auto& view = ctx.view;
	const auto& atlas = ctx.atlas;
	const auto& visual = *overlay.visual;
	Brush* brush = overlay.current_brush;

	ASSERT(brush->canDrag());

	if (brush->is<WallBrush>()) {
		const int start_map_x = std::min(overlay.last_click_map_x, view.mouse_map_x);
		const int start_map_y = std::min(overlay.last_click_map_y, view.mouse_map_y);
		const int end_map_x = std::max(overlay.last_click_map_x, view.mouse_map_x) + 1;
		const int end_map_y = std::max(overlay.last_click_map_y, view.mouse_map_y) + 1;
		const int start_sx = start_map_x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
		const int start_sy = start_map_y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();
		const int end_sx = end_map_x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
		const int end_sy = end_map_y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();
		const int delta_x = end_sx - start_sx;
		const int delta_y = end_sy - start_sy;

		sprite_batch.drawRect((float)start_sx, (float)start_sy, (float)(end_sx - start_sx), (float)TILE_SIZE, brush_color, atlas);
		if (delta_y > TILE_SIZE) {
			sprite_batch.drawRect((float)start_sx, (float)(end_sy - TILE_SIZE), (float)(end_sx - start_sx), (float)TILE_SIZE, brush_color, atlas);
		}
		if (delta_x > TILE_SIZE && delta_y > TILE_SIZE) {
			const float h = (float)((end_sy - TILE_SIZE) - (start_sy + TILE_SIZE));
			sprite_batch.drawRect((float)(end_sx - TILE_SIZE), (float)(start_sy + TILE_SIZE), (float)TILE_SIZE, h, brush_color, atlas);
		}
		if (delta_y > TILE_SIZE) {
			const float h = (float)((end_sy - TILE_SIZE) - (start_sy + TILE_SIZE));
			sprite_batch.drawRect((float)start_sx, (float)(start_sy + TILE_SIZE), (float)TILE_SIZE, h, brush_color, atlas);
		}
		return;
	}

	if (overlay.brush_shape == BRUSHSHAPE_SQUARE || brush->is<SpawnBrush>()) {
		if (brush->is<RAWBrush>() || brush->is<OptionalBorderBrush>()) {
			const int start_x = std::min(overlay.last_click_map_x, view.mouse_map_x);
			const int end_x = std::max(overlay.last_click_map_x, view.mouse_map_x);
			const int start_y = std::min(overlay.last_click_map_y, view.mouse_map_y);
			const int end_y = std::max(overlay.last_click_map_y, view.mouse_map_y);
			auto* raw_brush = brush->is<RAWBrush>() ? brush->as<RAWBrush>() : nullptr;

			for (int y = start_y; y <= end_y; ++y) {
				const int cy = y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();
				for (int x = start_x; x <= end_x; ++x) {
					const int cx = x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
					if (brush->is<OptionalBorderBrush>()) {
						sprite_batch.drawRect(
							(float)cx, (float)cy, (float)TILE_SIZE, (float)TILE_SIZE,
							getOverlayCheckColor(brush, *overlay.map_access, Position(x, y, view.floor), visual), atlas
						);
					} else {
						overlay.item_drawer->DrawRawBrush(sprite_batch, overlay.sprite_drawer, cx, cy, raw_brush->getItemID(), 160, 160, 160, 160);
					}
				}
			}
			return;
		}

		const int start_map_x = std::min(overlay.last_click_map_x, view.mouse_map_x);
		const int start_map_y = std::min(overlay.last_click_map_y, view.mouse_map_y);
		const int end_map_x = std::max(overlay.last_click_map_x, view.mouse_map_x) + 1;
		const int end_map_y = std::max(overlay.last_click_map_y, view.mouse_map_y) + 1;
		const int start_sx = start_map_x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
		const int start_sy = start_map_y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();
		const int end_sx = end_map_x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
		const int end_sy = end_map_y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();
		const float w = (float)(end_sx - start_sx);
		const float h = (float)(end_sy - start_sy);

		if (visual.use_automagic && brush->needBorders()) {
			constexpr float thickness = 1.0f;
			sprite_batch.drawRect((float)start_sx, (float)start_sy, w, thickness, brush_color, atlas);
			sprite_batch.drawRect((float)start_sx, (float)(start_sy + h - thickness), w, thickness, brush_color, atlas);
			sprite_batch.drawRect((float)start_sx, (float)(start_sy + thickness), thickness, h - 2 * thickness, brush_color, atlas);
			sprite_batch.drawRect((float)(start_sx + w - thickness), (float)(start_sy + thickness), thickness, h - 2 * thickness, brush_color, atlas);
			return;
		}

		sprite_batch.drawRect((float)start_sx, (float)start_sy, w, h, brush_color, atlas);
		return;
	}

	const int width = std::max(std::abs(view.mouse_map_y - overlay.last_click_map_y), std::abs(view.mouse_map_x - overlay.last_click_map_x));
	const int start_x = view.mouse_map_x < overlay.last_click_map_x ? overlay.last_click_map_x - width : overlay.last_click_map_x;
	const int end_x = view.mouse_map_x < overlay.last_click_map_x ? overlay.last_click_map_x : overlay.last_click_map_x + width;
	const int start_y = view.mouse_map_y < overlay.last_click_map_y ? overlay.last_click_map_y - width : overlay.last_click_map_y;
	const int end_y = view.mouse_map_y < overlay.last_click_map_y ? overlay.last_click_map_y : overlay.last_click_map_y + width;
	const int center_x = start_x + (end_x - start_x) / 2;
	const int center_y = start_y + (end_y - start_y) / 2;
	const float radius = width / 2.0f + 0.005f;
	auto* raw_brush = brush->is<RAWBrush>() ? brush->as<RAWBrush>() : nullptr;

	for (int y = start_y - 1; y <= end_y + 1; ++y) {
		const int cy = y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();
		const float dy = (float)(center_y - y);
		for (int x = start_x - 1; x <= end_x + 1; ++x) {
			const float dx = (float)(center_x - x);
			if (std::sqrt(dx * dx + dy * dy) >= radius) {
				continue;
			}

			const int cx = x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
			if (brush->is<RAWBrush>()) {
				overlay.item_drawer->DrawRawBrush(sprite_batch, overlay.sprite_drawer, cx, cy, raw_brush->getItemID(), 160, 160, 160, 160);
			} else {
				sprite_batch.drawRect((float)cx, (float)cy, (float)TILE_SIZE, (float)TILE_SIZE, brush_color, atlas);
			}
		}
	}
}
