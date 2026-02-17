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

/*
 * @file basemap.h
 * @brief Base class for map data structures.
 *
 * Defines the BaseMap class which manages the low-level spatial hash grid
 * of tiles, and the MapIterator for iterating over all tiles in the map.
 */

#ifndef RME_BASE_MAP_H_
#define RME_BASE_MAP_H_

#include "app/main.h"
#include "map/position.h"
#include "io/filehandle.h"
#include "map/map_allocator.h"
#include "map/tile.h"
#include "map/spatial_hash_grid.h"
#include <unordered_map>
#include <memory>
#include <iterator>
#include <ranges>

// Class declarations
class SpatialHashGrid;
class BaseMap;
class MapIterator;
class Floor;
class MapNode;
class TileLocation;

/*
 * @brief Iterator for traversing all tiles in a BaseMap.
 *
 * Provides a forward iterator compatible with C++ standard algorithms.
 * It iterates through the spatial hash grid structure to visit every allocated tile.
 */
class MapIterator {
public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = TileLocation;
	using difference_type = std::ptrdiff_t;
	using pointer = TileLocation*;
	using reference = TileLocation&;

	/*
	 * @brief Constructs a MapIterator.
	 * @param _map The map to iterate over (nullptr for end iterator).
	 */
	MapIterator(BaseMap* _map = nullptr);
	~MapIterator() = default;
	MapIterator(const MapIterator& other);

	/*
	 * @brief Dereferences the iterator to get the current TileLocation.
	 * @return Reference to the TileLocation.
	 */
	TileLocation& operator*() const noexcept;

	/*
	 * @brief Dereferences the iterator to get a pointer to the current TileLocation.
	 * @return Pointer to the TileLocation.
	 */
	TileLocation* operator->() noexcept {
		return current_tile;
	}
	const TileLocation* operator->() const noexcept {
		return current_tile;
	}

	/*
	 * @brief Advances the iterator to the next tile (prefix).
	 * @return Reference to self.
	 */
	MapIterator& operator++() noexcept;

	/*
	 * @brief Advances the iterator to the next tile (postfix).
	 * @return Copy of previous state.
	 */
	MapIterator operator++(int) noexcept;

	/*
	 * @brief Checks for equality with another iterator.
	 * @param other The other iterator.
	 * @return true if equal.
	 */
	bool operator==(const MapIterator& other) const noexcept;

	/*
	 * @brief Checks for inequality.
	 * @param other The other iterator.
	 * @return true if not equal.
	 */
	bool operator!=(const MapIterator& other) const noexcept {
		return !(other == *this);
	}

private:
	bool findNext();

	using CellIterator = std::unordered_map<uint64_t, std::unique_ptr<SpatialHashGrid::GridCell>>::iterator;
	CellIterator cell_it;
	int node_i, floor_i, tile_i;

	TileLocation* current_tile;
	BaseMap* map;

	friend class BaseMap;
};

/*
 * @brief Base class for managing map tiles and spatial organization.
 *
 * BaseMap wraps the SpatialHashGrid and provides a high-level API for
 * accessing, creating, and modifying tiles at specific coordinates.
 * It also manages memory allocation for map elements.
 */
class BaseMap {
public:
	BaseMap();
	virtual ~BaseMap() = default;

	// This doesn't destroy the map structure, just clears it, if param is true, delete all tiles too.
	/*
	 * @brief Clears the map contents.
	 * @param del If true, deletes all tile objects.
	 */
	void clear(bool del = true);

	/*
	 * @brief Returns an iterator to the first tile.
	 * @return Begin iterator.
	 */
	MapIterator begin();

	/*
	 * @brief Returns an iterator to the end of the map.
	 * @return End iterator.
	 */
	MapIterator end();

	/*
	 * @brief Returns a range view of all tiles.
	 * @return C++20 subrange.
	 */
	auto tiles() {
		return std::ranges::subrange(begin(), end());
	}

	/*
	 * @brief Gets the total number of allocated tiles.
	 * @return Tile count.
	 */
	uint64_t size() const {
		return tilecount;
	}

	// these functions take a position and returns a tile on the map
	/*
	 * @brief Creates a new tile at the specified coordinates.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 * @param z Z coordinate.
	 * @return Pointer to the new Tile.
	 */
	Tile* createTile(int x, int y, int z);

	/*
	 * @brief Gets an existing tile at the specified coordinates.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 * @param z Z coordinate.
	 * @return Pointer to the Tile, or nullptr if not found.
	 */
	Tile* getTile(int x, int y, int z);

	/*
	 * @brief Gets an existing tile at the specified position.
	 * @param pos The position.
	 * @return Pointer to the Tile, or nullptr.
	 */
	Tile* getTile(const Position& pos);

	/*
	 * @brief Gets an existing tile or creates a new one if missing.
	 * @param pos The position.
	 * @return Pointer to the Tile.
	 */
	Tile* getOrCreateTile(const Position& pos);

	/*
	 * @brief Gets a tile (const version).
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 * @param z Z coordinate.
	 * @return Const pointer to Tile.
	 */
	const Tile* getTile(int x, int y, int z) const;

	/*
	 * @brief Gets a tile (const version).
	 * @param pos The position.
	 * @return Const pointer to Tile.
	 */
	const Tile* getTile(const Position& pos) const;

	/*
	 * @brief Gets the internal TileLocation structure.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 * @param z Z coordinate.
	 * @return Pointer to TileLocation.
	 */
	TileLocation* getTileL(int x, int y, int z);
	TileLocation* getTileL(const Position& pos);

	/*
	 * @brief Creates a new TileLocation.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 * @param z Z coordinate.
	 * @return Pointer to the new TileLocation.
	 */
	TileLocation* createTileL(int x, int y, int z);
	TileLocation* createTileL(const Position& pos);

	/*
	 * @brief Gets TileLocation (const).
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 * @param z Z coordinate.
	 * @return Const pointer to TileLocation.
	 */
	const TileLocation* getTileL(int x, int y, int z) const;
	const TileLocation* getTileL(const Position& pos) const;

	// Get a Map Node from the map
	/*
	 * @brief Gets the map node (chunk) for the given coordinates.
	 * @param x X coordinate (absolute).
	 * @param y Y coordinate (absolute).
	 * @return Pointer to MapNode.
	 */
	MapNode* getLeaf(int x, int y) {
		return grid.getLeaf(x, y);
	}

	/*
	 * @brief Creates or retrieves a map node for the given coordinates.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 * @return Pointer to MapNode.
	 */
	MapNode* createLeaf(int x, int y) {
		return grid.getLeafForce(x, y);
	}

	/*
	 * @brief Visits all map nodes within a rectangular area.
	 * @tparam Func Callback type.
	 * @param min_x Min X bounds.
	 * @param min_y Min Y bounds.
	 * @param max_x Max X bounds.
	 * @param max_y Max Y bounds.
	 * @param func Callback to execute on each node.
	 */
	template <typename Func>
	void visitLeaves(int min_x, int min_y, int max_x, int max_y, Func&& func) {
		grid.visitLeaves(min_x, min_y, max_x, max_y, std::forward<Func>(func));
	}

	// Assigns a tile, it might seem pointless to provide position, but it is not, as the passed tile may be nullptr
	/*
	 * @brief Replaces the tile at specific coordinates with a new one.
	 * @param _x X coordinate.
	 * @param _y Y coordinate.
	 * @param _z Z coordinate.
	 * @param newtile Unique pointer to the new tile (takes ownership).
	 * @return Unique pointer to the OLD tile (if any), or nullptr.
	 */
	[[nodiscard]] std::unique_ptr<Tile> setTile(int _x, int _y, int _z, std::unique_ptr<Tile> newtile);

	/*
	 * @brief Replaces the tile at a specific position.
	 * @param pos The position.
	 * @param newtile The new tile.
	 * @return The old tile.
	 */
	[[nodiscard]] std::unique_ptr<Tile> setTile(const Position& pos, std::unique_ptr<Tile> newtile) {
		return setTile(pos.x, pos.y, pos.z, std::move(newtile));
	}

	/*
	 * @brief Sets a tile using its internal coordinates.
	 * @param newtile The new tile (must have valid coordinates).
	 * @return The old tile.
	 */
	[[nodiscard]] std::unique_ptr<Tile> setTile(std::unique_ptr<Tile> newtile) {
		ASSERT(newtile);
		int x = newtile->getX();
		int y = newtile->getY();
		int z = newtile->getZ();
		return setTile(x, y, z, std::move(newtile));
	}
	// Replaces a tile and returns the old one
	/*
	 * @brief Swaps a tile at specific coordinates.
	 * @param _x X coordinate.
	 * @param _y Y coordinate.
	 * @param _z Z coordinate.
	 * @param newtile The new tile.
	 * @return The old tile.
	 */
	[[nodiscard]] std::unique_ptr<Tile> swapTile(int _x, int _y, int _z, std::unique_ptr<Tile> newtile);

	/*
	 * @brief Swaps a tile at a specific position.
	 * @param pos The position.
	 * @param newtile The new tile.
	 * @return The old tile.
	 */
	[[nodiscard]] std::unique_ptr<Tile> swapTile(const Position& pos, std::unique_ptr<Tile> newtile) {
		return swapTile(pos.x, pos.y, pos.z, std::move(newtile));
	}

	/*
	 * @brief Gets the underlying spatial hash grid.
	 * @return Reference to SpatialHashGrid.
	 */
	SpatialHashGrid& getGrid() {
		return grid;
	}
	const SpatialHashGrid& getGrid() const {
		return grid;
	}

	// Clears the visiblity according to the mask passed
	/*
	 * @brief Clears visibility flags on all tiles based on a mask.
	 * @param mask The bitmask of flags to clear.
	 */
	void clearVisible(uint32_t mask);

	/*
	 * @brief Gets the total tile count.
	 * @return Tile count.
	 */
	uint64_t getTileCount() const {
		return tilecount;
	}

public:
	MapAllocator allocator;

protected:
	uint64_t tilecount;

	SpatialHashGrid grid; // The Spatial Hash Grid

	friend class MapNode;
	friend class MapProcessor;
	friend class EditorPersistence;
	friend class MapIterator;
};

inline Tile* BaseMap::getTile(int x, int y, int z) {
	TileLocation* l = getTileL(x, y, z);
	return l ? l->get() : nullptr;
}

inline Tile* BaseMap::getTile(const Position& pos) {
	TileLocation* l = getTileL(pos);
	return l ? l->get() : nullptr;
}

inline const Tile* BaseMap::getTile(int x, int y, int z) const {
	const TileLocation* l = getTileL(x, y, z);
	return l ? l->get() : nullptr;
}

inline const Tile* BaseMap::getTile(const Position& pos) const {
	const TileLocation* l = getTileL(pos);
	return l ? l->get() : nullptr;
}

#endif
