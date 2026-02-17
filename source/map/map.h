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
 * @file map.h
 * @brief Main map class representing the entire world.
 *
 * Defines the Map class which inherits from BaseMap and manages global
 * map state including towns, houses, spawns, and conversion logic.
 */

#ifndef RME_MAP_H_
#define RME_MAP_H_

#include "map/basemap.h"
#include "map/tile.h"
#include "game/town.h"
#include "game/house.h"
#include "game/spawn.h"
#include "game/complexitem.h"
#include "game/waypoints.h"
#include "io/templates.h"

/**
 * @brief Represents the entire editable map.
 *
 * The Map class extends BaseMap to include high-level game structures
 * such as towns, houses, and spawns. It also handles map-wide operations
 * like file I/O, format conversion, and error reporting.
 *
 * @see BaseMap
 */
class Map : public BaseMap {
public:
	// ctor and dtor
	Map();
	~Map() override;

	// Operations on the entire map

	/**
	 * @brief Scans the map for invalid tiles and optionally cleans them.
	 *
	 * Checks for tiles with invalid items or states.
	 *
	 * @param showdialog If true, displays a dialog with the results.
	 */
	void cleanInvalidTiles(bool showdialog = false);

	/**
	 * @brief Converts all house tiles from one ID to another.
	 * @param fromId The original house ID.
	 * @param toId The new house ID.
	 */
	void convertHouseTiles(uint32_t fromId, uint32_t toId);

	// Save a bmp image of the minimap

	//
	/**
	 * @brief Converts the map to a different version format.
	 * @param to The target MapVersion.
	 * @param showdialog If true, shows progress/confirmation.
	 * @return true if successful.
	 */
	bool convert(MapVersion to, bool showdialog = false);

	/**
	 * @brief Converts the map using a specific conversion map.
	 * @param cm The conversion rules.
	 * @param showdialog If true, shows progress/confirmation.
	 * @return true if successful.
	 */
	bool convert(const ConversionMap& cm, bool showdialog = false);

	// Query information about the map

	/**
	 * @brief Gets the current version of the map format.
	 * @return The MapVersion.
	 */
	MapVersion getVersion() const;

	/**
	 * @brief Checks if the map has unsaved changes.
	 * @return true if modified since last save.
	 */
	bool hasChanged() const;

	/**
	 * @brief Marks the map as changed.
	 *
	 * Triggers the "modified" state (e.g., adding a * to the window title).
	 *
	 * @return true.
	 */
	bool doChange();

	/**
	 * @brief Clears the modified state.
	 * @return true.
	 */
	bool clearChanges();

	// Errors/warnings
	/**
	 * @brief Checks if there are any warnings from recent operations.
	 * @return true if warnings exist.
	 */
	bool hasWarnings() const {
		return warnings.size() != 0;
	}

	/**
	 * @brief Gets the list of current warnings.
	 * @return Vector of warning strings.
	 */
	const std::vector<std::string>& getWarnings() const {
		return warnings;
	}

	/**
	 * @brief Checks if there is a critical error.
	 * @return true if error exists.
	 */
	bool hasError() const {
		return error.size() != 0;
	}

	/**
	 * @brief Gets the current error message.
	 * @return The error string.
	 */
	wxString getError() const {
		return error;
	}

	// Mess with spawns
	/**
	 * @brief Adds a spawn to the specified tile.
	 * @param spawn The tile where the spawn is located.
	 * @return true if successful.
	 */
	bool addSpawn(Tile* spawn);

	/**
	 * @brief Removes a spawn from the specified tile.
	 * @param tile The tile containing the spawn.
	 */
	void removeSpawn(Tile* tile);

	/**
	 * @brief Removes a spawn from the specified position.
	 * @param position The world position.
	 */
	void removeSpawn(const Position& position) {
		removeSpawn(getTile(position));
	}

	// Returns all possible spawns on the target tile
	/**
	 * @brief Gets all spawns located on a specific tile.
	 * @param t The tile to check.
	 * @return List of spawns.
	 */
	SpawnList getSpawnList(Tile* t);

	/**
	 * @brief Gets all spawns at a specific position.
	 * @param position The world position.
	 * @return List of spawns.
	 */
	SpawnList getSpawnList(const Position& position) {
		return getSpawnList(getTile(position));
	}

	/**
	 * @brief Gets all spawns at specific coordinates.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 * @param z Z coordinate.
	 * @return List of spawns.
	 */
	SpawnList getSpawnList(int32_t x, int32_t y, int32_t z) {
		return getSpawnList(getTile(x, y, z));
	}

	// Returns true if the map has been saved
	// ie. it knows which file it should be saved to
	/**
	 * @brief Checks if the map is associated with a file.
	 * @return true if a filename is set.
	 */
	bool hasFile() const;

	/**
	 * @brief Gets the filename associated with the map.
	 * @return The filename string.
	 */
	std::string getFilename() const {
		return filename;
	}

	/**
	 * @brief Gets the logical name of the map.
	 * @return The map name.
	 */
	std::string getName() const {
		return name;
	}

	/**
	 * @brief Sets the logical name of the map.
	 * @param n The new name.
	 */
	void setName(const std::string& n) {
		name = n;
	}

	// Get map data
	/**
	 * @brief Gets the defined width of the map.
	 * @return Width in tiles.
	 */
	int getWidth() const {
		return width;
	}

	/**
	 * @brief Gets the defined height of the map.
	 * @return Height in tiles.
	 */
	int getHeight() const {
		return height;
	}

	/**
	 * @brief Gets the map description.
	 * @return Description string.
	 */
	std::string getMapDescription() const {
		return description;
	}

	/**
	 * @brief Gets the filename of the external house file.
	 * @return House filename.
	 */
	std::string getHouseFilename() const {
		return housefile;
	}

	/**
	 * @brief Gets the filename of the external spawn file.
	 * @return Spawn filename.
	 */
	std::string getSpawnFilename() const {
		return spawnfile;
	}

	// Set some map data
	/**
	 * @brief Sets the map width.
	 * @param new_width New width.
	 */
	void setWidth(int new_width);

	/**
	 * @brief Sets the map height.
	 * @param new_height New height.
	 */
	void setHeight(int new_height);

	/**
	 * @brief Sets the map description.
	 * @param new_description New description.
	 */
	void setMapDescription(const std::string& new_description);

	/**
	 * @brief Sets the external house filename.
	 * @param new_housefile New filename.
	 */
	void setHouseFilename(const std::string& new_housefile);

	/**
	 * @brief Sets the external spawn filename.
	 * @param new_spawnfile New filename.
	 */
	void setSpawnFilename(const std::string& new_spawnfile);

	/**
	 * @brief Marks the map as having a name (not unnamed).
	 */
	void flagAsNamed() {
		unnamed = false;
	}

	/**
	 * @brief Initializes the map as an empty map.
	 */
	void initializeEmpty();

protected:
	// Loads a map
	/**
	 * @brief Opens and loads a map file.
	 * @param identifier The path to the map file.
	 * @return true if successful.
	 */
	bool open(const std::string& identifier);

protected:
	void removeSpawnInternal(Tile* tile);

	std::vector<std::string> warnings;
	wxString error;

	std::string name; // The map name, NOT the same as filename
	std::string filename; // the maps filename
	std::string description; // The description of the map

	MapVersion mapVersion;

	// Map Width and Height - for info purposes
	uint16_t width, height;

	std::string spawnfile; // The maps spawnfile
	std::string housefile; // The housefile
	std::string waypointfile; // The waypoints file (stores extended waypoint information such as id, preferred icon and matching town)

public:
	Towns towns;
	Houses houses;
	Spawns spawns;

protected:
	bool has_changed; // If the map has changed
	bool unnamed; // If the map has yet to receive a name

	friend class IOMapOTBM;
	friend class IOMapOTMM;
	friend class Editor;
	friend class SelectionOperations;
	friend class MapProcessor;
	friend class EditorPersistence;

public:
	Waypoints waypoints;
};

/**
 * @brief Iterates over items on the map.
 * @tparam ForeachType The type of the callback function/functor.
 * @param map The map to iterate over.
 * @param foreach The callback to execute for each item.
 * @param selectedTiles If true, only iterates items on selected tiles.
 */
template <typename ForeachType>
inline void foreach_ItemOnMap(Map& map, ForeachType& foreach, bool selectedTiles) {
	long long done = 0;

	std::vector<Container*> containers;
	containers.reserve(32);

	std::ranges::for_each(map.tiles(), [&map, &foreach, &done, &containers, selectedTiles](auto& tile_loc) {
		++done;
		Tile* tile = tile_loc.get();
		if (selectedTiles && !tile->isSelected()) {
			return;
		}

		if (tile->ground) {
			foreach (map, tile, tile->ground.get(), done)
				;
		}

		for (const auto& item : tile->items) {
			containers.clear();
			Container* container = item->asContainer();
			foreach (map, tile, item.get(), done)
				;

			if (container) {
				containers.push_back(container);

				size_t index = 0;
				while (index < containers.size()) {
					container = containers[index++];

					auto& contents = container->getVector();
					for (const auto& i : contents) {
						Container* c = i->asContainer();
						foreach (map, tile, i.get(), done)
							;

						if (c) {
							containers.push_back(c);
						}
					}
				}
			}
		}
	});
}

/**
 * @brief Iterates over all tiles on the map.
 * @tparam ForeachType The type of the callback.
 * @param map The map to iterate.
 * @param foreach The callback to execute for each tile.
 */
template <typename ForeachType>
inline void foreach_TileOnMap(Map& map, ForeachType& foreach) {
	long long done = 0;
	std::ranges::for_each(map.tiles(), [&](auto& tile_loc) {
		foreach (map, tile_loc.get(), ++done)
			;
	});
}

/**
 * @brief Removes tiles from the map based on a condition.
 * @tparam RemoveIfType The type of the predicate.
 * @param map The map to modify.
 * @param remove_if The predicate determining removal.
 * @return The number of tiles removed.
 */
template <typename RemoveIfType>
inline long long remove_if_TileOnMap(Map& map, RemoveIfType& remove_if) {
	long long done = 0;
	long long removed = 0;
	long long total = map.getTileCount();

	std::ranges::for_each(map.tiles(), [&](auto& tile_loc) {
		Tile* tile = tile_loc.get();
		if (remove_if(map, tile, removed, done, total)) {
			map.setTile(tile->getPosition(), std::unique_ptr<Tile>());
			++removed;
		}
		++done;
	});

	return removed;
}

/**
 * @brief Removes items from the map based on a condition.
 * @tparam RemoveIfType The type of the predicate.
 * @param map The map to modify.
 * @param condition The predicate determining removal.
 * @param selectedOnly If true, only checks items on selected tiles.
 * @return The number of items removed.
 */
template <typename RemoveIfType>
inline int64_t RemoveItemOnMap(Map& map, RemoveIfType& condition, bool selectedOnly) {
	int64_t done = 0;
	int64_t removed = 0;

	std::ranges::for_each(map.tiles(), [&](auto& tile_loc) {
		++done;
		Tile* tile = tile_loc.get();
		if (selectedOnly && !tile->isSelected()) {
			return;
		}

		if (tile->ground) {
			if (condition(map, tile->ground.get(), removed, done)) {
				tile->ground.reset();
				++removed;
			}
		}

		// Use C++20's std::erase_if for a safer and more idiomatic way to remove elements.
		std::erase_if(tile->items, [&](const auto& item) {
			if (condition(map, item.get(), removed, done)) {
				++removed;
				return true;
			}
			return false;
		});
	});
	return removed;
}

#endif
