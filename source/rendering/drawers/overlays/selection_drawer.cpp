#include "app/main.h"

#include "rendering/drawers/overlays/selection_drawer.h"
#include "rendering/core/primitive_renderer.h"
#include <glm/glm.hpp>
#include "rendering/core/render_view.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/view_snapshot.h"
#include <algorithm>
#include <cmath>

void SelectionDrawer::draw(PrimitiveRenderer& primitive_renderer, const ViewState& view, const ViewSnapshot& snapshot, const RenderSettings& settings) {
	if (settings.ingame) {
		return;
	}

	// Draw bounding box
	// View coordinates (after zoom, relative to viewport)
	const float last_click_rx = static_cast<float>(snapshot.last_click_abs_x - view.view_scroll_x);
	const float last_click_ry = static_cast<float>(snapshot.last_click_abs_y - view.view_scroll_y);
	const float cursor_rx = static_cast<float>(snapshot.cursor_x * view.zoom);
	const float cursor_ry = static_cast<float>(snapshot.cursor_y * view.zoom);

	const float x = std::min(last_click_rx, cursor_rx);
	const float y = std::min(last_click_ry, cursor_ry);
	const float w = std::abs(cursor_rx - last_click_rx);
	const float h = std::abs(cursor_ry - last_click_ry);

	// Restore axis-aligned feedback by removing w/h requirement
	// w=0 or h=0 will just draw lines

	const glm::vec4 rect(x, y, w, h);

	// Draw semi-transparent fill
	primitive_renderer.drawRect(rect, glm::vec4(1.0f, 1.0f, 1.0f, 0.2f));

	// Draw white outline (physical 1px thickness)
	// thickness is in logical units, so thickness=view.zoom results in 1 physical pixel
	primitive_renderer.drawBox(rect, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), view.zoom);
}
