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

#include "../../main.h"
#include "input_dispatcher.h"
#include <chrono>

namespace rme {
	namespace input {

		InputDispatcher::InputDispatcher() = default;

		void InputDispatcher::initialize() {
			if (initialized_) {
				return;
			}

			// Connect mouse handler to coordinate mapper
			mouseHandler_.setCoordinateMapper(&coordinateMapper_);

			// Setup default viewport
			coordinateMapper_.setTileSize(32);
			coordinateMapper_.setViewport(800, 600, 1.0f);

			initialized_ = true;
		}

		void InputDispatcher::shutdown() {
			receivers_.clear();
			initialized_ = false;
		}

		void InputDispatcher::addReceiver(InputReceiver* receiver) {
			if (receiver) {
				receivers_.push_back(receiver);
				setupCallbacks();
			}
		}

		void InputDispatcher::removeReceiver(InputReceiver* receiver) {
			receivers_.erase(std::remove(receivers_.begin(), receivers_.end(), receiver), receivers_.end());
			setupCallbacks();
		}

		void InputDispatcher::setupCallbacks() {
			// Forward mouse move events to receivers
			mouseHandler_.setMoveCallback([this](const MouseEvent& event) {
				for (auto* receiver : receivers_) {
					receiver->onMouseMove(event);
				}
			});

			// Forward click events to receivers
			mouseHandler_.setClickCallback([this](const MouseEvent& event) {
				for (auto* receiver : receivers_) {
					receiver->onMouseClick(event);
				}
			});

			// Forward double-click events to receivers
			mouseHandler_.setDoubleClickCallback([this](const MouseEvent& event) {
				for (auto* receiver : receivers_) {
					receiver->onMouseDoubleClick(event);
				}
			});

			// Forward drag events to receivers
			mouseHandler_.setDragCallback([this](const MouseEvent& event, const DragState& drag) {
				for (auto* receiver : receivers_) {
					receiver->onMouseDrag(event, drag);
				}
			});

			// Forward wheel events to receivers
			mouseHandler_.setWheelCallback([this](const MouseEvent& event) {
				for (auto* receiver : receivers_) {
					receiver->onMouseWheel(event);
				}
			});

			mouseHandler_.setMouseDownCallback([this](const MouseEvent& event) {
				for (auto* receiver : receivers_) {
					receiver->onMouseDown(event);
				}
			});

			mouseHandler_.setMouseUpCallback([this](const MouseEvent& event) {
				for (auto* receiver : receivers_) {
					receiver->onMouseUp(event);
				}
			});
		}

		void InputDispatcher::setViewport(int width, int height, float zoom) {
			coordinateMapper_.setViewport(width, height, zoom);
		}

		void InputDispatcher::setScroll(int scrollX, int scrollY) {
			coordinateMapper_.setScroll(scrollX, scrollY);
		}

		void InputDispatcher::setFloor(int floor) {
			coordinateMapper_.setFloor(floor);
		}

		KeyModifiers InputDispatcher::makeModifiers(bool ctrl, bool alt, bool shift) {
			KeyModifiers mods;
			mods.ctrl = ctrl;
			mods.alt = alt;
			mods.shift = shift;
			mods.meta = false;
			return mods;
		}

		void InputDispatcher::handleMouseMove(int x, int y, bool ctrl, bool alt, bool shift) {
			mouseHandler_.onMouseMove(x, y, makeModifiers(ctrl, alt, shift));
		}

		void InputDispatcher::handleMouseDown(int x, int y, MouseButton button, bool ctrl, bool alt, bool shift) {
			mouseHandler_.onMouseDown(x, y, button, makeModifiers(ctrl, alt, shift));
		}

		void InputDispatcher::handleMouseUp(int x, int y, MouseButton button, bool ctrl, bool alt, bool shift) {
			mouseHandler_.onMouseUp(x, y, button, makeModifiers(ctrl, alt, shift));
		}

		void InputDispatcher::handleMouseWheel(int x, int y, int delta, bool ctrl, bool alt, bool shift) {
			mouseHandler_.onMouseWheel(x, y, delta, makeModifiers(ctrl, alt, shift));
		}

		void InputDispatcher::handleMouseDoubleClick(int x, int y, MouseButton button, bool ctrl, bool alt, bool shift) {
			mouseHandler_.onMouseDoubleClick(x, y, button, makeModifiers(ctrl, alt, shift));
		}

		void InputDispatcher::handleKeyDown(int32_t keyCode, bool ctrl, bool alt, bool shift) {
			KeyEvent event;
			event.type = InputEventType::KeyDown;
			event.keyCode = keyCode;
			event.modifiers = makeModifiers(ctrl, alt, shift);

			auto now = std::chrono::high_resolution_clock::now();
			auto duration = now.time_since_epoch();
			event.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1000000.0;

			for (auto* receiver : receivers_) {
				receiver->onKeyDown(event);
			}
		}

		void InputDispatcher::handleKeyUp(int32_t keyCode, bool ctrl, bool alt, bool shift) {
			KeyEvent event;
			event.type = InputEventType::KeyUp;
			event.keyCode = keyCode;
			event.modifiers = makeModifiers(ctrl, alt, shift);

			auto now = std::chrono::high_resolution_clock::now();
			auto duration = now.time_since_epoch();
			event.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1000000.0;

			for (auto* receiver : receivers_) {
				receiver->onKeyUp(event);
			}
		}

	} // namespace input
} // namespace rme
