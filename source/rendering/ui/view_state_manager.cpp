#include "app/main.h"
#include "rendering/ui/view_state_manager.h"
#include "rendering/core/coordinate_mapper.h"
#include "ui/map_window.h"
#include <wx/window.h>
#include <algorithm>

ViewStateManager::ViewStateManager(wxWindow* canvas_window) :
	canvas_(canvas_window),
	floor_(GROUND_LAYER),
	zoom_(1.0) {
}

void ViewStateManager::setFloor(int floor) {
	floor_ = std::clamp(floor, MIN_FLOOR, MAX_FLOOR);
}

void ViewStateManager::changeFloor(int new_floor, bool notify_scrollbar) {
	setFloor(new_floor);
}

void ViewStateManager::setZoom(double value) {
	zoom_ = std::clamp(value, MIN_ZOOM, MAX_ZOOM);
}

int ViewStateManager::getScrollX() const {
	int x = 0, y = 0;
	static_cast<MapWindow*>(canvas_->GetParent())->GetViewStart(&x, &y);
	return x;
}

int ViewStateManager::getScrollY() const {
	int x = 0, y = 0;
	static_cast<MapWindow*>(canvas_->GetParent())->GetViewStart(&x, &y);
	return y;
}

void ViewStateManager::screenToMap(int screen_x, int screen_y, int* map_x, int* map_y) const {
	int start_x = getScrollX();
	int start_y = getScrollY();
	CoordinateMapper::ScreenToMap(screen_x, screen_y, start_x, start_y, zoom_, floor_,
		canvas_->GetContentScaleFactor(), map_x, map_y);
}
