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

#ifndef RME_TILE_OPERATIONS_H
#define RME_TILE_OPERATIONS_H

class Tile;
class BaseMap;
class Item;
class WallBrush;

namespace TileOperations {
	void borderize(Tile* tile, BaseMap* parent);
	void wallize(Tile* tile, BaseMap* parent);
	void tableize(Tile* tile, BaseMap* parent);
	void carpetize(Tile* tile, BaseMap* parent);

	void cleanBorders(Tile* tile);
	void cleanWalls(Tile* tile, bool dontdelete = false);
	void cleanWalls(Tile* tile, WallBrush* wb);
	void cleanTables(Tile* tile, bool dontdelete = false);

	void addBorderItem(Tile* tile, Item* item);
	void addWallItem(Tile* tile, Item* item);
}

#endif
