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

#ifndef RME_MOUSE_HANDLER_H_
#define RME_MOUSE_HANDLER_H_

#include "input_types.h"
#include "coordinate_mapper.h"
#include <functional>

namespace rme {
	namespace input {

		/// Callback types for mouse events
		using MouseMoveCallback = std::function<void(const MouseEvent&)>;
		using MouseClickCallback = std::function<void(const MouseEvent&)>;
		using MouseDragCallback = std::function<void(const MouseEvent&, const DragState&)>;
		using MouseWheelCallback = std::function<void(const MouseEvent&)>;
		using MouseButtonCallback = std::function<void(const MouseEvent&)>;

		/// Handles mouse input processing and event dispatching
		/// Tracks mouse state, detects clicks vs drags, and dispatches events
		class MouseHandler {
		public:
			MouseHandler();
			~MouseHandler() = default;

			/// Set the coordinate mapper for screen-to-map conversion
			void setCoordinateMapper(const CoordinateMapper* mapper);

			/// Process raw mouse movement
			/// @param screenX Screen X position
			/// @param screenY Screen Y position
			/// @param modifiers Current keyboard modifiers
			void onMouseMove(int screenX, int screenY, const KeyModifiers& modifiers);

			/// Process mouse button press
			/// @param screenX Screen X position
			/// @param screenY Screen Y position
			/// @param button Which button was pressed
			/// @param modifiers Current keyboard modifiers
			void onMouseDown(int screenX, int screenY, MouseButton button, const KeyModifiers& modifiers);

			/// Process mouse button release
			/// @param screenX Screen X position
			/// @param screenY Screen Y position
			/// @param button Which button was released
			/// @param modifiers Current keyboard modifiers
			void onMouseUp(int screenX, int screenY, MouseButton button, const KeyModifiers& modifiers);

			/// Process mouse wheel
			/// @param screenX Screen X position
			/// @param screenY Screen Y position
			/// @param delta Wheel delta (positive = up, negative = down)
			/// @param modifiers Current keyboard modifiers
			void onMouseWheel(int screenX, int screenY, int delta, const KeyModifiers& modifiers);

			/// Process double-click
			/// @param screenX Screen X position
			/// @param screenY Screen Y position
			/// @param button Which button was double-clicked
			/// @param modifiers Current keyboard modifiers
			void onMouseDoubleClick(int screenX, int screenY, MouseButton button, const KeyModifiers& modifiers);

			// Event callbacks
			void setMoveCallback(MouseMoveCallback callback) {
				moveCallback_ = std::move(callback);
			}
			void setClickCallback(MouseClickCallback callback) {
				clickCallback_ = std::move(callback);
			}
			void setDragCallback(MouseDragCallback callback) {
				dragCallback_ = std::move(callback);
			}
			void setWheelCallback(MouseWheelCallback callback) {
				wheelCallback_ = std::move(callback);
			}
			void setDoubleClickCallback(MouseClickCallback callback) {
				doubleClickCallback_ = std::move(callback);
			}
			void setMouseDownCallback(MouseButtonCallback callback) {
				mouseDownCallback_ = std::move(callback);
			}
			void setMouseUpCallback(MouseButtonCallback callback) {
				mouseUpCallback_ = std::move(callback);
			}

			// State accessors
			[[nodiscard]] const ScreenCoord& currentScreenPos() const noexcept {
				return currentScreen_;
			}
			[[nodiscard]] const MapCoord& currentMapPos() const noexcept {
				return currentMap_;
			}
			[[nodiscard]] bool isButtonDown(MouseButton button) const noexcept;
			[[nodiscard]] bool isDragging() const noexcept {
				return dragState_.active;
			}
			[[nodiscard]] const DragState& dragState() const noexcept {
				return dragState_;
			}

			/// Drag detection threshold (in pixels)
			void setDragThreshold(int pixels) {
				dragThreshold_ = pixels;
			}
			[[nodiscard]] int dragThreshold() const noexcept {
				return dragThreshold_;
			}

		private:
			const CoordinateMapper* mapper_ = nullptr;

			// Current state
			ScreenCoord currentScreen_;
			MapCoord currentMap_;
			bool buttonState_[6] = { false }; // Indexed by MouseButton

			// Drag tracking
			DragState dragState_;
			ScreenCoord mouseDownPos_;
			int dragThreshold_ = 3;

			// Callbacks
			MouseMoveCallback moveCallback_;
			MouseClickCallback clickCallback_;
			MouseDragCallback dragCallback_;
			MouseWheelCallback wheelCallback_;
			MouseClickCallback doubleClickCallback_;
			MouseButtonCallback mouseDownCallback_;
			MouseButtonCallback mouseUpCallback_;

			// Helpers
			MouseEvent createEvent(InputEventType type, const ScreenCoord& screen, MouseButton button, const KeyModifiers& modifiers);
			void updatePosition(int screenX, int screenY);
			bool checkDragThreshold(const ScreenCoord& start, const ScreenCoord& current) const;
		};

	} // namespace input
} // namespace rme

#endif // RME_MOUSE_HANDLER_H_
