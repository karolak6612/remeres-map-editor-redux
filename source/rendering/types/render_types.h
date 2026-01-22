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

#ifndef RME_RENDER_TYPES_H_
#define RME_RENDER_TYPES_H_

#include <cstdint>

namespace rme {
	namespace render {

		// Forward declarations
		struct Viewport;
		/// Options controlling what is rendered
		struct RenderOptions {
			bool showGrid = false;
			bool showLights = false;
			bool showLightStr = true; // Legacy
			bool showTechItems = true;
			bool showWaypoints = true;
			bool ingame = false;
			bool isDrawing = false;
			bool isDragging = false;

			bool showAllFloors = true;
			bool showCreatures = true;
			bool showSpawns = true;
			bool showHouses = true;
			bool showShade = true;
			bool showSpecialTiles = true;
			bool showItems = true;

			bool highlightItems = false;
			bool highlightLockedDoors = true;
			bool showBlocking = false;
			bool showTooltips = false;
			bool showAsMinimap = false;
			bool showOnlyColors = false;
			bool showOnlyModified = false;
			bool showPreview = false;
			bool showHooks = false;
			bool hideItemsWhenZoomed = true;

			bool alwaysShowZones = false;
			bool extendedHouseShader = false;

			bool transparentFloors = false;
			bool transparentItems = false;
			bool showIngameBox = false;
			bool experimentalFog = false;
			bool showTowns = false; // Add this one too, used in TileRenderer

			int brushSize = 1;
			int brushShape = 0; // Square
		};

		class RenderState;

		/// 2D point with integer coordinates
		struct Point {
			int x = 0;
			int y = 0;

			constexpr Point() = default;
			constexpr Point(int x, int y) :
				x(x), y(y) { }

			[[nodiscard]] constexpr bool operator==(const Point& other) const noexcept {
				return x == other.x && y == other.y;
			}
			[[nodiscard]] constexpr bool operator!=(const Point& other) const noexcept {
				return !(*this == other);
			}
			[[nodiscard]] constexpr Point operator+(const Point& other) const noexcept {
				return Point(x + other.x, y + other.y);
			}
			[[nodiscard]] constexpr Point operator-(const Point& other) const noexcept {
				return Point(x - other.x, y - other.y);
			}
		};

		/// 2D size with integer dimensions
		struct Size {
			int width = 0;
			int height = 0;

			constexpr Size() = default;
			constexpr Size(int w, int h) :
				width(w), height(h) { }

			[[nodiscard]] constexpr bool isValid() const noexcept {
				return width > 0 && height > 0;
			}
			[[nodiscard]] constexpr int area() const noexcept {
				return width * height;
			}
		};

		/// Rectangle defined by position and size
		struct Rect {
			int x = 0;
			int y = 0;
			int width = 0;
			int height = 0;

			constexpr Rect() = default;
			constexpr Rect(int x, int y, int w, int h) :
				x(x), y(y), width(w), height(h) { }
			constexpr Rect(const Point& pos, const Size& size) :
				x(pos.x), y(pos.y), width(size.width), height(size.height) { }

			[[nodiscard]] constexpr Point position() const noexcept {
				return Point(x, y);
			}
			[[nodiscard]] constexpr Size size() const noexcept {
				return Size(width, height);
			}
			[[nodiscard]] constexpr int right() const noexcept {
				return x + width;
			}
			[[nodiscard]] constexpr int bottom() const noexcept {
				return y + height;
			}

			[[nodiscard]] constexpr bool contains(int px, int py) const noexcept {
				return px >= x && px < x + width && py >= y && py < y + height;
			}
			[[nodiscard]] constexpr bool contains(const Point& p) const noexcept {
				return contains(p.x, p.y);
			}
		};

		// Tile constants
		inline constexpr int kTileSize = 32;
		inline constexpr int kGroundLayer = 7;
		inline constexpr int kMaxLayer = 15;
		inline constexpr int kMinLayer = 0;

		/// Render pass order (determines draw sequence)
		enum class RenderPass : uint8_t {
			Background = 0,
			Tiles,
			Selection,
			DraggingShadow,
			HigherFloors,
			Brush,
			Grid,
			Light,
			UI,
			Tooltips,
			Count
		};

		/// Blend modes for rendering
		enum class BlendMode : uint8_t {
			Normal = 0,
			Additive,
			Multiply,
			Light
		};

	} // namespace render
} // namespace rme

#endif // RME_RENDER_TYPES_H_
