#ifndef RME_RENDERING_DRAWERS_OVERLAYS_BRUSH_BRUSH_OVERLAY_COMMON_H_
#define RME_RENDERING_DRAWERS_OVERLAYS_BRUSH_BRUSH_OVERLAY_COMMON_H_

#include "brushes/border/optional_border_brush.h"
#include "brushes/brush.h"
#include "brushes/waypoint/waypoint_brush.h"
#include "rendering/core/brush_visual_settings.h"
#include "rendering/core/map_access.h"
#include "map/position.h"

#include <glm/glm.hpp>

enum class OverlayBrushColor {
	Brush,
	House,
	Flag,
	Spawn,
	Eraser,
	Valid,
	Invalid,
	Blank,
};

[[nodiscard]] inline glm::vec4 getOverlayBrushColor(OverlayBrushColor color, const BrushVisualSettings& visual) {
	switch (color) {
		case OverlayBrushColor::Brush:
			return glm::vec4(
				visual.cursor_red / 255.0f, visual.cursor_green / 255.0f, visual.cursor_blue / 255.0f, visual.cursor_alpha / 255.0f
			);
		case OverlayBrushColor::Flag:
		case OverlayBrushColor::House:
			return glm::vec4(
				visual.cursor_alt_red / 255.0f, visual.cursor_alt_green / 255.0f, visual.cursor_alt_blue / 255.0f,
				visual.cursor_alt_alpha / 255.0f
			);
		case OverlayBrushColor::Spawn:
		case OverlayBrushColor::Eraser:
		case OverlayBrushColor::Invalid:
			return glm::vec4(166.0f / 255.0f, 0.0f, 0.0f, 128.0f / 255.0f);
		case OverlayBrushColor::Valid:
			return glm::vec4(0.0f, 166.0f / 255.0f, 0.0f, 128.0f / 255.0f);
		case OverlayBrushColor::Blank:
		default:
			return glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);
	}
}

[[nodiscard]] inline glm::vec4 getOverlayCheckColor(Brush* brush, IMapAccess& map_access, const Position& pos, const BrushVisualSettings& visual) {
	return brush->canDraw(&map_access.getBaseMap(), pos) ? getOverlayBrushColor(OverlayBrushColor::Valid, visual)
	                                                     : getOverlayBrushColor(OverlayBrushColor::Invalid, visual);
}

inline void getOverlayTileColor(Brush* brush, IMapAccess& map_access, const Position& position, uint8_t& r, uint8_t& g, uint8_t& b) {
	if (brush->canDraw(&map_access.getBaseMap(), position)) {
		if (brush->is<WaypointBrush>()) {
			r = 0x00;
			g = 0xff;
			b = 0x00;
		} else {
			r = 0x00;
			g = 0x00;
			b = 0xff;
		}
		return;
	}

	r = 0xff;
	g = 0x00;
	b = 0x00;
}

#endif
