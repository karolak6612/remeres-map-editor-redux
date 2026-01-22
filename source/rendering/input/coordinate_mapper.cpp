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

#include "../../logging/logger.h"
#include "main.h"
#include "coordinate_mapper.h"
#include <algorithm>
#include <cmath>

namespace rme {
	namespace input {

		void CoordinateMapper::setViewport(int viewWidth, int viewHeight, float zoom) {
			LOG_RENDER_INFO("[VIEWPORT] Setting CoordinateMapper viewport: {}x{} - Zoom: {}", viewWidth, viewHeight, zoom);
			viewWidth_ = viewWidth;
			viewHeight_ = viewHeight;
			zoom_ = std::max(0.1f, zoom); // Prevent division by zero
			updateVisibleRange();
		}

		void CoordinateMapper::setScroll(int scrollX, int scrollY) {
			LOG_RENDER_DEBUG("[VIEWPORT] Setting CoordinateMapper scroll: ({}, {})", scrollX, scrollY);
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
			// Convert screen position to world-pixel position by dividing by zoom
			// Then add scroll offset and divide by base tile size
			int worldX = static_cast<int>(screen.x / zoom_) + scrollX_;
			int worldY = static_cast<int>(screen.y / zoom_) + scrollY_;

			MapCoord result(worldX / tileSize_, worldY / tileSize_, floor_);
			// LOG_RENDER_TRACE("[MAPPER] Screen: ({},{}) -> World: ({},{}) -> Map: ({},{}) [Zoom: {}, Scroll: ({},{})]",
			// 	screen.x, screen.y, worldX, worldY, result.x, result.y, zoom_, scrollX_, scrollY_);
			return result;
		}

		ScreenCoord CoordinateMapper::mapToScreen(const MapCoord& map) const {
			// Convert map tile position to world-pixel position
			// Subtract scroll offset, then multiply by zoom for screen position
			int worldX = map.x * tileSize_ - scrollX_;
			int worldY = map.y * tileSize_ - scrollY_;

			return ScreenCoord(
				static_cast<int>(worldX * zoom_),
				static_cast<int>(worldY * zoom_)
			);
		}

		ScreenCoord CoordinateMapper::getTileOffset(const ScreenCoord& screen) const {
			// Get world-pixel position
			int worldX = static_cast<int>(screen.x / zoom_) + scrollX_;
			int worldY = static_cast<int>(screen.y / zoom_) + scrollY_;

			// Get position within the tile in world-pixels
			int offsetX = worldX % tileSize_;
			int offsetY = worldY % tileSize_;

			// Handle negative modulo
			if (offsetX < 0) {
				offsetX += tileSize_;
			}
			if (offsetY < 0) {
				offsetY += tileSize_;
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
			// Calculate visible tile range in world-pixels
			// Scroll is already in world-pixels
			startTileX_ = scrollX_ / tileSize_;
			startTileY_ = scrollY_ / tileSize_;

			// End tile is start + world-pixel width of viewport
			// World Width = ViewWidth / Zoom
			int worldWidth = static_cast<int>(viewWidth_ / zoom_);
			int worldHeight = static_cast<int>(viewHeight_ / zoom_);

			endTileX_ = startTileX_ + (worldWidth / tileSize_) + 2; // +1 for partial tile, +1 for safety
			endTileY_ = startTileY_ + (worldHeight / tileSize_) + 2;

			LOG_RENDER_DEBUG("[VIEWPORT] Visible tile range updated: ({},{}) to ({},{}) [Zoom: {}]", startTileX_, startTileY_, endTileX_, endTileY_, zoom_);
		}

	} // namespace input
} // namespace rme
