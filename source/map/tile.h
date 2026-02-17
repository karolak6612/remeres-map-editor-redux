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
 * @file tile.h
 * @brief Core tile data structure for the map editor.
 *
 * Defines the Tile class which represents a single map cell containing
 * ground, items, creatures, and spawn data. Tiles are the fundamental
 * building blocks of the map grid.
 */

#ifndef RME_TILE_H
#define RME_TILE_H

#include "map/position.h"
#include "game/item.h"

class TileLocation;
class GroundBrush;
class WallBrush;
class House;
class Map;
#include "app/rme_forward_declarations.h"
#include <unordered_set>
#include <memory>

/*
 * @brief Bitmask flags representing the state and properties of a tile.
 *
 * These flags combine both map-level properties (like Protection Zone) and
 * editor-internal states (like selection status).
 */
enum {
	TILESTATE_NONE = 0x0000,
	TILESTATE_PROTECTIONZONE = 0x0001, /* Tile is a protection zone (PZ). */
	TILESTATE_DEPRECATED = 0x0002, // Reserved
	TILESTATE_NOPVP = 0x0004, /* No PVP allowed on this tile. */
	TILESTATE_NOLOGOUT = 0x0008, /* Players cannot logout on this tile. */
	TILESTATE_PVPZONE = 0x0010, /* Tile is a PVP zone. */
	TILESTATE_REFRESH = 0x0020, /* Tile needs a visual refresh. */
	// Internal
	TILESTATE_SELECTED = 0x0001, /* Tile is currently selected in the editor. */
	TILESTATE_UNIQUE = 0x0002, /* Tile contains a unique item. */
	TILESTATE_BLOCKING = 0x0004, /* Tile blocks movement (calculated). */
	TILESTATE_OP_BORDER = 0x0008, /* If this is true, gravel will be placed on the tile! */
	TILESTATE_HAS_TABLE = 0x0010, /* Tile contains a table item. */
	TILESTATE_HAS_CARPET = 0x0020, /* Tile contains a carpet item. */
	TILESTATE_MODIFIED = 0x0040, /* Tile has been modified since last save. */
	TILESTATE_HOOK_SOUTH = 0x0080, /* Tile has a hook spot to the south. */
	TILESTATE_HOOK_EAST = 0x0100, /* Tile has a hook spot to the east. */
};

enum : uint8_t {
	INVALID_MINIMAP_COLOR = 0xFF
};

/*
 * @brief Represents a single cell in the map grid.
 *
 * A Tile holds all items, ground info, creatures, and metadata for one
 * (x, y, z) position. Tiles are owned by the BaseMap and accessed via
 * MapRegion quadtree lookups.
 *
 * @see BaseMap
 * @see MapRegion
 */
class Tile {
public: // Members
	TileLocation* location;
	std::unique_ptr<Item> ground;
	std::vector<std::unique_ptr<Item>> items;
	std::unique_ptr<Creature> creature;
	std::unique_ptr<Spawn> spawn;
	uint32_t house_id; // House id for this tile (pointer not safe)

public:
	/*
	 * @brief Constructs a Tile associated with a specific map location.
	 *
	 * Use this constructor when the tile is part of a map structure.
	 *
	 * @param location The memory location where this tile resides.
	 */
	Tile(TileLocation& location);

	/*
	 * @brief Constructs a temporary Tile with specific coordinates.
	 *
	 * Use this when the tile is only used internally by the editor (like in certain brushes)
	 * and not part of the main map structure.
	 *
	 * @param x X-coordinate.
	 * @param y Y-coordinate.
	 * @param z Z-coordinate.
	 */
	Tile(int x, int y, int z);

	~Tile();

	/*
	 * @brief Creates a deep copy of this tile associated with a new map.
	 *
	 * Copies all contents (ground, items, creatures) to a new Tile instance.
	 *
	 * @param map The map that will own the new tile.
	 * @return A unique pointer to the newly created tile copy.
	 */
	std::unique_ptr<Tile> deepCopy(BaseMap& map);

	/*
	 * @brief Sets the storage location of the tile.
	 *
	 * Stores state that remains between the tile being moved (like house exits).
	 *
	 * @param where Pointer to the TileLocation.
	 */
	void setLocation(TileLocation* where) {
		location = where;
	}

	/*
	 * @brief Gets the storage location of the tile.
	 * @return Pointer to the TileLocation.
	 */
	TileLocation* getLocation() {
		return location;
	}

	const TileLocation* getLocation() const {
		return location;
	}

	/*
	 * @brief Gets the absolute position of the tile in the world.
	 * @return The (x, y, z) Position object.
	 */
	Position getPosition();
	const Position getPosition() const;

	/*
	 * @brief Gets the X-coordinate of the tile.
	 * @return The X coordinate.
	 */
	int getX() const;

	/*
	 * @brief Gets the Y-coordinate of the tile.
	 * @return The Y coordinate.
	 */
	int getY() const;

	/*
	 * @brief Gets the Z-coordinate (floor) of the tile.
	 * @return The Z coordinate.
	 */
	int getZ() const;

public: // Functions
	/*
	 * @brief Merges the contents of another tile into this one.
	 *
	 * Moves items and properties from the source tile to this tile.
	 *
	 * @param other The source tile to merge from.
	 */
	void merge(Tile* other);

	/*
	 * @brief Compares the content (ground and items) of two tiles.
	 *
	 * Checks if both tiles have the same ground and identical item stacks.
	 *
	 * @param other The tile to compare with.
	 * @return true if contents are equal, false otherwise.
	 */
	bool isContentEqual(const Tile* other) const;

	/*
	 * @brief Checks if the tile has been modified since load/save.
	 * @return true if modified, false otherwise.
	 */
	bool isModified() const {
		return testFlags(statflags, TILESTATE_MODIFIED);
	}

	/*
	 * @brief Marks the tile as modified.
	 */
	void modify() {
		statflags |= TILESTATE_MODIFIED;
	}

	/*
	 * @brief Clears the modified flag.
	 */
	void unmodify() {
		statflags &= ~TILESTATE_MODIFIED;
	}

	/*
	 * @brief Calculates the memory footprint of the tile and its contents.
	 * @return Size in bytes.
	 */
	uint32_t memsize() const;

	/*
	 * @brief Checks if the tile has no items (ground doesn't count as an item here).
	 * @return true if the item stack is empty.
	 */
	bool empty() const {
		return size() == 0;
	}

	/*
	 * @brief Gets the number of items on the tile.
	 * @return Number of items in the stack.
	 */
	int size() const;

	/*
	 * @brief Checks if the tile blocks movement.
	 * @return true if blocking.
	 */
	bool isBlocking() const {
		return testFlags(statflags, TILESTATE_BLOCKING);
	}

	/*
	 * @brief Checks if the tile is a Protection Zone.
	 * @return true if PZ.
	 */
	bool isPZ() const {
		return testFlags(mapflags, TILESTATE_PROTECTIONZONE);
	}

	/*
	 * @brief Sets or unsets the Protection Zone flag.
	 * @param pz true to set, false to unset.
	 */
	void setPZ(bool pz) {
		if (pz) {
			mapflags |= TILESTATE_PROTECTIONZONE;
		} else {
			mapflags &= ~TILESTATE_PROTECTIONZONE;
		}
	}

	/*
	 * @brief Checks if any item on the tile has the specified property.
	 * @param prop The item property to check for.
	 * @return true if found.
	 */
	bool hasProperty(enum ITEMPROPERTY prop) const;

	/*
	 * @brief Gets the stack index of a specific item.
	 * @param item The item to search for.
	 * @return Index of the item, or -1 if not found.
	 */
	int getIndexOf(Item* item) const;

	/*
	 * @brief Gets the topmost item on the stack.
	 * @return Pointer to the top item, or nullptr if empty.
	 */
	Item* getTopItem() const;

	/*
	 * @brief Gets the item at a specific stack index.
	 * @param index The index to retrieve.
	 * @return Pointer to the item, or nullptr if index is invalid.
	 */
	Item* getItemAt(int index) const;

	/*
	 * @brief Adds an item to this tile's item stack.
	 *
	 * The item is inserted at the appropriate position based on its
	 * ordering priority (ground < borders < items < top items).
	 *
	 * @param item The item to add. Ownership is transferred to the tile.
	 */
	void addItem(std::unique_ptr<Item> item);

	/*
	 * @brief Selects the tile (and all its items) in the editor.
	 */
	void select();

	/*
	 * @brief Deselects the tile (and all its items).
	 */
	void deselect();

	/*
	 * @brief Selects only the ground layer of the tile.
	 *
	 * This selects borders too.
	 */
	void selectGround();

	/*
	 * @brief Deselects the ground layer.
	 */
	void deselectGround();

	/*
	 * @brief Checks if the tile is currently selected.
	 * @return true if selected.
	 */
	bool isSelected() const {
		return testFlags(statflags, TILESTATE_SELECTED);
	}

	/*
	 * @brief Checks if the tile contains a unique item.
	 * @return true if unique item present.
	 */
	bool hasUniqueItem() const {
		return testFlags(statflags, TILESTATE_UNIQUE);
	}

	/*
	 * @brief Removes and returns all selected items from the tile.
	 * @param ignoreTileSelected If true, ignores the tile's own selection state.
	 * @return Vector of unique pointers to the removed items.
	 */
	std::vector<std::unique_ptr<Item>> popSelectedItems(bool ignoreTileSelected = false);

	/*
	 * @brief Gets a list of currently selected items on the tile without removing them.
	 * @param unzoomed Context-dependent flag (TODO: clarify usage).
	 * @return Vector of raw pointers to selected items.
	 */
	ItemVector getSelectedItems(bool unzoomed = false);

	/*
	 * @brief Gets the topmost selected item.
	 * @return Pointer to the top selected item.
	 */
	Item* getTopSelectedItem();

	/*
	 * @brief Refreshes internal flags based on current contents.
	 *
	 * Updates blocking status, selection state, etc.
	 */
	void update();

	/*
	 * @brief Gets the minimap color index for this tile.
	 * @return The color index.
	 */
	uint8_t getMiniMapColor() const;

	/*
	 * @brief Checks if the tile has a ground layer.
	 * @return true if ground exists.
	 */
	bool hasGround() const {
		return ground != nullptr;
	}

	/*
	 * @brief Checks if the tile has border items.
	 * @return true if borders exist.
	 */
	bool hasBorders() const {
		return items.size() && items[0]->isBorder();
	}

	/*
	 * @brief Gets the ground brush associated with the tile's ground item.
	 * @return Pointer to the GroundBrush.
	 */
	GroundBrush* getGroundBrush() const;

	/*
	 * @brief Adds a border item to the tile.
	 *
	 * Added at the bottom of all items (above ground).
	 *
	 * @param item The border item to add.
	 */
	void addBorderItem(std::unique_ptr<Item> item);

	/*
	 * @brief Checks if the tile contains a table.
	 * @return true if table present.
	 */
	bool hasTable() const {
		return testFlags(statflags, TILESTATE_HAS_TABLE);
	}

	/*
	 * @brief Gets the table item if present.
	 * @return Pointer to the table item.
	 */
	Item* getTable() const;

	/*
	 * @brief Checks if the tile contains a carpet.
	 * @return true if carpet present.
	 */
	bool hasCarpet() const {
		return testFlags(statflags, TILESTATE_HAS_CARPET);
	}

	/*
	 * @brief Gets the carpet item if present.
	 * @return Pointer to the carpet item.
	 */
	Item* getCarpet() const;

	/*
	 * @brief Checks for a south-facing hook spot.
	 * @return true if hook spot exists.
	 */
	bool hasHookSouth() const {
		return testFlags(statflags, TILESTATE_HOOK_SOUTH);
	}

	/*
	 * @brief Checks for an east-facing hook spot.
	 * @return true if hook spot exists.
	 */
	bool hasHookEast() const {
		return testFlags(statflags, TILESTATE_HOOK_EAST);
	}

	/*
	 * @brief Checks if the tile has an optional border.
	 * @return true if optional border present.
	 */
	bool hasOptionalBorder() const {
		return testFlags(statflags, TILESTATE_OP_BORDER);
	}

	/*
	 * @brief Sets the optional border flag.
	 * @param b true to set, false to unset.
	 */
	void setOptionalBorder(bool b) {
		if (b) {
			statflags |= TILESTATE_OP_BORDER;
		} else {
			statflags &= ~TILESTATE_OP_BORDER;
		}
	}

	/*
	 * @brief Gets the first wall item on the tile.
	 * @return Pointer to the wall item.
	 */
	Item* getWall() const;

	/*
	 * @brief Checks if the tile contains a wall.
	 * @return true if wall present.
	 */
	bool hasWall() const;

	/*
	 * @brief Adds a wall item to the tile.
	 *
	 * Same as addItem, but performs an additional check to verify that it is a wall.
	 *
	 * @param item The wall item to add.
	 */
	void addWallItem(std::unique_ptr<Item> item);

	// Has to do with houses
	/*
	 * @brief Checks if this tile belongs to a house.
	 * @return true if house ID is non-zero.
	 */
	bool isHouseTile() const;

	/*
	 * @brief Gets the ID of the house this tile belongs to.
	 * @return The house ID.
	 */
	uint32_t getHouseID() const;

	/*
	 * @brief Sets the house ID for this tile.
	 * @param newHouseId The new house ID.
	 */
	void setHouseID(uint32_t newHouseId);

	/*
	 * @brief Marks this tile as an exit for a specific house.
	 * @param h The house.
	 */
	void addHouseExit(House* h);

	/*
	 * @brief Removes this tile as an exit for a specific house.
	 * @param h The house.
	 */
	void removeHouseExit(House* h);

	/*
	 * @brief Checks if this tile is a house exit.
	 * @return true if it is an exit.
	 */
	bool isHouseExit() const;

	/*
	 * @brief Checks if this tile is a town exit.
	 * @param map The map context.
	 * @return true if it is a town exit.
	 */
	bool isTownExit(Map& map) const;

	/*
	 * @brief Gets the list of house exits associated with this tile.
	 * @return Pointer to the HouseExitList.
	 */
	const HouseExitList* getHouseExits() const;
	HouseExitList* getHouseExits();

	/*
	 * @brief Checks if this tile is an exit for a specific house ID.
	 * @param exit The house ID.
	 * @return true if it is an exit for the given house.
	 */
	bool hasHouseExit(uint32_t exit) const;

	/*
	 * @brief Assigns this tile to a house structure.
	 * @param house The house object.
	 */
	void setHouse(House* house);

	// Mapflags (PZ, PVPZONE etc.)
	/*
	 * @brief Sets specific map flags.
	 * @param _flags Bitmask of flags to set.
	 */
	void setMapFlags(uint16_t _flags);

	/*
	 * @brief Unsets specific map flags.
	 * @param _flags Bitmask of flags to unset.
	 */
	void unsetMapFlags(uint16_t _flags);

	/*
	 * @brief Gets the current map flags.
	 * @return Bitmask of map flags.
	 */
	uint16_t getMapFlags() const;

	// Statflags (You really ought not to touch this)
	/*
	 * @brief Sets specific status flags (internal).
	 * @param _flags Bitmask of flags to set.
	 */
	void setStatFlags(uint16_t _flags);

	/*
	 * @brief Unsets specific status flags (internal).
	 * @param _flags Bitmask of flags to unset.
	 */
	void unsetStatFlags(uint16_t _flags);

	/*
	 * @brief Gets the current status flags.
	 * @return Bitmask of status flags.
	 */
	uint16_t getStatFlags() const;

protected:
	union {
		struct {
			uint16_t mapflags;
			uint16_t statflags;
		};
		uint32_t flags;
	};

private:
	uint8_t minimapColor;

	Tile(const Tile& tile); // No copy
	Tile& operator=(const Tile& i); // Can't copy
	Tile& operator==(const Tile& i); // Can't compare
};

/*
 * @brief Comparator for sorting tiles by position.
 * @param a First tile.
 * @param b Second tile.
 * @return true if a comes before b.
 */
bool tilePositionLessThan(const Tile* a, const Tile* b);

/*
 * @brief Comparator for sorting tiles by visual draw order.
 * @param a First tile.
 * @param b Second tile.
 * @return true if a should be drawn before b.
 */
bool tilePositionVisualLessThan(const Tile* a, const Tile* b);

using TileVector = std::vector<Tile*>;
using TileSet = std::vector<Tile*>;
using TileList = std::list<Tile*>;

inline bool Tile::hasWall() const {
	return getWall() != nullptr;
}

inline bool Tile::isHouseTile() const {
	return house_id != 0;
}

inline uint32_t Tile::getHouseID() const {
	return house_id;
}

inline void Tile::setMapFlags(uint16_t _flags) {
	mapflags = _flags | mapflags;
}

inline void Tile::unsetMapFlags(uint16_t _flags) {
	mapflags &= ~_flags;
}

inline uint16_t Tile::getMapFlags() const {
	return mapflags;
}

inline void Tile::setStatFlags(uint16_t _flags) {
	statflags = _flags | statflags;
}

inline void Tile::unsetStatFlags(uint16_t _flags) {
	statflags &= ~_flags;
}

inline uint16_t Tile::getStatFlags() const {
	return statflags;
}

#endif
