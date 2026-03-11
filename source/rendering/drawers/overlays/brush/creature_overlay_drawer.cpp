#include "app/main.h"

#include "rendering/drawers/overlays/brush/creature_overlay_drawer.h"

#include "brushes/creature/creature_brush.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/map_access.h"
#include "rendering/core/render_view.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/overlays/brush_overlay_drawer.h"

void CreatureOverlayDrawer::draw(const DrawContext& ctx, const BrushOverlayContext& overlay) {
	auto& sprite_batch = ctx.sprite_batch;
	const auto& view = ctx.view;
	auto* creature_brush = overlay.current_brush->as<CreatureBrush>();
	const int cx = view.mouse_map_x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
	const int cy = view.mouse_map_y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();
	const auto color = creature_brush->canDraw(&overlay.map_access->getBaseMap(), Position(view.mouse_map_x, view.mouse_map_y, view.floor))
		? DrawColor(255, 255, 255, 160)
		: DrawColor(255, 64, 64, 160);

	overlay.creature_drawer->BlitCreature(
		sprite_batch, overlay.sprite_drawer, cx, cy, creature_brush->getType()->outfit, SOUTH, CreatureDrawOptions {.color = color}
	);
}
