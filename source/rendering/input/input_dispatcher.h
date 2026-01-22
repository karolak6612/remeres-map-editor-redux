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

#ifndef RME_INPUT_DISPATCHER_H_
#define RME_INPUT_DISPATCHER_H_

#include "input_types.h"
#include "coordinate_mapper.h"
#include "mouse_handler.h"
#include <memory>
#include <vector>
#include <algorithm>

namespace rme {
	namespace input {

		/// Interface for objects that can receive input events
		class InputReceiver {
		public:
			virtual ~InputReceiver() = default;

			virtual void onMouseMove(const MouseEvent& event) { }
			virtual void onMouseClick(const MouseEvent& event) { }
			virtual void onMouseDoubleClick(const MouseEvent& event) { }
			virtual void onMouseDrag(const MouseEvent& event, const DragState& drag) { }
			virtual void onMouseWheel(const MouseEvent& event) { }
			virtual void onMouseDown(const MouseEvent& event) { }
			virtual void onMouseUp(const MouseEvent& event) { }
			virtual void onDragEnd(const DragState& drag) { }
			virtual void onKeyDown(const KeyEvent& event) { }
			virtual void onKeyUp(const KeyEvent& event) { }
		};

		/// Central dispatcher for all input events
		/// Coordinates between raw input and the various input handlers
		class InputDispatcher {
		public:
			InputDispatcher();
			~InputDispatcher() = default;

			/// Initialize the dispatcher
			/// @param mapper Pointer to the shared coordinate mapper
			void initialize(const CoordinateMapper* mapper);

			/// Shutdown and cleanup
			void shutdown();

			/// Add an input receiver for events
			void addReceiver(InputReceiver* receiver);

			/// Remove an input receiver
			void removeReceiver(InputReceiver* receiver);

			/// Access the coordinate mapper
			const CoordinateMapper* coordinateMapper() const {
				return coordinateMapper_;
			}

			/// Access the mouse handler
			MouseHandler& mouseHandler() {
				return mouseHandler_;
			}
			const MouseHandler& mouseHandler() const {
				return mouseHandler_;
			}

			// Viewport updates are no longer needed as we use shared mapper state

			// Raw input entry points (call these from wxWidgets event handlers)

			/// Process raw mouse motion event
			void handleMouseMove(int x, int y, bool ctrl, bool alt, bool shift);

			/// Process raw mouse button down event
			void handleMouseDown(int x, int y, MouseButton button, bool ctrl, bool alt, bool shift);

			/// Process raw mouse button up event
			void handleMouseUp(int x, int y, MouseButton button, bool ctrl, bool alt, bool shift);

			/// Process raw mouse wheel event
			void handleMouseWheel(int x, int y, int delta, bool ctrl, bool alt, bool shift);

			/// Process raw double-click event
			void handleMouseDoubleClick(int x, int y, MouseButton button, bool ctrl, bool alt, bool shift);

			/// Process raw key down event
			void handleKeyDown(int32_t keyCode, bool ctrl, bool alt, bool shift);

			/// Process raw key up event
			void handleKeyUp(int32_t keyCode, bool ctrl, bool alt, bool shift);

			// State queries
			[[nodiscard]] bool isDragging() const {
				return mouseHandler_.isDragging();
			}
			[[nodiscard]] const MapCoord& mouseMapPos() const {
				return mouseHandler_.currentMapPos();
			}
			[[nodiscard]] const ScreenCoord& mouseScreenPos() const {
				return mouseHandler_.currentScreenPos();
			}

		private:
			const CoordinateMapper* coordinateMapper_ = nullptr;
			MouseHandler mouseHandler_;
			std::vector<InputReceiver*> receivers_;
			bool initialized_ = false;

			// Create KeyModifiers from booleans
			static KeyModifiers makeModifiers(bool ctrl, bool alt, bool shift);

			// Setup callbacks that forward to receiver
			void setupCallbacks();
		};

	} // namespace input
} // namespace rme

#endif // RME_INPUT_DISPATCHER_H_
