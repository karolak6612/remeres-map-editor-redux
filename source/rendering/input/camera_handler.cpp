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
							int(g_settings.getFloat(Config::SCROLL_SPEED) * canvas_->GetZoom() * sx),
							int(g_settings.getFloat(Config::SCROLL_SPEED) * canvas_->GetZoom() * sy)
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

			double diff = -event.wheelDelta * g_settings.getFloat(Config::ZOOM_SPEED) / 640.0;
			double oldZoom = canvas_->GetZoom();
			canvas_->SetZoom(oldZoom + diff);

			canvas_->Refresh();
		}

	} // namespace input
} // namespace rme
