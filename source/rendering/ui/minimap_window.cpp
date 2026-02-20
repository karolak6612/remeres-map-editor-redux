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
	nvg(nullptr, NVGDeleter()),
	pan_timer(this) {
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
	Bind(wxEVT_TIMER, &MinimapWindow::OnDelayedUpdate, this, update_timer.GetId());
	Bind(wxEVT_TIMER, &MinimapWindow::OnPanTimer, this, pan_timer.GetId());
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

void MinimapWindow::OnPanTimer(wxTimerEvent& event) {
	if (!is_panning || !g_gui.IsEditorOpen()) {
		pan_timer.Stop();
		is_panning = false;
		return;
	}

	double speed = 0.2; // Smooth factor
	double dx = target_map_x - current_map_x;
	double dy = target_map_y - current_map_y;

	if (std::abs(dx) < 0.5 && std::abs(dy) < 0.5) {
		current_map_x = target_map_x;
		current_map_y = target_map_y;
		is_panning = false;
		pan_timer.Stop();
	} else {
		current_map_x += dx * speed;
		current_map_y += dy * speed;
	}

	g_gui.SetScreenCenterPosition(Position((int)current_map_x, (int)current_map_y, g_gui.GetCurrentFloor()));
	Refresh();
	// g_gui.RefreshView(); // Called by SetScreenCenterPosition
}

void MinimapWindow::OnPaint(wxPaintEvent& event) {
	wxPaintDC dc(this); // validates the paint event

	if (!context) {
		spdlog::error("MinimapWindow::OnPaint - No context!");
		return;
	}

	SetCurrent(*context);

	static bool gladInitialized = false;
	if (!gladInitialized) {
		if (!gladLoadGL()) {
			spdlog::error("MinimapWindow::OnPaint - Failed to load GLAD");
		}
		gladInitialized = true;
	}

	if (!nvg) {
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

	// Draw map content (OpenGL)
	drawer->Draw(dc, GetSize(), editor, canvas);

	// Glass Overlay (NanoVG)
	NVGcontext* vg = nvg.get();
	if (vg) {
		glClear(GL_STENCIL_BUFFER_BIT);
		int w, h;
		GetClientSize(&w, &h);
		nvgBeginFrame(vg, w, h, GetContentScaleFactor());

		// 1. Draw NanoVG View Box (cleaner lines + glow)
		if (g_settings.getInteger(Config::MINIMAP_VIEW_BOX)) {
			int screensize_x, screensize_y;
			int view_scroll_x, view_scroll_y;
			canvas->GetViewBox(&view_scroll_x, &view_scroll_y, &screensize_x, &screensize_y);

			int floor = g_gui.GetCurrentFloor();
			int floor_offset = (floor > GROUND_LAYER ? 0 : (GROUND_LAYER - floor));
			int view_start_x = view_scroll_x / TILE_SIZE + floor_offset;
			int view_start_y = view_scroll_y / TILE_SIZE + floor_offset;

			int tile_size = int(TILE_SIZE / canvas->GetZoom());
			int view_w = screensize_x / tile_size + 1;
			int view_h = screensize_y / tile_size + 1;

			int start_x = drawer->GetLastStartX();
			int start_y = drawer->GetLastStartY();

			float vx = (float)(view_start_x - start_x);
			float vy = (float)(view_start_y - start_y);
			float vw = (float)view_w;
			float vh = (float)view_h;

			// Glow effect
			NVGpaint glow = nvgBoxGradient(vg, vx, vy, vw, vh, 2.0f, 8.0f, nvgRGBA(255, 255, 255, 60), nvgRGBA(0, 0, 0, 0));
			nvgBeginPath(vg);
			nvgRect(vg, vx - 10, vy - 10, vw + 20, vh + 20);
			nvgRoundedRect(vg, vx, vy, vw, vh, 2.0f);
			nvgPathWinding(vg, NVG_HOLE);
			nvgFillPaint(vg, glow);
			nvgFill(vg);

			// Stroke
			nvgBeginPath(vg);
			nvgRoundedRect(vg, vx, vy, vw, vh, 2.0f);
			nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 200));
			nvgStrokeWidth(vg, 1.5f);
			nvgStroke(vg);
		}

		// 2. Subtle glass border
		nvgBeginPath(vg);
		nvgRoundedRect(vg, 1.5f, 1.5f, w - 3.0f, h - 3.0f, 4.0f);
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 60));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);

		// 3. Inner glow for "Glass" feel
		NVGpaint glow = nvgBoxGradient(vg, 0, 0, w, h, 4.0f, 20.0f, nvgRGBA(255, 255, 255, 10), nvgRGBA(0, 0, 0, 40));
		nvgBeginPath(vg);
		nvgRoundedRect(vg, 0, 0, w, h, 4.0f);
		nvgFillPaint(vg, glow);
		nvgFill(vg);

		// 4. Coordinates (HUD)
		int cx, cy;
		canvas->GetScreenCenter(&cx, &cy);
		int current_floor = g_gui.GetCurrentFloor();
		std::string coords = std::format("{}, {}, {}", cx, cy, current_floor);

		nvgFontSize(vg, 12.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);

		nvgFillColor(vg, nvgRGBA(0, 0, 0, 180)); // Shadow
		nvgText(vg, w - 7, h - 5, coords.c_str(), nullptr);

		nvgFillColor(vg, nvgRGBA(255, 255, 255, 220));
		nvgText(vg, w - 8, h - 6, coords.c_str(), nullptr);

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

	// Start Panning
	target_map_x = static_cast<double>(new_map_x);
	target_map_y = static_cast<double>(new_map_y);

	int cx, cy;
	g_gui.GetCurrentMapTab()->GetCanvas()->GetScreenCenter(&cx, &cy);
	current_map_x = static_cast<double>(cx);
	current_map_y = static_cast<double>(cy);

	is_panning = true;
	if (!pan_timer.IsRunning()) {
		pan_timer.Start(16); // 60fps
	}
}

void MinimapWindow::OnKey(wxKeyEvent& event) {
	if (g_gui.GetCurrentTab() != nullptr) {
		g_gui.GetCurrentMapTab()->GetEventHandler()->AddPendingEvent(event);
	}
}
