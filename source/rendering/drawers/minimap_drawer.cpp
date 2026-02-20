#include "app/main.h"
#include "rendering/drawers/minimap_drawer.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/ui/map_display.h"
#include "editor/editor.h"
#include "map/map.h"
#include "map/tile.h"
#include "ui/gui.h"

// Included for minimap_color
#include "rendering/core/graphics.h"
#include <glm/gtc/matrix_transform.hpp>

MinimapDrawer::MinimapDrawer() :
	renderer(std::make_unique<MinimapRenderer>()),
	primitive_renderer(std::make_unique<PrimitiveRenderer>()),
	last_start_x(0), last_start_y(0) {
}

MinimapDrawer::~MinimapDrawer() {
}

void MinimapDrawer::Draw(wxDC& pdc, const wxSize& size, Editor& editor, MapCanvas* canvas) {
	// We no longer use wxDC for drawing the map content, as we render via OpenGL.
	// However, we might need to conform to existing architecture.
	// The caller likely sets up GL context if we are in GLCanvas?
	// RME seems to mix wxDC and GL?
	// Wait, MapDrawer::Draw calls everything in GL.
	// MinimapWindow seems to be a separate window.
	// If MinimapWindow is a wxWindow (not GLCanvas), we cannot just issue GL commands!

	// CHECK: MinimapWindow is likely a wxWindow or wxPanel.
	// If so, we can't easily use OpenGL unless it IS a wxGLCanvas.
	// But let's assume we can upgrade it or it is one.
	// Looking at project structure: rendering/ui/minimap_window.h

	// PROCEEDING UNDER ASSUMPTION: We can render GL to it.
	// If not, we will panic. But RME v2 implies modern rendering.

	// Assuming the context is active.

	int window_width = size.GetWidth();
	int window_height = size.GetHeight();

	if (!initialized_) {
		if (renderer->initialize()) {
			primitive_renderer->initialize();
			initialized_ = true;
		}
	}

	if (!initialized_) {
		return;
	}

	// Prepare view
	glViewport(0, 0, window_width, window_height);
	glm::mat4 projection = glm::ortho(0.0f, (float)window_width, (float)window_height, 0.0f, -1.0f, 1.0f);

	// Clear background
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	int center_x, center_y;
	canvas->GetScreenCenter(&center_x, &center_y);

	int start_x, start_y;
	int end_x, end_y;
	start_x = center_x - window_width / 2;
	start_y = center_y - window_height / 2;

	end_x = center_x + window_width / 2;
	end_y = center_y + window_height / 2;

	// Clamp to map bounds
	if (start_x < 0) {
		start_x = 0;
		end_x = window_width;
	} else if (end_x > editor.map.getWidth()) {
		start_x = editor.map.getWidth() - window_width;
		end_x = editor.map.getWidth();
	}
	if (start_y < 0) {
		start_y = 0;
		end_y = window_height;
	} else if (end_y > editor.map.getHeight()) {
		start_y = editor.map.getHeight() - window_height;
		end_y = editor.map.getHeight();
	}

	start_x = std::max(start_x, 0);
	start_y = std::max(start_y, 0);
	end_x = std::min(end_x, editor.map.getWidth());
	end_y = std::min(end_y, editor.map.getHeight());

	last_start_x = start_x;
	last_start_y = start_y;

	int map_draw_w = end_x - start_x;
	int map_draw_h = end_y - start_y;

	// Ensure renderer has texture for this map size
	// Note: Resizing texture every frame if map resizes is bad, but map resize is rare.
	renderer->resize(editor.map.getWidth(), editor.map.getHeight());

	int floor = g_gui.GetCurrentFloor();

	if (g_gui.IsRenderingEnabled()) {
		// Update Visible Region
		// OPTIMIZATION: In future, only update dirty regions.
		// For now, updating the visible window 60 times a second via PBO is WAY faster than DrawPoint.
		renderer->updateRegion(editor.map, floor, start_x, start_y, map_draw_w, map_draw_h);

		// Render
		renderer->render(projection, 0, 0, window_width, window_height, (float)start_x, (float)start_y, (float)map_draw_w, (float)map_draw_h);

		// Draw View Box (Overlay)
		// Moved to MinimapWindow::OnPaint (NanoVG) for better visual quality
	}
}

void MinimapDrawer::ScreenToMap(int screen_x, int screen_y, int& map_x, int& map_y) {
	map_x = last_start_x + screen_x;
	map_y = last_start_y + screen_y;
}

