#include "app/main.h"

#include "app/visuals.h"
#include "editor/editor.h"
#include "game/item.h"
#include "game/sprites.h"
#include "map/tile.h"
#include "rendering/core/graphics.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/overlays/marker_drawer.h"
#include "rendering/drawers/overlays/visual_overlay_drawer.h"

namespace {
void drawMarkerVisual(SpriteBatch& sprite_batch, SpriteDrawer* drawer, VisualOverlayDrawer* overlay_drawer, int draw_x, int draw_y, MarkerVisualKind kind, uint32_t fallback_sprite_id, const wxColour& fallback_color, const Position& pos) {
	const VisualRule* rule = g_visuals.ResolveMarker(kind);
	if (!rule) {
		drawer->BlitSprite(sprite_batch, draw_x, draw_y, fallback_sprite_id, DrawColor(fallback_color.Red(), fallback_color.Green(), fallback_color.Blue(), fallback_color.Alpha()));
		return;
	}

	switch (rule->appearance.type) {
		case VisualAppearanceType::Rgba:
			drawer->glBlitSquare(sprite_batch, draw_x, draw_y, DrawColor(rule->appearance.color.Red(), rule->appearance.color.Green(), rule->appearance.color.Blue(), rule->appearance.color.Alpha()));
			return;
		case VisualAppearanceType::SpriteId:
			drawer->BlitSprite(sprite_batch, draw_x, draw_y, rule->appearance.sprite_id, DrawColor(rule->appearance.color.Red(), rule->appearance.color.Green(), rule->appearance.color.Blue(), rule->appearance.color.Alpha()));
			return;
		case VisualAppearanceType::OtherItemVisual:
			if (const auto definition = g_item_definitions.get(rule->appearance.item_id); definition) {
				drawer->BlitSprite(sprite_batch, draw_x, draw_y, definition.clientId(), DrawColor(rule->appearance.color.Red(), rule->appearance.color.Green(), rule->appearance.color.Blue(), rule->appearance.color.Alpha()));
				return;
			}
			break;
		case VisualAppearanceType::Png:
		case VisualAppearanceType::Svg:
			if (overlay_drawer && !rule->appearance.asset_path.empty()) {
				overlay_drawer->add(VisualOverlayRequest {
					.pos = pos,
					.asset_path = rule->appearance.asset_path,
					.color = rule->appearance.color,
					.placement = VisualOverlayPlacement::TileInset
				});
				return;
			}
			break;
	}

	drawer->BlitSprite(sprite_batch, draw_x, draw_y, fallback_sprite_id, DrawColor(fallback_color.Red(), fallback_color.Green(), fallback_color.Blue(), fallback_color.Alpha()));
}
}

MarkerDrawer::MarkerDrawer() {
}

MarkerDrawer::~MarkerDrawer() {
}

void MarkerDrawer::draw(SpriteBatch& sprite_batch, SpriteDrawer* drawer, int draw_x, int draw_y, const Tile* tile, Waypoint* waypoint, uint32_t current_house_id, Editor& editor, const DrawingOptions& options) {
	const Position pos = tile->getPosition();

	if (!options.ingame && waypoint && options.show_waypoints) {
		drawMarkerVisual(sprite_batch, drawer, visual_overlay_drawer, draw_x, draw_y, MarkerVisualKind::Waypoint, SPRITE_WAYPOINT, wxColour(64, 64, 255), pos);
	}

	if (tile->isHouseExit() && options.show_houses) {
		if (tile->hasHouseExit(current_house_id)) {
			drawMarkerVisual(sprite_batch, drawer, visual_overlay_drawer, draw_x, draw_y, MarkerVisualKind::HouseExitCurrent, SPRITE_HOUSE_EXIT, wxColour(64, 255, 255), pos);
		} else {
			drawMarkerVisual(sprite_batch, drawer, visual_overlay_drawer, draw_x, draw_y, MarkerVisualKind::HouseExitOther, SPRITE_HOUSE_EXIT, wxColour(64, 64, 255), pos);
		}
	}

	if (options.show_towns && tile->isTownExit(editor.map)) {
		drawMarkerVisual(sprite_batch, drawer, visual_overlay_drawer, draw_x, draw_y, MarkerVisualKind::TownTemple, SPRITE_TOWN_TEMPLE, wxColour(255, 255, 64, 170), pos);
	}

	if (tile->spawn && options.show_spawns) {
		if (tile->spawn->isSelected()) {
			drawMarkerVisual(sprite_batch, drawer, visual_overlay_drawer, draw_x, draw_y, MarkerVisualKind::SpawnSelected, SPRITE_SPAWN, wxColour(128, 128, 128), pos);
		} else {
			drawMarkerVisual(sprite_batch, drawer, visual_overlay_drawer, draw_x, draw_y, MarkerVisualKind::Spawn, SPRITE_SPAWN, wxColour(255, 255, 255), pos);
		}
	}
}
