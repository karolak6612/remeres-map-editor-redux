#ifndef RME_RENDERING_UI_VIEW_STATE_MANAGER_H_
#define RME_RENDERING_UI_VIEW_STATE_MANAGER_H_

#include <algorithm>

class wxWindow;

// Owns the viewport navigation state: floor, zoom.
// Scroll is delegated to the parent MapWindow (wxScrolledWindow).
// Single writer: all navigation changes go through this class.
// Readers: MapCanvas (for snapshot), sub-controllers (via const ref).
class ViewStateManager {
public:
	explicit ViewStateManager(wxWindow* canvas_window);

	// Floor management
	void setFloor(int floor);
	[[nodiscard]] int getFloor() const noexcept { return floor_; }
	void changeFloor(int new_floor, bool notify_scrollbar = true);

	// Zoom management
	[[nodiscard]] double getZoom() const noexcept { return zoom_; }
	void setZoom(double value);

	// Scroll (delegated to MapWindow)
	[[nodiscard]] int getScrollX() const;
	[[nodiscard]] int getScrollY() const;

	// Screen-to-map coordinate transformation (pure math, depends on scroll/zoom)
	void screenToMap(int screen_x, int screen_y, int* map_x, int* map_y) const;

	static constexpr double MIN_ZOOM = 0.125;
	static constexpr double MAX_ZOOM = 25.0;
	static constexpr int MIN_FLOOR = 0;
	static constexpr int MAX_FLOOR = 15;

private:
	wxWindow* canvas_; // non-owning, for scroll access via parent MapWindow
	int floor_ = 7;
	double zoom_ = 1.0;
};

#endif
