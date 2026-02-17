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
#include <cmath>

#include "rendering/core/graphics.h"
#include "editor/editor.h"
#include "map/map.h"

#include "ui/gui.h"
#include "rendering/ui/map_display.h"
#include "rendering/ui/minimap_window.h"

#include "rendering/drawers/minimap_drawer.h"

#define NANOVG_GL3
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
	m_smoothX(0.0),
	m_smoothY(0.0),
	m_lastFrameTime(0),
	m_dragging(false) {
	spdlog::info("MinimapWindow::MinimapWindow - Creating context");
	context = std::make_unique<wxGLContext>(this);
	if (!context->IsOK()) {
		spdlog::error("MinimapWindow::MinimapWindow - Context creation failed");
	}
	SetToolTip("Click to move camera");
	drawer = std::make_unique<MinimapDrawer>();

	Bind(wxEVT_LEFT_DOWN, &MinimapWindow::OnMouseClick, this);
	Bind(wxEVT_MOTION, &MinimapWindow::OnMouseMove, this);
	Bind(wxEVT_LEFT_UP, &MinimapWindow::OnMouseUp, this);
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

	// Smooth Panning Logic
	int targetX, targetY;
	canvas->GetScreenCenter(&targetX, &targetY);

	// Initialize if first run or reset
	if (m_smoothX == 0 && m_smoothY == 0) {
		m_smoothX = targetX;
		m_smoothY = targetY;
	}

	// Calculate Delta Time
	wxLongLong now = wxGetLocalTimeMillis();
	double dt = 0.016; // Default to ~60 FPS
	if (m_lastFrameTime > 0) {
		dt = (now - m_lastFrameTime).ToDouble() / 1000.0;
		if (dt > 0.1) dt = 0.1; // Cap at 100ms to prevent huge jumps
	}
	m_lastFrameTime = now;

	// Smooth Damp
	// Higher speed = snappier, Lower = smoother
	double speed = 15.0;

	// Snap if very close to avoid jitter
	if (std::abs(targetX - m_smoothX) < 0.5) m_smoothX = targetX;
	else m_smoothX += (targetX - m_smoothX) * speed * dt;

	if (std::abs(targetY - m_smoothY) < 0.5) m_smoothY = targetY;
	else m_smoothY += (targetY - m_smoothY) * speed * dt;

	// Draw Minimap with interpolated center
	drawer->Draw(dc, GetSize(), editor, canvas, Position(static_cast<int>(m_smoothX), static_cast<int>(m_smoothY), 0));

	// Glass Overlay & HUD
	NVGcontext* vg = nvg.get();
	if (vg) {
		glClear(GL_STENCIL_BUFFER_BIT);
		int w, h;
		GetClientSize(&w, &h);
		nvgBeginFrame(vg, w, h, GetContentScaleFactor());

		// 1. Draw View Box (Camera HUD)
		// Get View Box logic (borrowed from MinimapDrawer but adapted for NanoVG)
		if (g_settings.getInteger(Config::MINIMAP_VIEW_BOX)) {
			int screensize_x, screensize_y;
			int view_scroll_x, view_scroll_y;
			canvas->GetViewBox(&view_scroll_x, &view_scroll_y, &screensize_x, &screensize_y);

			int floor = g_gui.GetCurrentFloor();
			// Typically TileSize is 32. Using constant 32 as per MinimapDrawer implementation assumption.
			// Ideally should use TileSize constant.
			const int TS = 32;
			int floor_offset = (floor > GROUND_LAYER ? 0 : (GROUND_LAYER - floor));

			int view_start_x = view_scroll_x / TS + floor_offset;
			int view_start_y = view_scroll_y / TS + floor_offset;

			int tile_size = int(TS / canvas->GetZoom());
			if (tile_size < 1) tile_size = 1;
			int view_w = screensize_x / tile_size + 1;
			int view_h = screensize_y / tile_size + 1;

			// Convert to local minimap coords
			// We need the start_x/y that was just used for drawing
			int map_start_x = drawer->GetLastStartX();
			int map_start_y = drawer->GetLastStartY();

			float box_x = (float)(view_start_x - map_start_x);
			float box_y = (float)(view_start_y - map_start_y);
			float box_w = (float)view_w;
			float box_h = (float)view_h;

			// Draw View Box
			nvgBeginPath(vg);
			nvgRoundedRect(vg, box_x, box_y, box_w, box_h, 3.0f);

			// Inner Glow
			NVGpaint boxGlow = nvgBoxGradient(vg, box_x, box_y, box_w, box_h, 3.0f, 4.0f, nvgRGBA(255, 255, 255, 32), nvgRGBA(0, 0, 0, 0));
			nvgPathWinding(vg, NVG_HOLE);
			nvgFillPaint(vg, boxGlow);
			nvgFill(vg);

			// Stroke
			nvgBeginPath(vg);
			nvgRoundedRect(vg, box_x, box_y, box_w, box_h, 3.0f);
			nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 128));
			nvgStrokeWidth(vg, 1.5f);
			nvgStroke(vg);
		}

		// 2. Glass Overlay (Enhanced)
		// Outer border
		nvgBeginPath(vg);
		nvgRoundedRect(vg, 1.5f, 1.5f, w - 3.0f, h - 3.0f, 4.0f);
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 40));
		nvgStrokeWidth(vg, 3.0f);
		nvgStroke(vg);

		nvgBeginPath(vg);
		nvgRoundedRect(vg, 1.5f, 1.5f, w - 3.0f, h - 3.0f, 4.0f);
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 80));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);

		// Inner glow (Vignette)
		NVGpaint glow = nvgBoxGradient(vg, 0, 0, w, h, 4.0f, 20.0f, nvgRGBA(255, 255, 255, 10), nvgRGBA(0, 0, 0, 80));
		nvgBeginPath(vg);
		nvgRoundedRect(vg, 0, 0, w, h, 4.0f);
		nvgFillPaint(vg, glow);
		nvgFill(vg);

		nvgEndFrame(vg);
	}

	SwapBuffers();

	// If we are animating (not at target), request another frame
	if (std::abs(targetX - m_smoothX) > 0.1 || std::abs(targetY - m_smoothY) > 0.1) {
		Refresh();
	}
}

void MinimapWindow::OnMouseClick(wxMouseEvent& event) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	m_dragging = true;
	m_lastMousePos = event.GetPosition();

	// Jump to position on click
	int new_map_x, new_map_y;
	drawer->ScreenToMap(event.GetX(), event.GetY(), new_map_x, new_map_y);
	g_gui.SetScreenCenterPosition(Position(new_map_x, new_map_y, g_gui.GetCurrentFloor()));

	// Force refresh to start animation/update immediately
	Refresh();
	g_gui.RefreshView();
}

void MinimapWindow::OnMouseMove(wxMouseEvent& event) {
	if (m_dragging && event.Dragging() && event.LeftIsDown()) {
		if (!g_gui.IsEditorOpen()) return;

		wxPoint currentPos = event.GetPosition();
		int dx = currentPos.x - m_lastMousePos.x;
		int dy = currentPos.y - m_lastMousePos.y;

		if (dx != 0 || dy != 0) {
			// Calculate new center
			int currentCenterX, currentCenterY;
			g_gui.GetCurrentMapTab()->GetCanvas()->GetScreenCenter(&currentCenterX, &currentCenterY);

			// Minimap scale is typically 1:1, so move by dx/dy
			// If I click and drag the camera box, it should follow the mouse.
			// So if mouse moves RIGHT, camera center should move RIGHT.
			int newCenterX = currentCenterX + dx;
			int newCenterY = currentCenterY + dy;

			g_gui.SetScreenCenterPosition(Position(newCenterX, newCenterY, g_gui.GetCurrentFloor()));
			m_lastMousePos = currentPos;
			g_gui.RefreshView();
			Refresh();
		}
	}
	event.Skip();
}

void MinimapWindow::OnMouseUp(wxMouseEvent& event) {
	if (m_dragging) {
		m_dragging = false;
	}
	event.Skip();
}

void MinimapWindow::OnKey(wxKeyEvent& event) {
	if (g_gui.GetCurrentTab() != nullptr) {
		g_gui.GetCurrentMapTab()->GetEventHandler()->AddPendingEvent(event);
	}
}
