//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/core/coordinate_mapper.h"
#include "app/definitions.h" // For MAP_MAX_LAYER, GROUND_LAYER, TILE_SIZE if defined there, or map.h

#include <cmath>

void CoordinateMapper::ScreenToMap(int screen_x, int screen_y, int view_start_x, int view_start_y, double zoom, int floor, double scale_factor, int* map_x, int* map_y) {
	screen_x = static_cast<int>(screen_x * scale_factor);
	screen_y = static_cast<int>(screen_y * scale_factor);

	if (screen_x < 0) {
		*map_x = (view_start_x + screen_x) / TILE_SIZE;
	} else {
		*map_x = static_cast<int>(view_start_x + (screen_x * zoom)) / TILE_SIZE;
	}

	if (screen_y < 0) {
		*map_y = (view_start_y + screen_y) / TILE_SIZE;
	} else {
		*map_y = static_cast<int>(view_start_y + (screen_y * zoom)) / TILE_SIZE;
	}

	if (floor <= GROUND_LAYER) {
		*map_x += GROUND_LAYER - floor;
		*map_y += GROUND_LAYER - floor;
	}
}

void CoordinateMapper::MapToScreen(int map_x, int map_y, int view_start_x, int view_start_y, double zoom, int floor, double scale_factor, int* screen_x, int* screen_y) {
	if (floor <= GROUND_LAYER) {
		map_x -= (GROUND_LAYER - floor);
		map_y -= (GROUND_LAYER - floor);
	}

	const double raw_x = static_cast<double>(map_x * TILE_SIZE - view_start_x);
	const double raw_y = static_cast<double>(map_y * TILE_SIZE - view_start_y);

	if (raw_x < 0.0) {
		*screen_x = static_cast<int>(std::ceil(raw_x / scale_factor));
	} else {
		*screen_x = static_cast<int>(std::ceil(raw_x / (zoom * scale_factor)));
	}

	if (raw_y < 0.0) {
		*screen_y = static_cast<int>(std::ceil(raw_y / scale_factor));
	} else {
		*screen_y = static_cast<int>(std::ceil(raw_y / (zoom * scale_factor)));
	}
}
