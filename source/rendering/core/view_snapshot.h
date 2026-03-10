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

#ifndef RME_RENDERING_CORE_VIEW_SNAPSHOT_H_
#define RME_RENDERING_CORE_VIEW_SNAPSHOT_H_

#include "map/position.h"

class BaseMap;

// Captures all MapCanvas state needed by MapDrawer for a single frame.
// Constructed at the start of OnPaint(), passed into SetupVars() and Draw().
// Eliminates MapDrawer's back-reference to MapCanvas.
struct ViewSnapshot {
	// Core view parameters (used by RenderView::Setup)
	float zoom = 1.0f;
	int floor = 7;
	int mouse_map_x = 0;
	int mouse_map_y = 0;
	int view_scroll_x = 0;
	int view_scroll_y = 0;
	int screensize_x = 0;
	int screensize_y = 0;

	// Canvas-derived transient state for Draw() phase
	int last_click_map_x = 0;
	int last_click_map_y = 0;
	int last_cursor_map_x = 0;
	int last_cursor_map_y = 0;

	// Selection box drawing state
	int last_click_abs_x = 0;
	int last_click_abs_y = 0;
	int cursor_x = 0;
	int cursor_y = 0;

	// Brush overlay / drag state
	bool is_dragging_draw = false;
	Position drag_start;

	// Preview drawer state
	BaseMap* secondary_map = nullptr;
	bool is_pasting = false;
};

#endif
