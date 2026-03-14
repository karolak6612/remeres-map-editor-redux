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

namespace TileOperations {
	void update(class Tile* tile);
}

class TileLocation;
class GroundBrush;
class WallBrush;
class House;
class Map;
#include "app/rme_forward_declarations.h"
#include <unordered_set>
#include <memory>
#include <vector>

// TileOperations functions are now in tile_operations.h

enum class TileMapFlags : uint16_t {
	NONE = 0x0000,
	PROTECTIONZONE = 0x0001,
	DEPRECATED = 0x0002, // Reserved
	NOPVP = 0x0004,
	NOLOGOUT = 0x0008,
	PVPZONE = 0x0010,
	REFRESH = 0x0020,
};

enum class TileStatFlags : uint16_t {
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

constexpr TileMapFlags operator|(TileMapFlags a, TileMapFlags b) { return static_cast<TileMapFlags>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b)); }
constexpr TileMapFlags operator&(TileMapFlags a, TileMapFlags b) { return static_cast<TileMapFlags>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b)); }
constexpr TileMapFlags operator^(TileMapFlags a, TileMapFlags b) { return static_cast<TileMapFlags>(static_cast<uint16_t>(a) ^ static_cast<uint16_t>(b)); }
constexpr TileMapFlags operator~(TileMapFlags a) { return static_cast<TileMapFlags>(~static_cast<uint16_t>(a)); }
inline TileMapFlags& operator|=(TileMapFlags& a, TileMapFlags b) { return a = a | b; }
inline TileMapFlags& operator&=(TileMapFlags& a, TileMapFlags b) { return a = a & b; }
inline TileMapFlags& operator^=(TileMapFlags& a, TileMapFlags b) { return a = a ^ b; }

constexpr TileStatFlags operator|(TileStatFlags a, TileStatFlags b) { return static_cast<TileStatFlags>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b)); }
constexpr TileStatFlags operator&(TileStatFlags a, TileStatFlags b) { return static_cast<TileStatFlags>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b)); }
constexpr TileStatFlags operator^(TileStatFlags a, TileStatFlags b) { return static_cast<TileStatFlags>(static_cast<uint16_t>(a) ^ static_cast<uint16_t>(b)); }
constexpr TileStatFlags operator~(TileStatFlags a) { return static_cast<TileStatFlags>(~static_cast<uint16_t>(a)); }
inline TileStatFlags& operator|=(TileStatFlags& a, TileStatFlags b) { return a = a | b; }
inline TileStatFlags& operator&=(TileStatFlags& a, TileStatFlags b) { return a = a & b; }
inline TileStatFlags& operator^=(TileStatFlags& a, TileStatFlags b) { return a = a ^ b; }

constexpr uint8_t INVALID_MINIMAP_COLOR = 0xFF;

class Tile {
public: // Members
	TileLocation* location;
	std::unique_ptr<Item> ground;
	std::vector<std::unique_ptr<Item>> items;
	std::unique_ptr<Creature> creature;
	std::unique_ptr<Spawn> spawn;
	uint32_t house_id; // House id for this tile (pointer not safe)
	TileMapFlags mapflags;
	TileStatFlags statflags;
	uint8_t minimapColor;

public:
	// ALWAYS use this constructor if the Tile is EVER going to be placed on a map
	Tile(TileLocation& location);
	// Use this when the tile is only used internally by the editor (like in certain brushes)
	Tile(int x, int y, int z);

	~Tile();

	Tile(const Tile&) = delete;
	Tile& operator=(const Tile&) = delete;

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
	// Compare the content (ground and items) of two tiles
	bool isContentEqual(const Tile* other) const;

	// Has tile been modified since the map was loaded/created?
	bool isModified() const {
		return (statflags & TileStatFlags::MODIFIED) != TileStatFlags::NONE;
	}
	void modify() {
		statflags |= TileStatFlags::MODIFIED;
	}
	void unmodify() {
		statflags &= ~TileStatFlags::MODIFIED;
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
		return (statflags & TileStatFlags::BLOCKING) != TileStatFlags::NONE;
	}

	// PZ
	bool isPZ() const {
		return (mapflags & TileMapFlags::PROTECTIONZONE) != TileMapFlags::NONE;
	}
	void setPZ(bool pz) {
		if (pz) {
			mapflags |= TileMapFlags::PROTECTIONZONE;
		} else {
			mapflags &= ~TileMapFlags::PROTECTIONZONE;
		}
	}

	bool hasProperty(enum ITEMPROPERTY prop) const;

	int getIndexOf(Item* item) const;
	Item* getTopItem() const; // Returns the topmost item, or nullptr if the tile is empty
	Item* getItemAt(int index) const;
	void addItem(std::unique_ptr<Item> item);

	bool isSelected() const {
		return (statflags & TileStatFlags::SELECTED) != TileStatFlags::NONE;
	}
	bool hasUniqueItem() const {
		return (statflags & TileStatFlags::UNIQUE) != TileStatFlags::NONE;
	}

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

	bool hasTable() const {
		return (statflags & TileStatFlags::HAS_TABLE) != TileStatFlags::NONE;
	}
	Item* getTable() const;

	bool hasCarpet() const {
		return (statflags & TileStatFlags::HAS_CARPET) != TileStatFlags::NONE;
	}
	Item* getCarpet() const;

	bool hasHookSouth() const {
		return (statflags & TileStatFlags::HOOK_SOUTH) != TileStatFlags::NONE;
	}

	bool hasHookEast() const {
		return (statflags & TileStatFlags::HOOK_EAST) != TileStatFlags::NONE;
	}

	bool hasLight() const {
		return (statflags & TileStatFlags::HAS_LIGHT) != TileStatFlags::NONE;
	}

	bool hasOptionalBorder() const {
		return (statflags & TileStatFlags::OP_BORDER) != TileStatFlags::NONE;
	}
	void setOptionalBorder(bool b) {
		if (b) {
			statflags |= TileStatFlags::OP_BORDER;
		} else {
			statflags &= ~TileStatFlags::OP_BORDER;
		}
	}

	// Get the (first) wall of this tile
	Item* getWall() const;
	bool hasWall() const;

	// Has to do with houses
	bool isHouseTile() const;
	uint32_t getHouseID() const;
	void setHouseID(uint32_t newHouseId);

	bool isHouseExit() const;
	bool isTownExit(Map& map) const;
	const HouseExitList* getHouseExits() const;
	HouseExitList* getHouseExits();
	bool hasHouseExit(uint32_t exit) const;
	void setHouse(House* house);

	// Mapflags (PZ, PVPZONE etc.)
	void setMapFlags(TileMapFlags _flags);
	void unsetMapFlags(TileMapFlags _flags);
	TileMapFlags getMapFlags() const;

	// Statflags (You really ought not to touch this)
	void setStatFlags(TileStatFlags _flags);
	void unsetStatFlags(TileStatFlags _flags);
	TileStatFlags getStatFlags() const;
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

inline void Tile::setMapFlags(TileMapFlags _flags) {
	mapflags |= _flags;
}

inline void Tile::unsetMapFlags(TileMapFlags _flags) {
	mapflags &= ~_flags;
}

inline TileMapFlags Tile::getMapFlags() const {
	return mapflags;
}

inline void Tile::setStatFlags(TileStatFlags _flags) {
	statflags |= _flags;
}

inline void Tile::unsetStatFlags(TileStatFlags _flags) {
	statflags &= ~_flags;
}

inline TileStatFlags Tile::getStatFlags() const {
	return statflags;
}

#endif
