#include "app/main.h"

// glut include removed

#include "rendering/drawers/overlays/selection_drawer.h"
#include "rendering/core/primitive_renderer.h"
#include <glm/glm.hpp>
#include "rendering/core/render_view.h"
#include "rendering/core/drawing_options.h"
#include "rendering/ui/map_display.h"
#include "rendering/core/graphics.h"
#include "ui/gui.h"

void SelectionDrawer::draw(PrimitiveRenderer& primitive_renderer, const RenderView& view, const MapCanvas* canvas, const DrawingOptions& options) {
	if (options.ingame) {
		return;
	}

	// Draw bounding box
	// View coordinates (after zoom, relative to viewport)
	const float last_click_rx = static_cast<float>(canvas->last_click_abs_x - view.view_scroll_x);
	const float last_click_ry = static_cast<float>(canvas->last_click_abs_y - view.view_scroll_y);
	const float cursor_rx = static_cast<float>(canvas->cursor_x * view.zoom);
	const float cursor_ry = static_cast<float>(canvas->cursor_y * view.zoom);

	const float x = std::min(last_click_rx, cursor_rx);
	const float y = std::min(last_click_ry, cursor_ry);
	const float w = std::abs(cursor_rx - last_click_rx);
	const float h = std::abs(cursor_ry - last_click_ry);

	if (w <= 0.0f || h <= 0.0f) {
		return;
	}

	const glm::vec4 rect(x, y, w, h);

	// Draw semi-transparent fill
	primitive_renderer.drawRect(rect, glm::vec4(1.0f, 1.0f, 1.0f, 0.2f));

	// Draw white outline (1px thickness)
	primitive_renderer.drawBox(rect, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
}
