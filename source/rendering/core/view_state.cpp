#include "app/main.h"
#include "rendering/core/view_state.h"

#include "app/definitions.h" // For TILE_SIZE, GROUND_LAYER

int ViewState::getFloorAdjustment() const {
	if (floor > GROUND_LAYER) { // Underground
		return 0; // No adjustment
	} else {
		return TILE_SIZE * (GROUND_LAYER - floor);
	}
}

int ViewState::CalculateLayerOffset(int map_z) const {
	if (map_z <= GROUND_LAYER) {
		return (GROUND_LAYER - map_z) * TILE_SIZE;
	} else {
		return TILE_SIZE * (floor - map_z);
	}
}

bool ViewState::IsTileVisible(int map_x, int map_y, int map_z, int& out_x, int& out_y) const {
	int offset = CalculateLayerOffset(map_z);

	out_x = ((map_x * TILE_SIZE) - view_scroll_x) - offset;
	out_y = ((map_y * TILE_SIZE) - view_scroll_y) - offset;
	const int margin = PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS;

	// Use cached logical dimensions
	if (out_x < -margin || out_x > logical_width + margin || out_y < -margin || out_y > logical_height + margin) {
		return false;
	}
	return true;
}

bool ViewState::IsPixelVisible(int draw_x, int draw_y, int margin) const {
	// Logic matches IsTileVisible but uses pre-calculated draw coordinates.
	// screensize_x * zoom gives the logical viewport size (since TILE_SIZE is constant 32).

	// Use cached logical dimensions
	if (draw_x + TILE_SIZE + margin < 0 || draw_x - margin > logical_width || draw_y + TILE_SIZE + margin < 0 || draw_y - margin > logical_height) {
		return false;
	}
	return true;
}

bool ViewState::IsRectVisible(int draw_x, int draw_y, int width, int height, int margin) const {
	if (draw_x + width + margin < 0 || draw_x - margin > logical_width || draw_y + height + margin < 0 || draw_y - margin > logical_height) {
		return false;
	}
	return true;
}

bool ViewState::IsRectFullyInside(int draw_x, int draw_y, int width, int height) const {
	return (draw_x >= 0 && draw_x + width <= logical_width && draw_y >= 0 && draw_y + height <= logical_height);
}

void ViewState::getScreenPosition(int map_x, int map_y, int map_z, int& out_x, int& out_y) const {
	int offset = CalculateLayerOffset(map_z);

	out_x = ((map_x * TILE_SIZE) - view_scroll_x) - offset;
	out_y = ((map_y * TILE_SIZE) - view_scroll_y) - offset;
}
