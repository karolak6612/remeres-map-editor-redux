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

enum {
	TILESTATE_NONE = 0x0000,
	TILESTATE_PROTECTIONZONE = 0x0001,
	TILESTATE_DEPRECATED = 0x0002, // Reserved
	TILESTATE_NOPVP = 0x0004,
	TILESTATE_NOLOGOUT = 0x0008,
	TILESTATE_PVPZONE = 0x0010,
	TILESTATE_REFRESH = 0x0020,
	// Internal
	TILESTATE_SELECTED = 0x0001,
	TILESTATE_UNIQUE = 0x0002,
	TILESTATE_BLOCKING = 0x0004,
	TILESTATE_OP_BORDER = 0x0008, // If this is true, gravel will be placed on the tile!
	TILESTATE_HAS_TABLE = 0x0010,
	TILESTATE_HAS_CARPET = 0x0020,
	TILESTATE_MODIFIED = 0x0040,
	TILESTATE_HOOK_SOUTH = 0x0080,
	TILESTATE_HOOK_EAST = 0x0100,
};

enum : uint8_t {
	INVALID_MINIMAP_COLOR = 0xFF
};

class Tile {
public: // Members
	TileLocation* location;
	std::unique_ptr<Item> ground;
	std::vector<std::unique_ptr<Item>> items;
	std::unique_ptr<Creature> creature;
	std::unique_ptr<Spawn> spawn;
	uint32_t house_id; // House id for this tile (pointer not safe)

public:
	// ALWAYS use this constructor if the Tile is EVER going to be placed on a map
	Tile(TileLocation& location);
	// Use this when the tile is only used internally by the editor (like in certain brushes)
	Tile(int x, int y, int z);

	~Tile();

	// Argument is a the map to allocate the tile from
	[[nodiscard]] std::unique_ptr<Tile> deepCopy(BaseMap& map);

	// The location of the tile
	// Stores state that remains between the tile being moved (like house exits)
	void setLocation(TileLocation* where) {
		location = where;
	}
	[[nodiscard]] TileLocation* getLocation() {
		return location;
	}
	[[nodiscard]] const TileLocation* getLocation() const {
		return location;
	}

	// Position of the tile
	[[nodiscard]] Position getPosition();
	[[nodiscard]] const Position getPosition() const;
	[[nodiscard]] int getX() const;
	[[nodiscard]] int getY() const;
	[[nodiscard]] int getZ() const;

public: // Functions
	// Absorb the other tile into this tile
	void merge(Tile* other);

	// Compare the content (ground and items) of two tiles
	[[nodiscard]] bool isContentEqual(const Tile* other) const;

	// Has tile been modified since the map was loaded/created?
	[[nodiscard]] bool isModified() const {
		return testFlags(statflags, TILESTATE_MODIFIED);
	}
	void modify() {
		statflags |= TILESTATE_MODIFIED;
	}
	void unmodify() {
		statflags &= ~TILESTATE_MODIFIED;
	}

	// Get memory footprint size
	[[nodiscard]] uint32_t memsize() const;
	// Get number of items on the tile
	[[nodiscard]] bool empty() const {
		return size() == 0;
	}
	[[nodiscard]] int size() const;

	// Blocking?
	[[nodiscard]] bool isBlocking() const {
		return testFlags(statflags, TILESTATE_BLOCKING);
	}

	// PZ
	[[nodiscard]] bool isPZ() const {
		return testFlags(mapflags, TILESTATE_PROTECTIONZONE);
	}
	void setPZ(bool pz) {
		if (pz) {
			mapflags |= TILESTATE_PROTECTIONZONE;
		} else {
			mapflags &= ~TILESTATE_PROTECTIONZONE;
		}
	}

	[[nodiscard]] bool hasProperty(enum ITEMPROPERTY prop) const;

	[[nodiscard]] int getIndexOf(Item* item) const;
	[[nodiscard]] Item* getTopItem() const; // Returns the topmost item, or nullptr if the tile is empty
	[[nodiscard]] Item* getItemAt(int index) const;
	void addItem(std::unique_ptr<Item> item);

	void select();
	void deselect();
	// This selects borders too
	void selectGround();
	void deselectGround();

	[[nodiscard]] bool isSelected() const {
		return testFlags(statflags, TILESTATE_SELECTED);
	}
	[[nodiscard]] bool hasUniqueItem() const {
		return testFlags(statflags, TILESTATE_UNIQUE);
	}

	[[nodiscard]] std::vector<std::unique_ptr<Item>> popSelectedItems(bool ignoreTileSelected = false);
	[[nodiscard]] ItemVector getSelectedItems(bool unzoomed = false);
	[[nodiscard]] Item* getTopSelectedItem();

	// Refresh internal flags (such as selected etc.)
	void update();

	[[nodiscard]] uint8_t getMiniMapColor() const;

	// Does this tile have ground?
	[[nodiscard]] bool hasGround() const {
		return ground != nullptr;
	}
	[[nodiscard]] bool hasBorders() const {
		return items.size() && items[0]->isBorder();
	}

	// Get the border brush of this tile
	[[nodiscard]] GroundBrush* getGroundBrush() const;

	// Add a border item (added at the bottom of all items)
	void addBorderItem(std::unique_ptr<Item> item);

	[[nodiscard]] bool hasTable() const {
		return testFlags(statflags, TILESTATE_HAS_TABLE);
	}
	[[nodiscard]] Item* getTable() const;

	[[nodiscard]] bool hasCarpet() const {
		return testFlags(statflags, TILESTATE_HAS_CARPET);
	}
	[[nodiscard]] Item* getCarpet() const;

	[[nodiscard]] bool hasHookSouth() const {
		return testFlags(statflags, TILESTATE_HOOK_SOUTH);
	}

	[[nodiscard]] bool hasHookEast() const {
		return testFlags(statflags, TILESTATE_HOOK_EAST);
	}

	[[nodiscard]] bool hasOptionalBorder() const {
		return testFlags(statflags, TILESTATE_OP_BORDER);
	}
	void setOptionalBorder(bool b) {
		if (b) {
			statflags |= TILESTATE_OP_BORDER;
		} else {
			statflags &= ~TILESTATE_OP_BORDER;
		}
	}

	// Get the (first) wall of this tile
	[[nodiscard]] Item* getWall() const;
	[[nodiscard]] bool hasWall() const;
	// Add a wall item (same as just addItem, but an additional check to verify that it is a wall)
	void addWallItem(std::unique_ptr<Item> item);

	// Has to do with houses
	[[nodiscard]] bool isHouseTile() const;
	[[nodiscard]] uint32_t getHouseID() const;
	void setHouseID(uint32_t newHouseId);
	void addHouseExit(House* h);
	void removeHouseExit(House* h);
	[[nodiscard]] bool isHouseExit() const;
	[[nodiscard]] bool isTownExit(Map& map) const;
	[[nodiscard]] const HouseExitList* getHouseExits() const;
	[[nodiscard]] HouseExitList* getHouseExits();
	[[nodiscard]] bool hasHouseExit(uint32_t exit) const;
	void setHouse(House* house);

	// Mapflags (PZ, PVPZONE etc.)
	void setMapFlags(uint16_t _flags);
	void unsetMapFlags(uint16_t _flags);
	[[nodiscard]] uint16_t getMapFlags() const;

	// Statflags (You really ought not to touch this)
	void setStatFlags(uint16_t _flags);
	void unsetStatFlags(uint16_t _flags);
	[[nodiscard]] uint16_t getStatFlags() const;

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

bool tilePositionLessThan(const Tile* a, const Tile* b);
// This sorts them by draw order
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
