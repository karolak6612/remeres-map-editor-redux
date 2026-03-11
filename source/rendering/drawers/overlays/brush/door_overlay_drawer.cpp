#include "app/main.h"

#include "rendering/drawers/overlays/brush/door_overlay_drawer.h"

#include "rendering/core/draw_context.h"
#include "rendering/core/render_view.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/drawers/overlays/brush/brush_overlay_common.h"
#include "rendering/drawers/overlays/brush_overlay_drawer.h"

void DoorOverlayDrawer::draw(const DrawContext& ctx, const BrushOverlayContext& overlay) {
	auto& sprite_batch = ctx.sprite_batch;
	const auto& view = ctx.view;
	const auto& visual = *overlay.visual;
	const int cx = view.mouse_map_x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
	const int cy = view.mouse_map_y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();

	sprite_batch.drawRect(
		(float)cx, (float)cy, (float)TILE_SIZE, (float)TILE_SIZE,
		getOverlayCheckColor(overlay.current_brush, *overlay.map_access, Position(view.mouse_map_x, view.mouse_map_y, view.floor), visual), ctx.atlas
	);
}
