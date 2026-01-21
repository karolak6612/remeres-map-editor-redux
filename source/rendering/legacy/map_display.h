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

#ifndef RME_DISPLAY_WINDOW_H_
#define RME_DISPLAY_WINDOW_H_

#include "../../main.h"

#include "../../action.h"
#include "../../tile.h"
#include "../../creature.h"
#include <memory>

// New rendering architecture includes
#include "../opengl/gl_context.h"
#include "../pipeline/render_coordinator.h"
#include "../pipeline/render_state.h"
#include "../input/input_dispatcher.h"
#include "../input/input_types.h"

class Item;
class Creature;
class MapWindow;
class MapPopupMenu;
class AnimationTimer;
class MapDrawer;

namespace rme {
	namespace input {
		class CameraInputHandler;
		class BrushInputHandler;
		class SelectionInputHandler;
	}
}

/// MapCanvas - Thin wxGLCanvas wrapper following Proposition 2 architecture
/// Delegates rendering to RenderCoordinator and input to InputDispatcher
class MapCanvas : public wxGLCanvas {
public:
	MapCanvas(MapWindow* parent, Editor& editor, int* attriblist);
	virtual ~MapCanvas();
	void Reset();

	// ========================================================================
	// wxWidgets Event Handlers (thin - delegate to InputDispatcher)
	// ========================================================================
	void OnPaint(wxPaintEvent& event);
	void OnEraseBackground(wxEraseEvent& event) { }

	void OnMouseMove(wxMouseEvent& event);
	void OnMouseLeftRelease(wxMouseEvent& event);
	void OnMouseLeftClick(wxMouseEvent& event);
	void OnMouseLeftDoubleClick(wxMouseEvent& event);
	void OnMouseCenterClick(wxMouseEvent& event);
	void OnMouseCenterRelease(wxMouseEvent& event);
	void OnMouseRightClick(wxMouseEvent& event);
	void OnMouseRightRelease(wxMouseEvent& event);

	void OnKeyDown(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void OnWheel(wxMouseEvent& event);
	void OnGainMouse(wxMouseEvent& event);
	void OnLoseMouse(wxMouseEvent& event);

	// ========================================================================
	// Mouse Action Handlers (application logic)
	// ========================================================================
	void OnMouseActionRelease(wxMouseEvent& event);
	void OnMouseActionClick(wxMouseEvent& event);
	void OnMouseCameraClick(wxMouseEvent& event);
	void OnMouseCameraRelease(wxMouseEvent& event);
	void OnMousePropertiesClick(wxMouseEvent& event);
	void OnMousePropertiesRelease(wxMouseEvent& event);

	// ========================================================================
	// Menu Command Handlers
	// ========================================================================
	void OnCut(wxCommandEvent& event);
	void OnCopy(wxCommandEvent& event);
	void OnCopyPosition(wxCommandEvent& event);
	void OnCopyServerId(wxCommandEvent& event);
	void OnCopyClientId(wxCommandEvent& event);
	void OnCopyName(wxCommandEvent& event);
	void OnBrowseTile(wxCommandEvent& event);
	void OnPaste(wxCommandEvent& event);
	void OnDelete(wxCommandEvent& event);
	void OnGotoDestination(wxCommandEvent& event);
	void OnRotateItem(wxCommandEvent& event);
	void OnSwitchDoor(wxCommandEvent& event);
	void OnSelectRAWBrush(wxCommandEvent& event);
	void OnSelectGroundBrush(wxCommandEvent& event);
	void OnSelectDoodadBrush(wxCommandEvent& event);
	void OnSelectDoorBrush(wxCommandEvent& event);
	void OnSelectWallBrush(wxCommandEvent& event);
	void OnSelectCarpetBrush(wxCommandEvent& event);
	void OnSelectTableBrush(wxCommandEvent& event);
	void OnSelectCreatureBrush(wxCommandEvent& event);
	void OnSelectSpawnBrush(wxCommandEvent& event);
	void OnSelectHouseBrush(wxCommandEvent& event);
	void OnSelectCollectionBrush(wxCommandEvent& event);
	void OnSelectMoveTo(wxCommandEvent& event);
	void OnProperties(wxCommandEvent& event);

	// ========================================================================
	// Public Interface (maintained for compatibility)
	// ========================================================================
	void Refresh();

	void ScreenToMap(int screen_x, int screen_y, int* map_x, int* map_y);
	void MouseToMap(int* map_x, int* map_y) {
		ScreenToMap(cursor_x_, cursor_y_, map_x, map_y);
	}
	void GetScreenCenter(int* map_x, int* map_y);

	void StartPasting();
	void EndPasting();
	void EnterSelectionMode();
	void EnterDrawingMode();

	void UpdatePositionStatus(int x = -1, int y = -1);
	void UpdateZoomStatus();

	void ChangeFloor(int new_floor);
	int GetFloor() const {
		return floor_;
	}
	double GetZoom() const {
		return zoom_;
	}
	void SetZoom(double value);
	void GetViewBox(int* view_scroll_x, int* view_scroll_y, int* screensize_x, int* screensize_y) const;

	Position GetCursorPosition() const;

	void TakeScreenshot(wxFileName path, wxString format);

	// ========================================================================
	// InputReceiver Interface (callbacks from InputDispatcher)
	// ========================================================================
	void onMouseMove(const rme::input::MouseEvent& event) override;
	void onMouseClick(const rme::input::MouseEvent& event) override;
	void onMouseDoubleClick(const rme::input::MouseEvent& event) override;
	void onMouseDrag(const rme::input::MouseEvent& event, const rme::input::DragState& drag) override;
	void onMouseWheel(const rme::input::MouseEvent& event) override;

protected:
	void getTilesToDraw(int mouse_map_x, int mouse_map_y, int floor, PositionVector* tilestodraw, PositionVector* tilestoborder, bool fill = false);
	bool floodFill(Map* map, const Position& center, int x, int y, GroundBrush* brush, PositionVector* positions);

private:
	enum {
		BLOCK_SIZE = 100
	};

	inline int getFillIndex(int x, int y) const {
		return x + BLOCK_SIZE * y;
	}

	static bool processed[BLOCK_SIZE * BLOCK_SIZE];

	// ========================================================================
	// NEW ARCHITECTURE - Core Systems
	// ========================================================================
	std::unique_ptr<rme::render::RenderCoordinator> renderCoordinator_;
	rme::render::RenderState renderState_;
	rme::input::InputDispatcher inputDispatcher_;
	std::unique_ptr<rme::input::CameraInputHandler> cameraHandler_;
	std::unique_ptr<rme::input::BrushInputHandler> brushHandler_;
	std::unique_ptr<rme::input::SelectionInputHandler> selectionHandler_;

	void initializeRenderingSystems();
	void shutdownRenderingSystems();
	void syncRenderState();

	// ========================================================================
	// Legacy State (maintained for compatibility, will migrate incrementally)
	// ========================================================================
	Editor& editor_;
	MapDrawer* drawer_; // Legacy drawer - kept for transition
	int keyCode_;
	int countMaxFills_ = 0;

	// View state
	int floor_;
	double zoom_;
	int cursor_x_;
	int cursor_y_;

	// Interaction state
	bool dragging_;
	bool boundbox_selection_;
	bool screendragging_;
	bool isPasting() const;
	bool drawing_;
	bool dragging_draw_;
	bool replace_dragging_;

	uint8_t* screenshot_buffer_;

	int drag_start_x_;
	int drag_start_y_;
	int drag_start_z_;

	int last_cursor_map_x_;
	int last_cursor_map_y_;
	int last_cursor_map_z_;

	int last_click_map_x_;
	int last_click_map_y_;
	int last_click_map_z_;
	int last_click_abs_x_;
	int last_click_abs_y_;
	int last_click_x_;
	int last_click_y_;

	int last_mmb_click_x_;
	int last_mmb_click_y_;

	int view_scroll_x_;
	int view_scroll_y_;

	uint32_t current_house_id_;

	wxStopWatch refresh_watch_;
	MapPopupMenu* popup_menu_;
	AnimationTimer* animation_timer_;

	friend class MapDrawer;
	friend class rme::input::CameraInputHandler;
	friend class rme::input::BrushInputHandler;
	friend class rme::input::SelectionInputHandler;

	DECLARE_EVENT_TABLE()
};

// Right-click popup menu
class MapPopupMenu : public wxMenu {
public:
	MapPopupMenu(Editor& editor);
	virtual ~MapPopupMenu();

	void Update();

protected:
	Editor& editor;
};

class AnimationTimer : public wxTimer {
public:
	AnimationTimer(MapCanvas* canvas);
	~AnimationTimer();

	void Notify();
	void Start();
	void Stop();

private:
	MapCanvas* map_canvas;
	bool started;
};

#endif
