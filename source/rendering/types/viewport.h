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

#ifndef RME_RENDER_VIEWPORT_H_
#define RME_RENDER_VIEWPORT_H_

#include "render_types.h"

namespace rme {
	namespace render {

		/// Viewport represents the visible area of the map
		/// Handles coordinate transformations between screen and map space
		struct Viewport {
			int scrollX = 0; ///< Horizontal scroll offset in pixels
			int scrollY = 0; ///< Vertical scroll offset in pixels
			int width = 0; ///< Viewport width in pixels
			int height = 0; ///< Viewport height in pixels
			int floor = kGroundLayer; ///< Current floor being viewed
			float zoom = 1.0f; ///< Zoom level (1.0 = 100%)

			constexpr Viewport() = default;

			/// Calculate effective tile size based on zoom
			[[nodiscard]] constexpr int tileSize() const noexcept {
				return static_cast<int>(kTileSize / zoom);
			}

			/// Get the starting tile X coordinate visible in viewport
			[[nodiscard]] constexpr int startTileX() const noexcept {
				return scrollX / kTileSize;
			}

			/// Get the starting tile Y coordinate visible in viewport
			[[nodiscard]] constexpr int startTileY() const noexcept {
				return scrollY / kTileSize;
			}

			/// Get the ending tile X coordinate (exclusive) visible in viewport
			[[nodiscard]] constexpr int endTileX() const noexcept {
				return startTileX() + (width / tileSize()) + 2;
			}

			/// Get the ending tile Y coordinate (exclusive) visible in viewport
			[[nodiscard]] constexpr int endTileY() const noexcept {
				return startTileY() + (height / tileSize()) + 2;
			}

			/// Get floor adjustment offset for perspective rendering
			[[nodiscard]] constexpr int floorAdjustment() const noexcept {
				if (floor > kGroundLayer) {
					return 0;
				}
				return kTileSize * (kGroundLayer - floor);
			}

			/// Convert screen coordinates to map tile coordinates
			[[nodiscard]] Point screenToTile(int screenX, int screenY) const noexcept;

			/// Convert map tile coordinates to screen coordinates
			[[nodiscard]] Point tileToScreen(int tileX, int tileY) const noexcept;

			/// Get the center tile position
			[[nodiscard]] Point centerTile() const noexcept;

			/// Check if a tile is visible in the viewport
			[[nodiscard]] bool isTileVisible(int tileX, int tileY) const noexcept;

			/// Get the screen X coordinate for a given tile X
			[[nodiscard]] int getScreenX(int tileX) const noexcept;

			/// Get the screen Y coordinate for a given tile Y
			[[nodiscard]] int getScreenY(int tileY) const noexcept;
		};

	} // namespace render
} // namespace rme

#endif // RME_RENDER_VIEWPORT_H_
