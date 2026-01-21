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
			int ts = tileSize();
			int mapX = (screenX + scrollX) / ts;
			int mapY = (screenY + scrollY) / ts;
			return Point(mapX, mapY);
		}

		Point Viewport::tileToScreen(int tileX, int tileY) const noexcept {
			int ts = tileSize();
			int screenX = tileX * ts - scrollX;
			int screenY = tileY * ts - scrollY;
			return Point(screenX, screenY);
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
