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

/**
 * @file map_allocator.h
 * @brief Memory allocation helper for map structures.
 *
 * Defines the MapAllocator class, which centralizes the creation of
 * Tile, Floor, and MapNode objects.
 */

#ifndef RME_MAP_ALLOCATOR_H
#define RME_MAP_ALLOCATOR_H

#include "map/tile.h"
#include "map/map_region.h"

class BaseMap;

/**
 * @brief Factory class for allocating map elements.
 *
 * Provides methods to create unique pointers for Tiles, Floors, and MapNodes.
 * This abstraction allows for centralized memory management strategies if needed.
 */
class MapAllocator {

public:
	MapAllocator() { }
	~MapAllocator() { }

	// shorthands for tiles
	/**
	 * @brief Functor operator to allocate a tile.
	 * @param location The location to initialize the tile with.
	 * @return Unique pointer to the new Tile.
	 */
	std::unique_ptr<Tile> operator()(TileLocation* location) {
		return allocateTile(location);
	}

	//
	/**
	 * @brief Allocates a new Tile.
	 * @param location The memory location where the tile will reside.
	 * @return Unique pointer to the new Tile.
	 */
	std::unique_ptr<Tile> allocateTile(TileLocation* location) {
		return std::make_unique<Tile>(*location);
	}

	//
	/**
	 * @brief Allocates a new Floor.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 * @param z Z coordinate.
	 * @return Unique pointer to the new Floor.
	 */
	std::unique_ptr<Floor> allocateFloor(int x, int y, int z) {
		return std::make_unique<Floor>(x, y, z);
	}

	//
	/**
	 * @brief Allocates a new MapNode (region).
	 * @param map The map instance this node belongs to.
	 * @return Unique pointer to the new MapNode.
	 */
	std::unique_ptr<MapNode> allocateNode(BaseMap& map) {
		return std::make_unique<MapNode>(map);
	}
};

#endif
