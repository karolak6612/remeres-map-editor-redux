#include "app/main.h"

#include "rendering/drawers/overlays/brush/wall_overlay_drawer.h"

#include "rendering/core/draw_context.h"
#include "rendering/core/render_view.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/drawers/overlays/brush_overlay_drawer.h"

void WallOverlayDrawer::draw(const DrawContext& ctx, const BrushOverlayContext& overlay, const glm::vec4& brush_color) {
	auto& sprite_batch = ctx.sprite_batch;
	const auto& view = ctx.view;
	const auto& atlas = ctx.atlas;

	const int start_map_x = view.mouse_map_x - overlay.brush_size;
	const int start_map_y = view.mouse_map_y - overlay.brush_size;
	const int end_map_x = view.mouse_map_x + overlay.brush_size + 1;
	const int end_map_y = view.mouse_map_y + overlay.brush_size + 1;
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
		const float h = (float)(end_sy - start_sy - 2 * TILE_SIZE);
		sprite_batch.drawRect((float)(end_sx - TILE_SIZE), (float)(start_sy + TILE_SIZE), (float)TILE_SIZE, h, brush_color, atlas);
	}
	if (delta_y > TILE_SIZE) {
		const float h = (float)(end_sy - start_sy - 2 * TILE_SIZE);
		sprite_batch.drawRect((float)start_sx, (float)(start_sy + TILE_SIZE), (float)TILE_SIZE, h, brush_color, atlas);
	}
}
