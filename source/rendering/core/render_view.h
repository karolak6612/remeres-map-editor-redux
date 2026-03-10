#ifndef RME_RENDERING_RENDER_VIEW_H_
#define RME_RENDERING_RENDER_VIEW_H_

#include <glm/glm.hpp>
#include "map/position.h"
#include "app/definitions.h"

struct ViewBounds {
	int start_x = 0;
	int start_y = 0;
	int end_x = 0;
	int end_y = 0;
};

// Per-floor coordinate bounds computed from the base ViewState.
// Each floor in the draw loop expands the visible area by one tile in each direction
// to account for the diagonal offset of stacked floors. This struct captures those
// per-floor bounds explicitly, replacing the old pattern of mutating the view mid-frame.
struct FloorViewParams {
	int start_x;
	int start_y;
	int end_x;
	int end_y;
};

// Pure data struct describing the viewport state for a single frame.
// Contains only computed values — no pointers to UI objects, no GL calls.
// Constructed from explicit parameters in MapDrawer::SetupVars().
struct ViewState {
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

	// Compute projection and view matrices from current dimensions.
	// Pure math (GLM) — no GL calls.
	void ComputeProjection();

	int getFloorAdjustment() const;
	bool IsTileVisible(int map_x, int map_y, int map_z, int& out_x, int& out_y) const;
	bool IsPixelVisible(int draw_x, int draw_y, int margin = PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS) const;
	bool IsRectVisible(int draw_x, int draw_y, int width, int height, int margin = PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS) const;
	bool IsRectFullyInside(int draw_x, int draw_y, int width, int height) const;
	void getScreenPosition(int map_x, int map_y, int map_z, int& out_x, int& out_y) const;

	// Cached logical viewport dimensions for optimization
	float logical_width = 0.0f;
	float logical_height = 0.0f;
};


// Isolated GL side effects — explicit, named, and separate from data.
namespace GLViewport {
	// Issues glViewport for the given view state.
	void Apply(const ViewState& view);

	// Clears the framebuffer (color + depth).
	void Clear();
}

#endif
