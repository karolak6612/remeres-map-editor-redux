#include "app/main.h"
#include "rendering/core/view_state.h"
#include "rendering/core/visibility_utility.h"

#include "app/definitions.h" // For TILE_SIZE, GROUND_LAYER

int ViewState::getFloorAdjustment() const {
	if (floor > GROUND_LAYER) { // Underground
		return 0; // No adjustment
	} else {
		return TILE_SIZE * (GROUND_LAYER - floor);
	}
}

int ViewState::CalculateLayerOffset(int map_z) const {
	return rme::visibility::calculateLayerOffset(map_z, floor);
}

bool ViewState::IsTileVisible(int map_x, int map_y, int map_z, int& out_x, int& out_y) const {
	return rme::visibility::isTileVisible(map_x, map_y, map_z, view_scroll_x, view_scroll_y, logical_width, logical_height, floor, out_x, out_y);
}

bool ViewState::IsPixelVisible(int draw_x, int draw_y, int margin) const {
	return rme::visibility::isPixelVisible(draw_x, draw_y, logical_width, logical_height, margin);
}

bool ViewState::IsRectVisible(int draw_x, int draw_y, int width, int height, int margin) const {
	return rme::visibility::isRectVisible(draw_x, draw_y, width, height, logical_width, logical_height, margin);
}

bool ViewState::IsRectFullyInside(int draw_x, int draw_y, int width, int height) const {
	return rme::visibility::isRectFullyInside(draw_x, draw_y, width, height, logical_width, logical_height);
}

void ViewState::getScreenPosition(int map_x, int map_y, int map_z, int& out_x, int& out_y) const {
	rme::visibility::getScreenPosition(map_x, map_y, map_z, view_scroll_x, view_scroll_y, floor, out_x, out_y);
}
