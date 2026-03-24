#include "app/main.h"
#include "rendering/core/render_view.h"

#include "rendering/core/render_settings.h"
#include "app/definitions.h" // For TILE_SIZE, GROUND_LAYER, MAP_MAX_LAYER
#include <algorithm> // For std::min
#include <glm/gtc/matrix_transform.hpp>

int ViewState::getFloorAdjustment() const {
	if (floor > GROUND_LAYER) { // Underground
		return 0; // No adjustment
	} else {
		return TILE_SIZE * (GROUND_LAYER - floor);
	}
}

std::optional<TileScreenPos> ViewState::IsTileVisible(int map_x, int map_y, int map_z) const {
	int offset = (map_z <= GROUND_LAYER)
		? (GROUND_LAYER - map_z) * TILE_SIZE
		: TILE_SIZE * (floor - map_z);
	int out_x = (map_x * TILE_SIZE) - view_scroll_x - offset;
	int out_y = (map_y * TILE_SIZE) - view_scroll_y - offset;
	const int margin = PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS;

	// Use cached logical dimensions
	if (out_x < -margin || out_x > logical_width + margin || out_y < -margin || out_y > logical_height + margin) {
		return std::nullopt;
	}
	return TileScreenPos { out_x, out_y };
}

bool ViewState::IsPixelVisible(int draw_x, int draw_y, int margin) const {
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
	int offset = (map_z <= GROUND_LAYER)
		? (GROUND_LAYER - map_z) * TILE_SIZE
		: TILE_SIZE * (floor - map_z);
	out_x = (map_x * TILE_SIZE) - view_scroll_x - offset;
	out_y = (map_y * TILE_SIZE) - view_scroll_y - offset;
}

void ViewProjection::Compute(ViewState& vs) {
	int width = vs.screensize_x;
	int height = vs.screensize_y;

	vs.projectionMatrix = glm::ortho(0.0f, width * vs.zoom, height * vs.zoom, 0.0f, -1.0f, 1.0f);
	vs.viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.375f, 0.375f, 0.0f));
}

// --- GL side effects, isolated from ViewState data ---

void GLViewport::Apply(const ViewState& view) {
	glViewport(view.viewport_x, view.viewport_y, view.screensize_x, view.screensize_y);
}

void GLViewport::Clear() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
