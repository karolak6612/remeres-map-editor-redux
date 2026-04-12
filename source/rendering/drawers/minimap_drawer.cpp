#include "app/main.h"
#include "rendering/drawers/minimap_drawer.h"

#include "editor/editor.h"
#include "map/map.h"
#include "rendering/core/map_view_math.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/ui/map_display.h"
#include "ui/gui.h"
#include "ui/managers/minimap_manager.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace {

[[nodiscard]] double clampMapCoordinate(double value, int limit) {
	if (limit <= 0) {
		return 0.0;
	}
	return std::clamp(value, 0.0, static_cast<double>(limit - 1));
}

struct MinimapFloorRenderRange {
	int start_floor = GROUND_LAYER;
	int current_floor = GROUND_LAYER;
};

[[nodiscard]] MinimapFloorRenderRange getFloorRenderRange(const MinimapViewportState& viewport_state) {
	const int current_floor = MinimapViewport::ClampFloor(viewport_state.floor);
	if (!viewport_state.show_all_floors) {
		return {
			.start_floor = current_floor,
			.current_floor = current_floor,
		};
	}

	if (current_floor <= GROUND_LAYER) {
		return {
			.start_floor = GROUND_LAYER,
			.current_floor = current_floor,
		};
	}

	return {
		.start_floor = std::min(MAP_MAX_LAYER, current_floor + 2),
		.current_floor = current_floor,
	};
}

} // namespace

MinimapDrawer::MinimapDrawer() :
	renderer(std::make_unique<MinimapRenderer>()),
	primitive_renderer(std::make_unique<PrimitiveRenderer>()) {
}

MinimapDrawer::~MinimapDrawer() {
}

void MinimapDrawer::ReleaseGL() {
	if (renderer) {
		renderer->releaseGL();
	}
	primitive_renderer.reset();
	renderer.reset();
	initialized_ = false;
}

MinimapDrawer::VisibleWorldRect MinimapDrawer::BuildVisibleWorldRect(const wxSize& size, Editor& editor, const MinimapViewportState& viewport_state) {
	const double zoom_factor = MinimapViewport::GetZoomFactor(viewport_state.zoom_step);
	const double visible_map_width = std::max(1.0, size.GetWidth() * zoom_factor);
	const double visible_map_height = std::max(1.0, size.GetHeight() * zoom_factor);

	return {
		.start_x = viewport_state.center_x - visible_map_width / 2.0,
		.start_y = viewport_state.center_y - visible_map_height / 2.0,
		.width = visible_map_width,
		.height = visible_map_height,
	};
}

void MinimapDrawer::DrawMainCameraBox(const glm::mat4& projection, const wxSize& size, MapCanvas& canvas, const VisibleWorldRect& visible_rect) {
	if (!g_settings.getInteger(Config::MINIMAP_VIEW_BOX)) {
		return;
	}

	int view_scroll_x = 0;
	int view_scroll_y = 0;
	int screensize_x = 0;
	int screensize_y = 0;
	canvas.GetViewBox(&view_scroll_x, &view_scroll_y, &screensize_x, &screensize_y);

	const MainMapVisibleRect camera_rect = MainMapViewMath::GetVisibleRect({
		.view_scroll_x = view_scroll_x,
		.view_scroll_y = view_scroll_y,
		.pixel_width = screensize_x,
		.pixel_height = screensize_y,
		.zoom = canvas.GetZoom(),
		.floor = canvas.GetFloor(),
		.scale_factor = canvas.GetContentScaleFactor(),
	});

	const float scale_x = static_cast<float>(size.GetWidth() / visible_rect.width);
	const float scale_y = static_cast<float>(size.GetHeight() / visible_rect.height);
	const float x = static_cast<float>((camera_rect.start_x - visible_rect.start_x) * scale_x);
	const float y = static_cast<float>((camera_rect.start_y - visible_rect.start_y) * scale_y);
	const float w = static_cast<float>(camera_rect.width * scale_x);
	const float h = static_cast<float>(camera_rect.height * scale_y);

	primitive_renderer->setProjectionMatrix(projection);
	const glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
	primitive_renderer->drawLine(glm::vec2(x, y), glm::vec2(x + w, y), color);
	primitive_renderer->drawLine(glm::vec2(x, y + h), glm::vec2(x + w, y + h), color);
	primitive_renderer->drawLine(glm::vec2(x, y), glm::vec2(x, y + h), color);
	primitive_renderer->drawLine(glm::vec2(x + w, y), glm::vec2(x + w, y + h), color);
	primitive_renderer->flush();
}

void MinimapDrawer::DrawFloorShade(const glm::mat4& projection, const wxSize& size) {
	primitive_renderer->setProjectionMatrix(projection);
	primitive_renderer->drawRect(
		glm::vec4(0.0f, 0.0f, static_cast<float>(size.GetWidth()), static_cast<float>(size.GetHeight())),
		glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));
	primitive_renderer->flush();
}

void MinimapDrawer::Draw(const wxSize& size, Editor& editor, MapCanvas& canvas, const MinimapViewportState& viewport_state) {
	const int window_width = size.GetWidth();
	const int window_height = size.GetHeight();
	if (window_width <= 0 || window_height <= 0) {
		last_viewport_.valid = false;
		return;
	}

	if (!initialized_) {
		if (renderer->initialize()) {
			primitive_renderer->initialize();
			initialized_ = true;
		}
	}

	if (!initialized_) {
		last_viewport_.valid = false;
		return;
	}

	// gl API removed
	const glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(window_width), static_cast<float>(window_height), 0.0f, -1.0f, 1.0f);

	// gl API removed
	// gl API removed

	const VisibleWorldRect visible_rect = BuildVisibleWorldRect(size, editor, viewport_state);
	const double start_x = visible_rect.start_x;
	const double start_y = visible_rect.start_y;

	last_viewport_ = {
		.start_x = start_x,
		.start_y = start_y,
		.map_width = visible_rect.width,
		.map_height = visible_rect.height,
		.screen_width = window_width,
		.screen_height = window_height,
		.map_limit_x = editor.map.getWidth(),
		.map_limit_y = editor.map.getHeight(),
		.valid = true
	};

	renderer->bindMap(editor.map.getGeneration(), editor.map.getWidth(), editor.map.getHeight());

	const PendingMinimapInvalidation pending = g_minimap.TakePendingInvalidation(editor.map);
	if (pending.invalidate_all) {
		renderer->invalidateAll();
	}
	for (int floor = 0; floor < MAP_LAYERS; ++floor) {
		if (pending.floor_rects[floor].has_value()) {
			renderer->markDirty(floor, *pending.floor_rects[floor]);
		}
	}

	if (!g_gui.IsRenderingEnabled()) {
		return;
	}

	const auto floor_range = getFloorRenderRange(viewport_state);
	const MinimapDirtyRect visible_rect_pixels = {
		.x = static_cast<int>(std::floor(visible_rect.start_x)),
		.y = static_cast<int>(std::floor(visible_rect.start_y)),
		.width = std::max(1, static_cast<int>(std::ceil(visible_rect.start_x + visible_rect.width)) - static_cast<int>(std::floor(visible_rect.start_x))),
		.height = std::max(1, static_cast<int>(std::ceil(visible_rect.start_y + visible_rect.height)) - static_cast<int>(std::floor(visible_rect.start_y))),
	};

	for (int floor = floor_range.start_floor; floor > floor_range.current_floor; --floor) {
		renderer->flushVisible(editor.map, floor, visible_rect_pixels);
		renderer->renderVisible(projection, 0, 0, window_width, window_height, floor, visible_rect_pixels);
	}

	if (floor_range.start_floor > floor_range.current_floor) {
		DrawFloorShade(projection, size);
	}

	renderer->flushVisible(editor.map, floor_range.current_floor, visible_rect_pixels);
	renderer->renderVisible(projection, 0, 0, window_width, window_height, floor_range.current_floor, visible_rect_pixels);
	DrawMainCameraBox(projection, size, canvas, visible_rect);
}

void MinimapDrawer::ScreenToMap(int screen_x, int screen_y, int& map_x, int& map_y) {
	if (!last_viewport_.valid) {
		map_x = 0;
		map_y = 0;
		return;
	}

	const double normalized_x = std::clamp(screen_x / static_cast<double>(std::max(1, last_viewport_.screen_width)), 0.0, 1.0);
	const double normalized_y = std::clamp(screen_y / static_cast<double>(std::max(1, last_viewport_.screen_height)), 0.0, 1.0);

	const double raw_map_x = last_viewport_.start_x + normalized_x * last_viewport_.map_width;
	const double raw_map_y = last_viewport_.start_y + normalized_y * last_viewport_.map_height;

	map_x = static_cast<int>(std::floor(clampMapCoordinate(raw_map_x, last_viewport_.map_limit_x)));
	map_y = static_cast<int>(std::floor(clampMapCoordinate(raw_map_y, last_viewport_.map_limit_y)));
}
