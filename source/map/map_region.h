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
 * @file map_region.h
 * @brief Manages spatial regions and tile storage locations.
 *
 * Defines structures for organizing tiles into regions (MapNode), floors (Floor),
 * and individual tile containers (TileLocation). These classes manage memory
 * layout and visibility for the map.
 */

#ifndef RME_MAP_REGION_H
#define RME_MAP_REGION_H

#include "map/position.h"
#include "map/tile.h"
#include "map/spatial_hash_grid.h"
#include <utility>
#include <unordered_map>

class Tile;
class Floor;
class BaseMap;
class MapNode;

/**
 * @brief Holds a Tile and its metadata at a specific map position.
 *
 * TileLocation acts as a container for a Tile pointer and associated counters
 * (spawns, waypoints, towns) and house exits pointing to this location.
 * It ensures these properties persist even if the Tile object itself is replaced.
 */
class TileLocation {
	TileLocation();

public:
	~TileLocation();

	TileLocation(const TileLocation&) = delete;
	TileLocation& operator=(const TileLocation&) = delete;

protected:
	std::unique_ptr<Tile> tile;
	Position position;
	size_t spawn_count;
	size_t waypoint_count;
	size_t town_count;
	std::unique_ptr<HouseExitList> house_exits; // Any house exits pointing here

public:
	// Access tile
	// Can't set directly since that does not update tile count
	/**
	 * @brief Gets the tile stored at this location.
	 * @return Pointer to Tile.
	 */
	Tile* get() {
		return tile.get();
	}

	/**
	 * @brief Gets the tile (const).
	 * @return Const pointer to Tile.
	 */
	const Tile* get() const {
		return tile.get();
	}

	/**
	 * @brief Gets the number of items/entities at this location.
	 * @return Item count.
	 */
	int size() const;

	/**
	 * @brief Checks if the location is empty (no tile or empty tile).
	 * @return true if empty.
	 */
	bool empty() const;

	/**
	 * @brief Gets the absolute position of this location.
	 * @return Position struct.
	 */
	Position getPosition() const {
		return position;
	}

	int getX() const {
		return position.x;
	}
	int getY() const {
		return position.y;
	}
	int getZ() const {
		return position.z;
	}

	/**
	 * @brief Gets the count of spawns at this location.
	 * @return Spawn count.
	 */
	size_t getSpawnCount() const {
		return spawn_count;
	}
	void increaseSpawnCount() {
		spawn_count++;
	}
	void decreaseSpawnCount() {
		spawn_count--;
	}

	/**
	 * @brief Gets the count of waypoints at this location.
	 * @return Waypoint count.
	 */
	size_t getWaypointCount() const {
		return waypoint_count;
	}
	void increaseWaypointCount() {
		waypoint_count++;
	}
	void decreaseWaypointCount() {
		waypoint_count--;
	}

	/**
	 * @brief Gets the count of towns referencing this location.
	 * @return Town count.
	 */
	size_t getTownCount() const {
		return town_count;
	}
	void increaseTownCount() {
		town_count++;
	}
	void decreaseTownCount() {
		town_count--;
	}

	/**
	 * @brief Creates or retrieves the list of house exits pointing here.
	 * @return Pointer to HouseExitList.
	 */
	HouseExitList* createHouseExits() {
		if (house_exits) {
			return house_exits.get();
		}
		house_exits = std::make_unique<HouseExitList>();
		return house_exits.get();
	}

	/**
	 * @brief Gets the house exits list (if any).
	 * @return Pointer to HouseExitList or nullptr.
	 */
	HouseExitList* getHouseExits() {
		return house_exits.get();
	}

	friend class Floor;
	friend class MapNode;
	friend class Waypoints;
};

/**
 * @brief Represents a single Z-layer (floor) within a map region.
 *
 * Contains a fixed array of TileLocations.
 */
class Floor {
public:
	Floor(int x, int y, int z);
	TileLocation locs[MAP_LAYERS]; ///< Fixed size array of locations on this floor? (Note: MAP_LAYERS name is confusing if this is a floor)
};

/**
 * @brief Represents a spatial chunk of the map (Map Region).
 *
 * A MapNode typically covers a rectangular area and manages all floors
 * within that area. It handles visibility state and tile access.
 */
class MapNode {
public:
	MapNode(BaseMap& map);
	~MapNode();

	MapNode(const MapNode&) = delete;
	MapNode& operator=(const MapNode&) = delete;

	/**
	 * @brief Creates a new tile location at the given coordinates.
	 * @param x Local X coordinate?
	 * @param y Local Y coordinate?
	 * @param z Local Z coordinate?
	 * @return Pointer to new TileLocation.
	 */
	TileLocation* createTile(int x, int y, int z);

	/**
	 * @brief Retrieves a tile location.
	 * @param x Local X coordinate.
	 * @param y Local Y coordinate.
	 * @param z Local Z coordinate.
	 * @return Pointer to TileLocation.
	 */
	TileLocation* getTile(int x, int y, int z);

	/**
	 * @brief Sets a tile at the given coordinates.
	 * @param x Local X coordinate.
	 * @param y Local Y coordinate.
	 * @param z Local Z coordinate.
	 * @param tile Unique pointer to the new Tile.
	 * @return Unique pointer to any previous tile.
	 */
	std::unique_ptr<Tile> setTile(int x, int y, int z, std::unique_ptr<Tile> tile);

	/**
	 * @brief Clears (removes) a tile at the given coordinates.
	 * @param x Local X coordinate.
	 * @param y Local Y coordinate.
	 * @param z Local Z coordinate.
	 */
	void clearTile(int x, int y, int z);

	/**
	 * @brief Creates a floor structure at the given coordinates.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 * @param z Z coordinate.
	 * @return Pointer to the new Floor.
	 */
	Floor* createFloor(int x, int y, int z);

	/**
	 * @brief Gets an existing floor at a Z-level.
	 * @param z Z-level index.
	 * @return Pointer to Floor.
	 */
	Floor* getFloor(uint32_t z) {
		return array[z].get();
	}

	/**
	 * @brief Checks if a floor exists at a Z-level.
	 * @param z Z-level index.
	 * @return true if exists.
	 */
	bool hasFloor(uint32_t z);

	/**
	 * @brief Sets visibility flags for this node.
	 * @param underground Target underground layer?
	 * @param value Visibility state.
	 */
	void setVisible(bool underground, bool value);
	void setVisible(uint32_t client, bool underground, bool value);
	bool isVisible(uint32_t client, bool underground);
	void clearVisible(uint32_t client);

	bool isRequested(bool underground);
	void setRequested(bool underground, bool r);
	bool isVisible(bool underground);

	enum VisibilityFlags : uint32_t {
		VISIBLE_OVERGROUND = 1 << 0,
		VISIBLE_UNDERGROUND = 1 << 1,
		REQUESTED_UNDERGROUND = 1 << 2,
		REQUESTED_OVERGROUND = 1 << 3,
	};

protected:
	BaseMap& map;
	uint32_t visible;
	std::array<std::unique_ptr<Floor>, MAP_LAYERS> array;

	friend class BaseMap;
	friend class MapIterator;
	friend class SpatialHashGrid;
};

#endif
