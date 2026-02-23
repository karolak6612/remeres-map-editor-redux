#ifndef RME_RENDERING_RENDER_VIEW_H_
#define RME_RENDERING_RENDER_VIEW_H_

class MapCanvas;
struct DrawingOptions;

#include <glm/glm.hpp>
#include "map/position.h"
#include "app/definitions.h"

struct RenderView {
	float zoom;
	int tile_size;
	int floor;

	int start_x, start_y, start_z;
	int end_x, end_y, end_z, superend_z;
	int view_scroll_x, view_scroll_y;
	int screensize_x, screensize_y;
	int viewport_x, viewport_y;

	Position camera_pos;

	int mouse_map_x, mouse_map_y;

	glm::mat4 projectionMatrix;
	glm::mat4 viewMatrix;

	void Setup(MapCanvas* canvas, const DrawingOptions& options);
	void SetupGL();
	void ReleaseGL();
	void Clear();

	int getFloorAdjustment() const;
	inline bool IsTileVisible(int map_x, int map_y, int map_z, int& out_x, int& out_y) const {
		int offset = (map_z <= GROUND_LAYER)
			? (GROUND_LAYER - map_z) * TILE_SIZE
			: TILE_SIZE * (floor - map_z);
		out_x = (map_x * TILE_SIZE) - view_scroll_x - offset;
		out_y = (map_y * TILE_SIZE) - view_scroll_y - offset;
		const int margin = PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS;

		if (out_x < -margin || out_x > logical_width + margin || out_y < -margin || out_y > logical_height + margin) {
			return false;
		}
		return true;
	}

	inline bool IsPixelVisible(int draw_x, int draw_y, int margin = PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS) const {
		if (draw_x + TILE_SIZE + margin < 0 || draw_x - margin > logical_width || draw_y + TILE_SIZE + margin < 0 || draw_y - margin > logical_height) {
			return false;
		}
		return true;
	}

	// Checks if a rectangle (e.g. a node) is visible
	inline bool IsRectVisible(int draw_x, int draw_y, int width, int height, int margin = PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS) const {
		if (draw_x + width + margin < 0 || draw_x - margin > logical_width || draw_y + height + margin < 0 || draw_y - margin > logical_height) {
			return false;
		}
		return true;
	}

	// Checks if a rectangle is fully inside the viewport (no clipping needed)
	inline bool IsRectFullyInside(int draw_x, int draw_y, int width, int height) const {
		return (draw_x >= 0 && draw_x + width <= logical_width && draw_y >= 0 && draw_y + height <= logical_height);
	}

	void getScreenPosition(int map_x, int map_y, int map_z, int& out_x, int& out_y) const;

	// Cached logical viewport dimensions for optimization
	float logical_width = 0.0f;
	float logical_height = 0.0f;
};

#endif
