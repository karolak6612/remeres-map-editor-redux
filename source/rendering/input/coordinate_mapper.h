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

#ifndef RME_COORDINATE_MAPPER_H_
#define RME_COORDINATE_MAPPER_H_

#include "input_types.h"

namespace rme {
	namespace input {

		/// Converts between screen coordinates and map coordinates
		/// Handles zoom, scroll offset, and floor level calculations
		class CoordinateMapper {
		public:
			CoordinateMapper() = default;

			/// Update viewport parameters
			/// @param viewWidth Viewport width in pixels
			/// @param viewHeight Viewport height in pixels
			/// @param zoom Current zoom level (1.0 = normal)
			void setViewport(int viewWidth, int viewHeight, float zoom);

			/// Update scroll offset
			/// @param scrollX Horizontal scroll offset in pixels
			/// @param scrollY Vertical scroll offset in pixels
			void setScroll(int scrollX, int scrollY);

			/// Set current floor level
			/// @param floor Floor level (0-15, 7 = ground)
			void setFloor(int floor);

			/// Set tile size (base size before zoom)
			/// @param size Tile size in pixels
			void setTileSize(int size);

			/// Convert screen coordinates to map coordinates
			/// @param screen Screen position in pixels
			/// @return Map position (tile coordinates)
			[[nodiscard]] MapCoord screenToMap(const ScreenCoord& screen) const;

			/// Convert map coordinates to screen coordinates
			/// @param map Map position (tile coordinates)
			/// @return Screen position in pixels
			[[nodiscard]] ScreenCoord mapToScreen(const MapCoord& map) const;

			/// Get the offset within a tile (for sub-tile precision)
			/// @param screen Screen position
			/// @return Offset within tile (0 to tileSize-1)
			[[nodiscard]] ScreenCoord getTileOffset(const ScreenCoord& screen) const;

			/// Check if a screen coordinate is within the visible viewport
			/// @param screen Screen position to check
			/// @return true if visible
			[[nodiscard]] bool isVisible(const ScreenCoord& screen) const;

			/// Check if a map coordinate is within the visible tile range
			/// @param map Map position to check
			/// @return true if visible
			[[nodiscard]] bool isVisible(const MapCoord& map) const;

			// Getters for current state
			[[nodiscard]] int viewWidth() const noexcept {
				return viewWidth_;
			}
			[[nodiscard]] int viewHeight() const noexcept {
				return viewHeight_;
			}
			[[nodiscard]] float zoom() const noexcept {
				return zoom_;
			}
			[[nodiscard]] int scrollX() const noexcept {
				return scrollX_;
			}
			[[nodiscard]] int scrollY() const noexcept {
				return scrollY_;
			}
			[[nodiscard]] int floor() const noexcept {
				return floor_;
			}
			[[nodiscard]] int tileSize() const noexcept {
				return tileSize_;
			}
			[[nodiscard]] int effectiveTileSize() const noexcept {
				return static_cast<int>(tileSize_ * zoom_);
			}

			// Visible tile range
			[[nodiscard]] int startTileX() const noexcept {
				return startTileX_;
			}
			[[nodiscard]] int startTileY() const noexcept {
				return startTileY_;
			}
			[[nodiscard]] int endTileX() const noexcept {
				return endTileX_;
			}
			[[nodiscard]] int endTileY() const noexcept {
				return endTileY_;
			}

		private:
			int viewWidth_ = 800;
			int viewHeight_ = 600;
			float zoom_ = 1.0f;
			int scrollX_ = 0;
			int scrollY_ = 0;
			int floor_ = 7;
			int tileSize_ = 32;

			// Cached visible tile range
			int startTileX_ = 0;
			int startTileY_ = 0;
			int endTileX_ = 0;
			int endTileY_ = 0;

			/// Recalculate visible tile range
			void updateVisibleRange();
		};

	} // namespace input
} // namespace rme

#endif // RME_COORDINATE_MAPPER_H_
