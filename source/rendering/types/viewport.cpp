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

#include "viewport.h"

namespace rme {
	namespace render {

		Point Viewport::screenToTile(int screenX, int screenY) const noexcept {
			// Consistent with legacy MapCanvas::ScreenToMap and CoordinateMapper::screenToMap
			int worldX = static_cast<int>(screenX / zoom) + scrollX;
			int worldY = static_cast<int>(screenY / zoom) + scrollY;
			int mapX = worldX / kTileSize;
			int mapY = worldY / kTileSize;
			
			if (floor <= kGroundLayer) {
				mapX += kGroundLayer - floor;
				mapY += kGroundLayer - floor;
			}
			
			return Point(mapX, mapY);
		}

		Point Viewport::tileToScreen(int tileX, int tileY) const noexcept {
			// Consistent with legacy and CoordinateMapper::mapToScreen
			int adjustedTileX = tileX;
			int adjustedTileY = tileY;
			
			if (floor <= kGroundLayer) {
				adjustedTileX -= kGroundLayer - floor;
				adjustedTileY -= kGroundLayer - floor;
			}
			
			int worldX = adjustedTileX * kTileSize - scrollX;
			int worldY = adjustedTileY * kTileSize - scrollY;
			
			return Point(
				static_cast<int>(worldX * zoom),
				static_cast<int>(worldY * zoom)
			);
		}

		Point Viewport::centerTile() const noexcept {
			return screenToTile(width / 2, height / 2);
		}

		bool Viewport::isTileVisible(int tileX, int tileY) const noexcept {
			return tileX >= startTileX() && tileX < endTileX() && tileY >= startTileY() && tileY < endTileY();
		}

		int Viewport::getScreenX(int tileX) const noexcept {
			return tileX * tileSize() - scrollX;
		}

		int Viewport::getScreenY(int tileY) const noexcept {
			return tileY * tileSize() - scrollY;
		}

	} // namespace render
} // namespace rme
