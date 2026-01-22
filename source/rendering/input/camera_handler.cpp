//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "../../main.h"
#include <wx/window.h>
#include <wx/glcanvas.h>
#include "camera_handler.h"
#include "../canvas/map_canvas.h"
#include "../../map_window.h"
#include "../../gui.h"
#include "../../settings.h"

namespace rme {
	namespace input {

		CameraInputHandler::CameraInputHandler(rme::canvas::MapCanvas* canvas) :
			canvas_(canvas) {
		}

		void CameraInputHandler::onMouseDrag(const MouseEvent& event, const DragState& drag) {
			if (drag.button == MouseButton::Middle || (drag.button == MouseButton::Right && g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS))) {
				int sx = drag.deltaX();
				int sy = drag.deltaY();

				if (sx != 0 || sy != 0) {
					wxWindow* parent = canvas_->GetParent();
					if (parent) {
						MapWindow* window = static_cast<MapWindow*>(parent);
						window->ScrollRelative(
							int(g_settings.getFloat(Config::SCROLL_SPEED) * canvas_->viewManager().getZoom() * sx),
							int(g_settings.getFloat(Config::SCROLL_SPEED) * canvas_->viewManager().getZoom() * sy)
						);
					}
					canvas_->Refresh();
				}
			}
		}

		void CameraInputHandler::onMouseWheel(const MouseEvent& event) {
			if (event.modifiers.ctrl || event.modifiers.alt) {
				return; // Handled by other logic (floor/brush size)
			}

			double diff = event.wheelDelta * g_settings.getFloat(Config::ZOOM_SPEED) / 640.0;
			double oldZoom = canvas_->viewManager().getZoom();
			double newZoom = oldZoom + diff;

			// Clamp zoom level
			if (newZoom < 0.125) {
				diff = 0.125 - oldZoom;
				newZoom = 0.125;
			}
			if (newZoom > 25.00) {
				diff = 25.00 - oldZoom;
				newZoom = 25.0;
			}

			// Get view size
			int screensize_x, screensize_y;
			wxWindow* parent = canvas_->GetParent();
			if (parent) {
				MapWindow* window = static_cast<MapWindow*>(parent);
				window->GetViewSize(&screensize_x, &screensize_y);

				// Calculate scroll adjustment to keep mouse position stationary
				// This ensures zooming is centered around the cursor, not top-left
				int scroll_x = int(screensize_x * diff * (std::max(event.screenPos.x, 1) / double(screensize_x)));
				int scroll_y = int(screensize_y * diff * (std::max(event.screenPos.y, 1) / double(screensize_y)));

				window->ScrollRelative(-scroll_x, -scroll_y);
			}

			canvas_->viewManager().setZoom(newZoom);
			canvas_->Refresh();
		}

	} // namespace input
} // namespace rme
