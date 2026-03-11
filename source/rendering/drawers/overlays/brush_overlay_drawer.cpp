#include "app/main.h"

#include "rendering/drawers/overlays/brush_overlay_drawer.h"

#include "brushes/carpet/carpet_brush.h"
#include "brushes/creature/creature_brush.h"
#include "brushes/doodad/doodad_brush.h"
#include "brushes/door/door_brush.h"
#include "brushes/flag/flag_brush.h"
#include "brushes/house/house_brush.h"
#include "brushes/spawn/spawn_brush.h"
#include "brushes/table/table_brush.h"
#include "brushes/wall/wall_brush.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/render_settings.h"
#include "rendering/drawers/overlays/brush/brush_overlay_common.h"
#include "rendering/drawers/overlays/brush/creature_overlay_drawer.h"
#include "rendering/drawers/overlays/brush/door_overlay_drawer.h"
#include "rendering/drawers/overlays/brush/dragging_overlay_drawer.h"
#include "rendering/drawers/overlays/brush/generic_overlay_drawer.h"
#include "rendering/drawers/overlays/brush/wall_overlay_drawer.h"

BrushOverlayDrawer::BrushOverlayDrawer() :
	dragging_(std::make_unique<DraggingOverlayDrawer>()),
	wall_(std::make_unique<WallOverlayDrawer>()),
	door_(std::make_unique<DoorOverlayDrawer>()),
	creature_(std::make_unique<CreatureOverlayDrawer>()),
	generic_(std::make_unique<GenericOverlayDrawer>()) {
}

BrushOverlayDrawer::~BrushOverlayDrawer() = default;

void BrushOverlayDrawer::draw(const DrawContext& ctx, const BrushOverlayContext& overlay) {
	if (!overlay.is_drawing_mode || !overlay.current_brush || ctx.settings.ingame) {
		return;
	}

	OverlayBrushColor brush_color_type = OverlayBrushColor::Blank;
	Brush* brush = overlay.current_brush;
	if (brush->is<TerrainBrush>() || brush->is<TableBrush>() || brush->is<CarpetBrush>()) {
		brush_color_type = OverlayBrushColor::Brush;
	} else if (brush->is<HouseBrush>()) {
		brush_color_type = OverlayBrushColor::House;
	} else if (brush->is<FlagBrush>()) {
		brush_color_type = OverlayBrushColor::Flag;
	} else if (brush->is<SpawnBrush>()) {
		brush_color_type = OverlayBrushColor::Spawn;
	} else if (brush->is<EraserBrush>()) {
		brush_color_type = OverlayBrushColor::Eraser;
	}

	const glm::vec4 brush_color = getOverlayBrushColor(brush_color_type, *overlay.visual);
	if (overlay.is_dragging_draw) {
		dragging_->draw(ctx, overlay, brush_color);
		return;
	}

	if (brush->is<WallBrush>()) {
		wall_->draw(ctx, overlay, brush_color);
	} else if (brush->is<DoorBrush>()) {
		door_->draw(ctx, overlay);
	} else if (brush->is<CreatureBrush>()) {
		creature_->draw(ctx, overlay);
	} else if (!brush->is<DoodadBrush>()) {
		generic_->draw(ctx, overlay, brush_color);
	}
}
