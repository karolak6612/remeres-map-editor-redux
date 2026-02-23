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

#ifndef RME_MINIMAP_WINDOW_H_
#define RME_MINIMAP_WINDOW_H_

#include <wx/glcanvas.h>
#include <memory>

#include "rendering/core/graphics.h"
#include "util/nanovg_canvas.h"

class MinimapDrawer;
struct NVGcontext;

class MinimapWindow : public wxGLCanvas {
public:
	MinimapWindow(wxWindow* parent);
	~MinimapWindow() override;

	void OnPaint(wxPaintEvent&);
	void OnEraseBackground(wxEraseEvent&) { }
	void OnMouseClick(wxMouseEvent&);
	void OnSize(wxSizeEvent&);
	void OnClose(wxCloseEvent&);

	void DelayedUpdate();
	void OnDelayedUpdate(wxTimerEvent& event);
	void OnKey(wxKeyEvent& event);

protected:
	void EnsureNanoVG();
	std::unique_ptr<MinimapDrawer> drawer;
	wxTimer update_timer;
	wxGLContext* context;

	std::unique_ptr<NVGcontext, NVGDeleter> m_nvg;
};

#endif
