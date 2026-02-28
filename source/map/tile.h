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

enum class TileMapState : uint16_t {
	NONE = 0x0000,
	PROTECTIONZONE = 0x0001,
	DEPRECATED = 0x0002, // Reserved
	NOPVP = 0x0004,
	NOLOGOUT = 0x0008,
	PVPZONE = 0x0010,
	REFRESH = 0x0020,
};

enum class TileInternalState : uint16_t {
	NONE = 0x0000,
	SELECTED = 0x0001,
	UNIQUE = 0x0002,
	BLOCKING = 0x0004,
	OP_BORDER = 0x0008, // If this is true, gravel will be placed on the tile!
	HAS_TABLE = 0x0010,
	HAS_CARPET = 0x0020,
	MODIFIED = 0x0040,
	HOOK_SOUTH = 0x0080,
	HOOK_EAST = 0x0100,
	HAS_LIGHT = 0x0200,
};

inline constexpr TileMapState operator|(TileMapState a, TileMapState b) {
	return static_cast<TileMapState>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

inline constexpr TileMapState operator&(TileMapState a, TileMapState b) {
	return static_cast<TileMapState>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

inline constexpr TileMapState operator~(TileMapState a) {
	return static_cast<TileMapState>(~static_cast<uint16_t>(a));
}

inline TileMapState& operator|=(TileMapState& a, TileMapState b) {
	a = a | b;
	return a;
}

inline TileMapState& operator&=(TileMapState& a, TileMapState b) {
	a = a & b;
	return a;
}

inline constexpr TileInternalState operator|(TileInternalState a, TileInternalState b) {
	return static_cast<TileInternalState>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

inline constexpr TileInternalState operator&(TileInternalState a, TileInternalState b) {
	return static_cast<TileInternalState>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

inline constexpr TileInternalState operator~(TileInternalState a) {
	return static_cast<TileInternalState>(~static_cast<uint16_t>(a));
}

inline TileInternalState& operator|=(TileInternalState& a, TileInternalState b) {
	a = a | b;
	return a;
}

inline TileInternalState& operator&=(TileInternalState& a, TileInternalState b) {
	a = a & b;
	return a;
}

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

	Tile(const Tile&) = delete;
	Tile& operator=(const Tile&) = delete;

	// Argument is a the map to allocate the tile from
	std::unique_ptr<Tile> deepCopy(BaseMap& map);

	// The location of the tile
	// Stores state that remains between the tile being moved (like house exits)
	void setLocation(TileLocation* where) {
		location = where;
	}
	TileLocation* getLocation() {
		return location;
	}
	const TileLocation* getLocation() const {
		return location;
	}

	// Position of the tile
	Position getPosition();
	const Position getPosition() const;
	int getX() const;
	int getY() const;
	int getZ() const;

public: // Functions
	static bool positionLessThan(const Tile* a, const Tile* b);
	static bool positionVisualLessThan(const Tile* a, const Tile* b);
	// Absorb the other tile into this tile
	void merge(Tile* other);

	// Compare the content (ground and items) of two tiles
	bool isContentEqual(const Tile* other) const;

	// Has tile been modified since the map was loaded/created?
	bool isModified() const {
		return (statflags & TileInternalState::MODIFIED) != TileInternalState::NONE;
	}
	void modify() {
		statflags |= TileInternalState::MODIFIED;
	}
	void unmodify() {
		statflags &= ~TileInternalState::MODIFIED;
	}

	// Get memory footprint size
	uint32_t memsize() const;
	// Get number of items on the tile
	bool empty() const {
		return size() == 0;
	}
	int size() const;

	// Blocking?
	bool isBlocking() const {
		return (statflags & TileInternalState::BLOCKING) != TileInternalState::NONE;
	}

	// PZ
	bool isPZ() const {
		return (mapflags & TileMapState::PROTECTIONZONE) != TileMapState::NONE;
	}
	void setPZ(bool pz) {
		if (pz) {
			mapflags |= TileMapState::PROTECTIONZONE;
		} else {
			mapflags &= ~TileMapState::PROTECTIONZONE;
		}
	}

	bool hasProperty(enum ITEMPROPERTY prop) const;

	int getIndexOf(Item* item) const;
	Item* getTopItem() const; // Returns the topmost item, or nullptr if the tile is empty
	Item* getItemAt(int index) const;
	void addItem(std::unique_ptr<Item> item);

	void select();
	void deselect();
	// This selects borders too
	void selectGround();
	void deselectGround();

	bool isSelected() const {
		return (statflags & TileInternalState::SELECTED) != TileInternalState::NONE;
	}
	bool hasUniqueItem() const {
		return (statflags & TileInternalState::UNIQUE) != TileInternalState::NONE;
	}

	std::vector<std::unique_ptr<Item>> popSelectedItems(bool ignoreTileSelected = false);
	ItemVector getSelectedItems(bool unzoomed = false);
	Item* getTopSelectedItem();

	// Refresh internal flags (such as selected etc.)
	void update();

	uint8_t getMiniMapColor() const;

	// Does this tile have ground?
	bool hasGround() const {
		return ground != nullptr;
	}
	bool hasBorders() const {
		return items.size() && items[0]->isBorder();
	}

	// Get the border brush of this tile
	GroundBrush* getGroundBrush() const;

	// Add a border item (added at the bottom of all items)
	void addBorderItem(std::unique_ptr<Item> item);

	bool hasTable() const {
		return (statflags & TileInternalState::HAS_TABLE) != TileInternalState::NONE;
	}
	Item* getTable() const;

	bool hasCarpet() const {
		return (statflags & TileInternalState::HAS_CARPET) != TileInternalState::NONE;
	}
	Item* getCarpet() const;

	bool hasHookSouth() const {
		return (statflags & TileInternalState::HOOK_SOUTH) != TileInternalState::NONE;
	}

	bool hasHookEast() const {
		return (statflags & TileInternalState::HOOK_EAST) != TileInternalState::NONE;
	}

	bool hasLight() const {
		return (statflags & TileInternalState::HAS_LIGHT) != TileInternalState::NONE;
	}

	bool hasOptionalBorder() const {
		return (statflags & TileInternalState::OP_BORDER) != TileInternalState::NONE;
	}
	void setOptionalBorder(bool b) {
		if (b) {
			statflags |= TileInternalState::OP_BORDER;
		} else {
			statflags &= ~TileInternalState::OP_BORDER;
		}
	}

	// Get the (first) wall of this tile
	Item* getWall() const;
	bool hasWall() const;
	// Add a wall item (same as just addItem, but an additional check to verify that it is a wall)
	void addWallItem(std::unique_ptr<Item> item);

	// Has to do with houses
	bool isHouseTile() const;
	uint32_t getHouseID() const;
	void setHouseID(uint32_t newHouseId);
	void addHouseExit(House* h);
	void removeHouseExit(House* h);
	bool isHouseExit() const;
	bool isTownExit(Map& map) const;
	const HouseExitList* getHouseExits() const;
	HouseExitList* getHouseExits();
	bool hasHouseExit(uint32_t exit) const;
	void setHouse(House* house);

	// Mapflags (PZ, PVPZONE etc.)
	void setMapFlags(TileMapState _flags);
	void unsetMapFlags(TileMapState _flags);
	TileMapState getMapFlags() const;

	// Statflags (You really ought not to touch this)
	void setStatFlags(TileInternalState _flags);
	void unsetStatFlags(TileInternalState _flags);
	TileInternalState getStatFlags() const;

protected:
	TileMapState mapflags;
	TileInternalState statflags;

private:
	uint8_t minimapColor;
};





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

inline void Tile::setMapFlags(TileMapState _flags) {
	mapflags = _flags | mapflags;
}

inline void Tile::unsetMapFlags(TileMapState _flags) {
	mapflags &= ~_flags;
}

inline TileMapState Tile::getMapFlags() const {
	return mapflags;
}

inline void Tile::setStatFlags(TileInternalState _flags) {
	statflags = _flags | statflags;
}

inline void Tile::unsetStatFlags(TileInternalState _flags) {
	statflags &= ~_flags;
}

inline TileInternalState Tile::getStatFlags() const {
	return statflags;
}

#endif
