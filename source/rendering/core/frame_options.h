#ifndef RME_RENDERING_CORE_FRAME_OPTIONS_H_
#define RME_RENDERING_CORE_FRAME_OPTIONS_H_

#include <cstdint>
#include <optional>
#include "map/position.h"
#include "app/definitions.h"

// Per-frame transient state set each frame by MapCanvas/MapDrawer.
// Lifetime: single frame only.
struct FrameOptions {
	uint32_t current_house_id = 0;
	float highlight_pulse = 0.0f;
	bool dragging = false;
	bool boundbox_selection = false;
	DrawColor global_light_color { 128, 128, 128, 255 };
	std::optional<MapBounds> transient_selection_bounds;
};

#endif
