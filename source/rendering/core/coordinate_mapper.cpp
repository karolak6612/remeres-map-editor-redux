//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/core/coordinate_mapper.h"
#include "app/definitions.h" // For MAP_MAX_LAYER, GROUND_LAYER, TILE_SIZE if defined there, or map.h
#include "rendering/core/map_view_math.h"

#include <cmath>

void CoordinateMapper::ScreenToMap(int screen_x, int screen_y, int view_start_x, int view_start_y, double zoom, int floor, double scale_factor, int* map_x, int* map_y) {
	MainMapViewMath::ScreenToMap(
		screen_x,
		screen_y,
		MainMapViewport {
			.view_scroll_x = view_start_x,
			.view_scroll_y = view_start_y,
			.zoom = zoom,
			.floor = floor,
			.scale_factor = scale_factor,
		},
		map_x,
		map_y);
}

void CoordinateMapper::MapToScreen(int map_x, int map_y, int view_start_x, int view_start_y, double zoom, int floor, double scale_factor, int* screen_x, int* screen_y) {
	MainMapViewMath::MapToScreen(
		map_x,
		map_y,
		floor,
		MainMapViewport {
			.view_scroll_x = view_start_x,
			.view_scroll_y = view_start_y,
			.zoom = zoom,
			.floor = floor,
			.scale_factor = scale_factor,
		},
		screen_x,
		screen_y);
}
