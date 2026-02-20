//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include <spdlog/spdlog.h>
#include <format>
#include <algorithm>

#include "rendering/core/graphics.h"
#include "editor/editor.h"
#include "map/map.h"

#include "ui/gui.h"
#include "rendering/ui/map_display.h"
#include "rendering/ui/minimap_window.h"

#include "rendering/drawers/minimap_drawer.h"

#ifndef NANOVG_GL3
	#define NANOVG_GL3
#endif
#include <nanovg.h>
#include <nanovg_gl.h>

// Helper to create attributes
static wxGLAttributes& GetCoreProfileAttributes() {
	static wxGLAttributes vAttrs = []() {
		wxGLAttributes a;
		a.PlatformDefaults().Defaults().RGBA().DoubleBuffer().Depth(24).Stencil(8).EndList();
		return a;
	}();
	return vAttrs;
}

MinimapWindow::MinimapWindow(wxWindow* parent) :
	wxGLCanvas(parent, GetCoreProfileAttributes(), wxID_ANY, wxDefaultPosition, wxSize(205, 130)),
	update_timer(this),
	context(nullptr),
	nvg(nullptr, NVGDeleter()) {
	spdlog::info("MinimapWindow::MinimapWindow - Creating context");
	context = std::make_unique<wxGLContext>(this);
	if (!context->IsOK()) {
		spdlog::error("MinimapWindow::MinimapWindow - Context creation failed");
	}
	SetToolTip("Click to move camera");
	drawer = std::make_unique<MinimapDrawer>();

	Bind(wxEVT_LEFT_DOWN, &MinimapWindow::OnMouseClick, this);
	Bind(wxEVT_SIZE, &MinimapWindow::OnSize, this);
	Bind(wxEVT_PAINT, &MinimapWindow::OnPaint, this);
	Bind(wxEVT_ERASE_BACKGROUND, &MinimapWindow::OnEraseBackground, this);
	Bind(wxEVT_CLOSE_WINDOW, &MinimapWindow::OnClose, this);
	Bind(wxEVT_TIMER, &MinimapWindow::OnDelayedUpdate, this, wxID_ANY);
	Bind(wxEVT_KEY_DOWN, &MinimapWindow::OnKey, this);
}

MinimapWindow::~MinimapWindow() {
	spdlog::debug("MinimapWindow destructor started");
	spdlog::default_logger()->flush();
	if (context) {
		spdlog::debug("MinimapWindow destructor - setting context and resetting drawer/nvg");
		spdlog::default_logger()->flush();
		SetCurrent(*context);
		drawer.reset();
		nvg.reset();
	}
	spdlog::debug("MinimapWindow destructor finished");
	spdlog::default_logger()->flush();
}

void MinimapWindow::OnSize(wxSizeEvent& event) {
	Refresh();
}

void MinimapWindow::OnClose(wxCloseEvent&) {
	spdlog::info("MinimapWindow::OnClose called");
	spdlog::default_logger()->flush();
	g_gui.DestroyMinimap();
}

void MinimapWindow::DelayedUpdate() {
	// We only updated the window AFTER actions have taken place, that
	// way we don't waste too much performance on updating this window
	update_timer.Start(g_settings.getInteger(Config::MINIMAP_UPDATE_DELAY), true);
}

void MinimapWindow::OnDelayedUpdate(wxTimerEvent& event) {
	Refresh();
}

void MinimapWindow::OnPaint(wxPaintEvent& event) {
	wxPaintDC dc(this); // validates the paint event

	// spdlog::info("MinimapWindow::OnPaint");

	if (!context) {
		spdlog::error("MinimapWindow::OnPaint - No context!");
		return;
	}

	SetCurrent(*context);

	static bool gladInitialized = false;
	if (!gladInitialized) {
		spdlog::info("MinimapWindow::OnPaint - Initializing GLAD");
		if (!gladLoadGL()) {
			spdlog::error("MinimapWindow::OnPaint - Failed to load GLAD");
		} else {
			spdlog::info("MinimapWindow::OnPaint - GLAD loaded. GL Version: {}", (char*)glGetString(GL_VERSION));
		}
		gladInitialized = true;
	}

	if (!nvg) {
		// Minimap uses a separate NanoVG context to avoid state interference with the main
		// TextRenderer, as the minimap window has its own GL context and lifecycle.
		nvg.reset(nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES));
	}

	if (!g_gui.IsEditorOpen()) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		SwapBuffers();
		return;
	}
	Editor& editor = *g_gui.GetCurrentEditor();
	MapCanvas* canvas = g_gui.GetCurrentMapTab()->GetCanvas();

	// Mock dc passed to Draw, unused by new GL implementation
	drawer->Draw(dc, GetSize(), editor, canvas);

	// Glass Overlay
	NVGcontext* vg = nvg.get();
	if (vg) {
		glClear(GL_STENCIL_BUFFER_BIT);
		int w, h;
		GetClientSize(&w, &h);
		nvgBeginFrame(vg, w, h, GetContentScaleFactor());

		// 1. Coordinates HUD
		Position pos = canvas->GetCursorPosition();
		if (pos.isValid()) {
			std::string coords = std::format("{}, {}, {}", pos.x, pos.y, pos.z);
			nvgFontSize(vg, 12.0f);
			nvgFontFace(vg, "sans");

			float bounds[4];
			nvgTextBounds(vg, 0, 0, coords.c_str(), nullptr, bounds);
			float textW = bounds[2] - bounds[0];
			float textH = bounds[3] - bounds[1];

			nvgBeginPath(vg);
			nvgRoundedRect(vg, 4.0f, 4.0f, textW + 12.0f, textH + 8.0f, 4.0f);
			nvgFillColor(vg, nvgRGBA(0, 0, 0, 150));
			nvgFill(vg);

			nvgFillColor(vg, nvgRGBA(255, 255, 255, 200));
			nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			nvgText(vg, 10.0f, 8.0f, coords.c_str(), nullptr);
		}

		// 2. View Frustum
		int view_scroll_x, view_scroll_y;
		int screensize_x, screensize_y;
		canvas->GetViewBox(&view_scroll_x, &view_scroll_y, &screensize_x, &screensize_y);
		int floor = g_gui.GetCurrentFloor();
		int floor_offset = (floor > GROUND_LAYER ? 0 : (GROUND_LAYER - floor));

		// Map coordinates of the view
		int view_map_x = view_scroll_x / TILE_SIZE + floor_offset;
		int view_map_y = view_scroll_y / TILE_SIZE + floor_offset;

		// Calculate visible tiles based on zoom
		// Note: We use max(1.0) to prevent division by zero, though Zoom should be > 0.
		double zoom = std::max(0.1, canvas->GetZoom());
		int view_w = static_cast<int>(screensize_x / (TILE_SIZE * zoom)) + 1;
		int view_h = static_cast<int>(screensize_y / (TILE_SIZE * zoom)) + 1;

		int last_start_x = drawer->GetLastStartX();
		int last_start_y = drawer->GetLastStartY();

		float vx = static_cast<float>(view_map_x - last_start_x);
		float vy = static_cast<float>(view_map_y - last_start_y);
		float vw = static_cast<float>(view_w);
		float vh = static_cast<float>(view_h);

		nvgBeginPath(vg);
		nvgRect(vg, vx, vy, vw, vh);
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 180));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);

		// 3. Vignette
		NVGpaint vignette = nvgRadialGradient(vg, w / 2.0f, h / 2.0f, std::min(w, h) / 3.0f, std::min(w, h) * 0.8f, nvgRGBA(0, 0, 0, 0), nvgRGBA(0, 0, 0, 100));
		nvgBeginPath(vg);
		nvgRect(vg, 0, 0, w, h);
		nvgFillPaint(vg, vignette);
		nvgFill(vg);

		// Subtle glass border
		nvgBeginPath(vg);
		nvgRoundedRect(vg, 1.5f, 1.5f, w - 3.0f, h - 3.0f, 4.0f);
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 60));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);

		nvgEndFrame(vg);
	}

	SwapBuffers();
}

void MinimapWindow::OnMouseClick(wxMouseEvent& event) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}
	int new_map_x, new_map_y;
	drawer->ScreenToMap(event.GetX(), event.GetY(), new_map_x, new_map_y);

	g_gui.SetScreenCenterPosition(Position(new_map_x, new_map_y, g_gui.GetCurrentFloor()));
	Refresh();
	g_gui.RefreshView();
}

void MinimapWindow::OnKey(wxKeyEvent& event) {
	if (g_gui.GetCurrentTab() != nullptr) {
		g_gui.GetCurrentMapTab()->GetEventHandler()->AddPendingEvent(event);
	}
}
