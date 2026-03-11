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

#include "rendering/core/graphics.h"
#include "editor/editor.h"
#include "map/map.h"

#include "ui/gui.h"
#include "rendering/ui/map_display.h"
#include "rendering/ui/minimap_window.h"

#include "rendering/drawers/minimap_drawer.h"

// Helper to create attributes
static wxGLAttributes& GetCoreProfileAttributes() {
	static wxGLAttributes vAttrs = []() {
		wxGLAttributes a;
		a.PlatformDefaults().Defaults().RGBA().DoubleBuffer().Depth(24).Stencil(8).EndList();
		return a;
	}();
	return vAttrs;
}

MinimapWindow::MinimapWindow(wxWindow* parent, GUI& gui, Settings& settings) :
	wxGLCanvas(parent, GetCoreProfileAttributes(), wxID_ANY, wxDefaultPosition, wxSize(205, 130)),
	update_timer(this),
	gui_(gui),
	settings_(settings) {
	spdlog::info("MinimapWindow::MinimapWindow - Creating own context shared with global");
	m_glContext = std::make_unique<wxGLContext>(this, gui_.GetGLContext(this));
	if (!m_glContext || !m_glContext->IsOK()) {
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
	bool context_ok = false;
	if (m_glContext) {
		context_ok = g_gl_context.EnsureContextCurrent(*m_glContext, this);
	}

	if (context_ok) {
		drawer.reset();
	} else {
		spdlog::warn("MinimapWindow: Destroying without a current OpenGL context. Cleanup might be incomplete.");
	}

	g_gl_context.UnregisterCanvas(this);
	spdlog::debug("MinimapWindow destructor finished");
}

void MinimapWindow::OnSize(wxSizeEvent& event) {
	Refresh();
}

void MinimapWindow::OnClose(wxCloseEvent&) {
	spdlog::info("MinimapWindow::OnClose called");
	spdlog::default_logger()->flush();
	gui_.DestroyMinimap();
}

void MinimapWindow::DelayedUpdate() {
	// We only updated the window AFTER actions have taken place, that
	// way we don't waste too much performance on updating this window
	update_timer.Start(settings_.getInteger(Config::MINIMAP_UPDATE_DELAY), true);
}

void MinimapWindow::OnDelayedUpdate(wxTimerEvent& event) {
	Refresh();
}

void MinimapWindow::OnPaint(wxPaintEvent& event) {
	wxPaintDC dc(this); // validates the paint event

	if (!m_glContext) {
		spdlog::error("MinimapWindow::OnPaint - No context!");
		return;
	}

	SetCurrent(*m_glContext);

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

	if (!gui_.IsEditorOpen()) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		SwapBuffers();
		return;
	}
	Editor& editor = *gui_.GetCurrentEditor();
	MapCanvas* canvas = gui_.GetCurrentMapTab()->GetCanvas();

	// Mock dc passed to Draw, unused by new GL implementation
	drawer->Draw(dc, GetSize(), editor, canvas);

	SwapBuffers();
}

void MinimapWindow::OnMouseClick(wxMouseEvent& event) {
	if (!gui_.IsEditorOpen()) {
		return;
	}
	int new_map_x, new_map_y;
	drawer->ScreenToMap(event.GetX(), event.GetY(), new_map_x, new_map_y);

	gui_.SetScreenCenterPosition(Position(new_map_x, new_map_y, gui_.GetCurrentFloor()));
	Refresh();
	gui_.RefreshView();
}

void MinimapWindow::OnKey(wxKeyEvent& event) {
	if (gui_.GetCurrentTab() != nullptr) {
		gui_.GetCurrentMapTab()->GetEventHandler()->AddPendingEvent(event);
	}
}
