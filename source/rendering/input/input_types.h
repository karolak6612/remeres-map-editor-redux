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

#ifndef RME_INPUT_TYPES_H_
#define RME_INPUT_TYPES_H_

#include <cstdint>

namespace rme {
	namespace input {

		/// Screen coordinates (raw mouse position in pixels)
		struct ScreenCoord {
			int x = 0;
			int y = 0;

			constexpr ScreenCoord() = default;
			constexpr ScreenCoord(int x, int y) :
				x(x), y(y) { }
		};

		/// Map coordinates (tile position on the map)
		struct MapCoord {
			int x = 0;
			int y = 0;
			int z = 0; // Floor level

			constexpr MapCoord() = default;
			constexpr MapCoord(int x, int y, int z = 7) :
				x(x), y(y), z(z) { }

			[[nodiscard]] constexpr bool operator==(const MapCoord& other) const noexcept {
				return x == other.x && y == other.y && z == other.z;
			}
			[[nodiscard]] constexpr bool operator!=(const MapCoord& other) const noexcept {
				return !(*this == other);
			}
		};

		/// Mouse button enumeration
		enum class MouseButton : uint8_t {
			None = 0,
			Left = 1,
			Right = 2,
			Middle = 3,
			Extra1 = 4,
			Extra2 = 5
		};

		/// Keyboard modifiers
		struct KeyModifiers {
			bool ctrl = false;
			bool alt = false;
			bool shift = false;
			bool meta = false; // Windows/Command key

			[[nodiscard]] constexpr bool none() const noexcept {
				return !ctrl && !alt && !shift && !meta;
			}
			[[nodiscard]] constexpr bool any() const noexcept {
				return ctrl || alt || shift || meta;
			}
		};

		/// Input event types
		enum class InputEventType : uint8_t {
			MouseMove,
			MouseDown,
			MouseUp,
			MouseClick,
			MouseDoubleClick,
			MouseDrag,
			MouseWheel,
			KeyDown,
			KeyUp,
			KeyPress
		};

		/// Mouse event data
		struct MouseEvent {
			InputEventType type = InputEventType::MouseMove;
			ScreenCoord screenPos;
			MapCoord mapPos;
			MouseButton button = MouseButton::None;
			KeyModifiers modifiers;
			int wheelDelta = 0;
			double timestamp = 0.0;

			[[nodiscard]] bool isLeftButton() const noexcept {
				return button == MouseButton::Left;
			}
			[[nodiscard]] bool isRightButton() const noexcept {
				return button == MouseButton::Right;
			}
			[[nodiscard]] bool isMiddleButton() const noexcept {
				return button == MouseButton::Middle;
			}
		};

		/// Keyboard event data
		struct KeyEvent {
			InputEventType type = InputEventType::KeyDown;
			int32_t keyCode = 0;
			KeyModifiers modifiers;
			double timestamp = 0.0;
		};

		/// Drag state for tracking mouse drag operations
		struct DragState {
			bool active = false;
			ScreenCoord startScreen;
			MapCoord startMap;
			ScreenCoord currentScreen;
			MapCoord currentMap;
			MouseButton button = MouseButton::None;

			void begin(const MouseEvent& event) {
				active = true;
				startScreen = event.screenPos;
				startMap = event.mapPos;
				currentScreen = event.screenPos;
				currentMap = event.mapPos;
				button = event.button;
			}

			void update(const MouseEvent& event) {
				currentScreen = event.screenPos;
				currentMap = event.mapPos;
			}

			void end() {
				active = false;
			}

			[[nodiscard]] int deltaX() const noexcept {
				return currentScreen.x - startScreen.x;
			}
			[[nodiscard]] int deltaY() const noexcept {
				return currentScreen.y - startScreen.y;
			}
			[[nodiscard]] int mapDeltaX() const noexcept {
				return currentMap.x - startMap.x;
			}
			[[nodiscard]] int mapDeltaY() const noexcept {
				return currentMap.y - startMap.y;
			}
		};

	} // namespace input
} // namespace rme

#endif // RME_INPUT_TYPES_H_
