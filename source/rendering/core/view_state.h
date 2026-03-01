#ifndef RME_RENDERING_CORE_VIEW_STATE_H_
#define RME_RENDERING_CORE_VIEW_STATE_H_

#include <glm/glm.hpp>
#include "map/position.h"
#include "app/definitions.h"

struct ViewBounds {
	int start_x;
	int start_y;
	int end_x;
	int end_y;
};

struct ViewState {
	float zoom = 1.0f;
	int tile_size = 32;
	int floor = 7;

	int start_z = 7;
	int end_z = 7;
	int superend_z = 0;

	int view_scroll_x = 0;
	int view_scroll_y = 0;
	int screensize_x = 0;
	int screensize_y = 0;
	int viewport_x = 0;
	int viewport_y = 0;

	Position camera_pos;

	int mouse_map_x = 0;
	int mouse_map_y = 0;

	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);

	// Cached logical viewport dimensions for optimization
	float logical_width = 0.0f;
	float logical_height = 0.0f;

	// Base view bounds for the camera itself
	int camera_start_x = 0;
	int camera_start_y = 0;
	int camera_end_x = 0;
	int camera_end_y = 0;

	int getFloorAdjustment() const;
	bool IsTileVisible(int map_x, int map_y, int map_z, int& out_x, int& out_y) const;
	bool IsPixelVisible(int draw_x, int draw_y, int margin = PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS) const;
	// Checks if a rectangle (e.g. a node) is visible
	bool IsRectVisible(int draw_x, int draw_y, int width, int height, int margin = PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS) const;
	// Checks if a rectangle is fully inside the viewport (no clipping needed)
	bool IsRectFullyInside(int draw_x, int draw_y, int width, int height) const;
	void getScreenPosition(int map_x, int map_y, int map_z, int& out_x, int& out_y) const;
};

#endif
