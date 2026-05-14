#ifndef RME_RENDERING_CORE_FLOOR_VISIBILITY_MODE_H_
#define RME_RENDERING_CORE_FLOOR_VISIBILITY_MODE_H_

#include "app/definitions.h"

#include <algorithm>

enum class FloorVisibilityMode : int {
	ClientVisible = 0,
	AllVisible = 1,
};

struct FloorVisibilityRange {
	int start_floor = GROUND_LAYER;
	int end_floor = GROUND_LAYER;
	bool draw_all_visited_floors = false;
};

[[nodiscard]] constexpr FloorVisibilityMode SanitizeFloorVisibilityMode(int value) noexcept;
[[nodiscard]] constexpr FloorVisibilityRange BuildFloorVisibilityRange(int current_floor, bool show_all_floors, FloorVisibilityMode mode) noexcept;

[[nodiscard]] constexpr FloorVisibilityMode SanitizeFloorVisibilityMode(int value) noexcept {
	switch (static_cast<FloorVisibilityMode>(value)) {
		case FloorVisibilityMode::AllVisible:
			return FloorVisibilityMode::AllVisible;
		case FloorVisibilityMode::ClientVisible:
		default:
			return FloorVisibilityMode::ClientVisible;
	}
}

[[nodiscard]] constexpr FloorVisibilityRange BuildFloorVisibilityRange(int current_floor, bool show_all_floors, FloorVisibilityMode mode) noexcept {
	const int floor = std::clamp(current_floor, 0, MAP_MAX_LAYER);
	if (!show_all_floors) {
		return {
			.start_floor = floor,
			.end_floor = floor,
			.draw_all_visited_floors = false,
		};
	}

	if (mode == FloorVisibilityMode::AllVisible) {
		return {
			.start_floor = MAP_MAX_LAYER,
			.end_floor = floor,
			.draw_all_visited_floors = true,
		};
	}

	if (floor <= GROUND_LAYER) {
		return {
			.start_floor = GROUND_LAYER,
			.end_floor = 0,
			.draw_all_visited_floors = false,
		};
	}

	return {
		.start_floor = std::min(MAP_MAX_LAYER, floor + 2),
		.end_floor = GROUND_LAYER + 1,
		.draw_all_visited_floors = false,
	};
}

#endif
