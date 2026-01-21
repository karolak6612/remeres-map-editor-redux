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

#include "main.h"
#include "mouse_handler.h"
#include <cmath>
#include <chrono>

namespace rme {
	namespace input {

		MouseHandler::MouseHandler() {
			for (int i = 0; i < 6; ++i) {
				buttonState_[i] = false;
			}
		}

		void MouseHandler::setCoordinateMapper(CoordinateMapper* mapper) {
			mapper_ = mapper;
		}

		void MouseHandler::updatePosition(int screenX, int screenY) {
			currentScreen_ = ScreenCoord(screenX, screenY);
			if (mapper_) {
				currentMap_ = mapper_->screenToMap(currentScreen_);
			}
		}

		MouseEvent MouseHandler::createEvent(InputEventType type, const ScreenCoord& screen, MouseButton button, const KeyModifiers& modifiers) {
			MouseEvent event;
			event.type = type;
			event.screenPos = screen;
			if (mapper_) {
				event.mapPos = mapper_->screenToMap(screen);
			}
			event.button = button;
			event.modifiers = modifiers;

			auto now = std::chrono::high_resolution_clock::now();
			auto duration = now.time_since_epoch();
			event.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1000000.0;

			return event;
		}

		bool MouseHandler::checkDragThreshold(const ScreenCoord& start, const ScreenCoord& current) const {
			int dx = std::abs(current.x - start.x);
			int dy = std::abs(current.y - start.y);
			return dx > dragThreshold_ || dy > dragThreshold_;
		}

		bool MouseHandler::isButtonDown(MouseButton button) const noexcept {
			int idx = static_cast<int>(button);
			if (idx >= 0 && idx < 6) {
				return buttonState_[idx];
			}
			return false;
		}

		void MouseHandler::onMouseMove(int screenX, int screenY, const KeyModifiers& modifiers) {
			updatePosition(screenX, screenY);

			// Check if we should start dragging
			if (!dragState_.active) {
				for (int i = 1; i < 6; ++i) {
					if (buttonState_[i] && checkDragThreshold(mouseDownPos_, currentScreen_)) {
						// Start drag
						MouseEvent startEvent = createEvent(InputEventType::MouseDown, mouseDownPos_, static_cast<MouseButton>(i), modifiers);
						dragState_.begin(startEvent);
						break;
					}
				}
			}

			// Update drag if active
			if (dragState_.active) {
				MouseEvent event = createEvent(InputEventType::MouseDrag, currentScreen_, dragState_.button, modifiers);
				dragState_.update(event);

				if (dragCallback_) {
					dragCallback_(event, dragState_);
				}
			} else {
				// Normal mouse move
				if (moveCallback_) {
					MouseEvent event = createEvent(InputEventType::MouseMove, currentScreen_, MouseButton::None, modifiers);
					moveCallback_(event);
				}
			}
		}

		void MouseHandler::onMouseDown(int screenX, int screenY, MouseButton button, const KeyModifiers& modifiers) {
			updatePosition(screenX, screenY);

			int idx = static_cast<int>(button);
			if (idx >= 0 && idx < 6) {
				buttonState_[idx] = true;
			}

			mouseDownPos_ = currentScreen_;

			if (mouseDownCallback_) {
				mouseDownCallback_(createEvent(InputEventType::MouseDown, currentScreen_, button, modifiers));
			}
		}

		void MouseHandler::onMouseUp(int screenX, int screenY, MouseButton button, const KeyModifiers& modifiers) {
			updatePosition(screenX, screenY);

			int idx = static_cast<int>(button);
			if (idx >= 0 && idx < 6) {
				buttonState_[idx] = false;
			}

			if (mouseUpCallback_) {
				mouseUpCallback_(createEvent(InputEventType::MouseUp, currentScreen_, button, modifiers));
			}

			// Check if this was a drag ending or a click
			if (dragState_.active && dragState_.button == button) {
				// End drag
				dragState_.end();
			} else {
				// This was a click (no drag started)
				if (clickCallback_) {
					MouseEvent event = createEvent(InputEventType::MouseClick, currentScreen_, button, modifiers);
					clickCallback_(event);
				}
			}
		}

		void MouseHandler::onMouseWheel(int screenX, int screenY, int delta, const KeyModifiers& modifiers) {
			updatePosition(screenX, screenY);

			if (wheelCallback_) {
				MouseEvent event = createEvent(InputEventType::MouseWheel, currentScreen_, MouseButton::None, modifiers);
				event.wheelDelta = delta;
				wheelCallback_(event);
			}
		}

		void MouseHandler::onMouseDoubleClick(int screenX, int screenY, MouseButton button, const KeyModifiers& modifiers) {
			updatePosition(screenX, screenY);

			if (doubleClickCallback_) {
				MouseEvent event = createEvent(InputEventType::MouseDoubleClick, currentScreen_, button, modifiers);
				doubleClickCallback_(event);
			}
		}

	} // namespace input
} // namespace rme
