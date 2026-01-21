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
#include "coordinate_mapper.h"
#include <algorithm>
#include <cmath>

namespace rme {
	namespace input {

		void CoordinateMapper::setViewport(int viewWidth, int viewHeight, float zoom) {
			viewWidth_ = viewWidth;
			viewHeight_ = viewHeight;
			zoom_ = std::max(0.1f, zoom); // Prevent division by zero
			updateVisibleRange();
		}

		void CoordinateMapper::setScroll(int scrollX, int scrollY) {
			scrollX_ = scrollX;
			scrollY_ = scrollY;
			updateVisibleRange();
		}

		void CoordinateMapper::setFloor(int floor) {
			floor_ = std::clamp(floor, 0, 15);
		}

		void CoordinateMapper::setTileSize(int size) {
			tileSize_ = std::max(1, size);
			updateVisibleRange();
		}

		MapCoord CoordinateMapper::screenToMap(const ScreenCoord& screen) const {
			int effectiveSize = effectiveTileSize();
			if (effectiveSize <= 0) {
				effectiveSize = 1;
			}

			// Convert screen position to map tile position
			int mapX = (screen.x + scrollX_) / effectiveSize;
			int mapY = (screen.y + scrollY_) / effectiveSize;

			return MapCoord(mapX, mapY, floor_);
		}

		ScreenCoord CoordinateMapper::mapToScreen(const MapCoord& map) const {
			int effectiveSize = effectiveTileSize();

			// Convert map tile position to screen position
			int screenX = map.x * effectiveSize - scrollX_;
			int screenY = map.y * effectiveSize - scrollY_;

			return ScreenCoord(screenX, screenY);
		}

		ScreenCoord CoordinateMapper::getTileOffset(const ScreenCoord& screen) const {
			int effectiveSize = effectiveTileSize();
			if (effectiveSize <= 0) {
				effectiveSize = 1;
			}

			// Get position within the tile
			int offsetX = (screen.x + scrollX_) % effectiveSize;
			int offsetY = (screen.y + scrollY_) % effectiveSize;

			// Handle negative modulo
			if (offsetX < 0) {
				offsetX += effectiveSize;
			}
			if (offsetY < 0) {
				offsetY += effectiveSize;
			}

			return ScreenCoord(offsetX, offsetY);
		}

		bool CoordinateMapper::isVisible(const ScreenCoord& screen) const {
			return screen.x >= 0 && screen.x < viewWidth_ && screen.y >= 0 && screen.y < viewHeight_;
		}

		bool CoordinateMapper::isVisible(const MapCoord& map) const {
			// Check floor first
			if (map.z != floor_) {
				return false;
			}

			// Check tile range
			return map.x >= startTileX_ && map.x <= endTileX_ && map.y >= startTileY_ && map.y <= endTileY_;
		}

		void CoordinateMapper::updateVisibleRange() {
			int effectiveSize = effectiveTileSize();
			if (effectiveSize <= 0) {
				effectiveSize = 1;
			}

			// Calculate visible tile range
			// Note: scrollX_ and scrollY_ are in PIXELS
			// We need to convert these to tile coordinates

			startTileX_ = scrollX_ / effectiveSize;
			startTileY_ = scrollY_ / effectiveSize;

			// End tile is start + tiles fitting in view
			endTileX_ = startTileX_ + (viewWidth_ / effectiveSize) + 2; // +1 for partial tile, +1 for safety
			endTileY_ = startTileY_ + (viewHeight_ / effectiveSize) + 2;
		}

	} // namespace input
} // namespace rme
