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

enum class TileState : uint16_t {
	NONE = 0x0000,
	PROTECTIONZONE = 0x0001,
	DEPRECATED = 0x0002, // Reserved
	NOPVP = 0x0004,
	NOLOGOUT = 0x0008,
	PVPZONE = 0x0010,
	REFRESH = 0x0020,
};

enum class TileInternalState : uint16_t {
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
	uint16_t mapflags;
	uint16_t statflags;
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
	[[nodiscard]] Position getPosition();
	[[nodiscard]] const Position getPosition() const;
	[[nodiscard]] int getX() const;
	[[nodiscard]] int getY() const;
	[[nodiscard]] int getZ() const;

public: // Functions
	// Compare the content (ground and items) of two tiles
	[[nodiscard]] bool isContentEqual(const Tile* other) const;

	// Has tile been modified since the map was loaded/created?
	[[nodiscard]] bool isModified() const {
		return testFlags(statflags, static_cast<uint16_t>(TileInternalState::MODIFIED));
	}
	void modify() {
		statflags |= static_cast<uint16_t>(TileInternalState::MODIFIED);
	}
	void unmodify() {
		statflags &= ~static_cast<uint16_t>(TileInternalState::MODIFIED);
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
		return testFlags(statflags, static_cast<uint16_t>(TileInternalState::BLOCKING));
	}

	// PZ
	[[nodiscard]] bool isPZ() const {
		return testFlags(mapflags, static_cast<uint16_t>(TileState::PROTECTIONZONE));
	}
	void setPZ(bool pz) {
		if (pz) {
			mapflags |= static_cast<uint16_t>(TileState::PROTECTIONZONE);
		} else {
			mapflags &= ~static_cast<uint16_t>(TileState::PROTECTIONZONE);
		}
	}

	[[nodiscard]] bool hasProperty(enum ITEMPROPERTY prop) const;

	[[nodiscard]] int getIndexOf(Item* item) const;
	[[nodiscard]] Item* getTopItem() const; // Returns the topmost item, or nullptr if the tile is empty
	[[nodiscard]] Item* getItemAt(int index) const;
	void addItem(std::unique_ptr<Item> item);

	[[nodiscard]] bool isSelected() const {
		return testFlags(statflags, static_cast<uint16_t>(TileInternalState::SELECTED));
	}
	[[nodiscard]] bool hasUniqueItem() const {
		return testFlags(statflags, static_cast<uint16_t>(TileInternalState::UNIQUE));
	}

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

	[[nodiscard]] bool hasTable() const {
		return testFlags(statflags, static_cast<uint16_t>(TileInternalState::HAS_TABLE));
	}
	[[nodiscard]] Item* getTable() const;

	[[nodiscard]] bool hasCarpet() const {
		return testFlags(statflags, static_cast<uint16_t>(TileInternalState::HAS_CARPET));
	}
	[[nodiscard]] Item* getCarpet() const;

	[[nodiscard]] bool hasHookSouth() const {
		return testFlags(statflags, static_cast<uint16_t>(TileInternalState::HOOK_SOUTH));
	}

	[[nodiscard]] bool hasHookEast() const {
		return testFlags(statflags, static_cast<uint16_t>(TileInternalState::HOOK_EAST));
	}

	[[nodiscard]] bool hasLight() const {
		return testFlags(statflags, static_cast<uint16_t>(TileInternalState::HAS_LIGHT));
	}

	[[nodiscard]] bool hasOptionalBorder() const {
		return testFlags(statflags, static_cast<uint16_t>(TileInternalState::OP_BORDER));
	}
	void setOptionalBorder(bool b) {
		if (b) {
			statflags |= static_cast<uint16_t>(TileInternalState::OP_BORDER);
		} else {
			statflags &= ~static_cast<uint16_t>(TileInternalState::OP_BORDER);
		}
	}

	// Get the (first) wall of this tile
	[[nodiscard]] Item* getWall() const;
	[[nodiscard]] bool hasWall() const;

	// Has to do with houses
	[[nodiscard]] bool isHouseTile() const;
	[[nodiscard]] uint32_t getHouseID() const;
	void setHouseID(uint32_t newHouseId);

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
