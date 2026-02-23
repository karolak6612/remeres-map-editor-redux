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

#include "app/main.h"

#include "brushes/brush.h"
#include "rendering/core/graphics.h"
#include "ui/gui.h"
#include "map/tile.h"
#include "game/complexitem.h"
#include "io/iomap.h"
#include "game/item.h"
#include "brushes/managers/brush_manager.h"
#include <unordered_map>

#include "brushes/ground/ground_brush.h"
#include "brushes/carpet/carpet_brush.h"
#include "brushes/table/table_brush.h"
#include "brushes/wall/wall_brush.h"
#include <array>
#include <string_view>

std::unique_ptr<Item> Item::Create(uint16_t _type, uint16_t _subtype /*= 0xFFFF*/) {
	if (_type == 0) {
		return nullptr;
	}
	std::unique_ptr<Item> newItem;

	const ItemType& it = g_items[_type];

	if (it.id != 0) {
		if (it.isDepot()) {
			newItem = std::make_unique<Depot>(_type);
		} else if (it.isContainer()) {
			newItem = std::make_unique<Container>(_type);
		} else if (it.isTeleport()) {
			newItem = std::make_unique<Teleport>(_type);
		} else if (it.isDoor()) {
			newItem = std::make_unique<Door>(_type);
		} else if (it.isPodium()) {
			newItem = std::make_unique<Podium>(_type);
		} else if (_subtype == 0xFFFF) {
			if (it.isFluidContainer()) {
				newItem = std::make_unique<Item>(_type, LIQUID_NONE);
			} else if (it.isSplash()) {
				newItem = std::make_unique<Item>(_type, LIQUID_WATER);
			} else if (it.charges > 0) {
				newItem = std::make_unique<Item>(_type, it.charges);
			} else {
				newItem = std::make_unique<Item>(_type, 1);
			}
		} else {
			newItem = std::make_unique<Item>(_type, _subtype);
		}
	} else {
		newItem = std::make_unique<Item>(_type, _subtype);
	}

	return newItem;
}

Item::Item(unsigned short _type, unsigned short _count) :
	id(_type),
	subtype(1),
	selected(false) {
	if (hasSubtype()) {
		subtype = _count;
	}
}

Item::~Item() {
	////
}

std::unique_ptr<Item> Item::deepCopy() const {
	std::unique_ptr<Item> copy = Create(id, subtype);
	if (copy) {
		copy->selected = selected;
		if (attributes) {
			copy->attributes = newd ItemAttributeMap(*attributes);
		}
	}
	return copy;
}

Item* transformItem(Item* old_item, uint16_t new_id, Tile* parent) {
	if (old_item == nullptr) {
		return nullptr;
	}

	const uint16_t old_id = old_item->getID();
	old_item->setID(new_id);
	// Through the magic of deepCopy, this will now be a pointer to an item of the correct type.
	std::unique_ptr<Item> new_item_ptr = old_item->deepCopy();
	Item* new_item = new_item_ptr.get();

	if (parent) {
		// Find the old item and remove it from the tile, insert this one instead!
		if (old_item == parent->ground.get()) {
			parent->ground = std::move(new_item_ptr);
			return new_item;
		}

		std::queue<Container*> containers;
		for (auto it = parent->items.begin(); it != parent->items.end(); ++it) {
			if (it->get() == old_item) {
				*it = std::move(new_item_ptr);
				return new_item;
			}

			Container* c = (*it)->asContainer();
			if (c) {
				containers.push(c);
			}
		}

		while (containers.size() != 0) {
			Container* container = containers.front();
			auto& v = container->getVector();
			for (auto it = v.begin(); it != v.end(); ++it) {
				Item* i = it->get();
				Container* c = i->asContainer();
				if (c) {
					containers.push(c);
				}

				if (i == old_item) {
					// Found it!
					*it = std::move(new_item_ptr);
					return new_item;
				}
			}
			containers.pop();
		}
	}

	// If we reached here, the item was not found in the parent or parent was null.
	// Restore the old ID to keep the object consistent.
	old_item->setID(old_id);
	return nullptr;
}

uint32_t Item::memsize() const {
	uint32_t mem = sizeof(*this);
	return mem;
}

void Item::setID(uint16_t newid) {
	id = newid;
}

void Item::setSubtype(uint16_t n) {
	subtype = n;
}

bool Item::hasSubtype() const {
	const ItemType& it = g_items[id];
	return (it.isFluidContainer() || it.isSplash() || isCharged() || it.stackable || it.charges != 0);
}

uint16_t Item::getSubtype() const {
	if (hasSubtype()) {
		return subtype;
	}
	return 0;
}

bool Item::hasProperty(enum ITEMPROPERTY prop) const {
	const ItemType& it = g_items[id];
	switch (prop) {
		case BLOCKSOLID:
			if (it.unpassable) {
				return true;
			}
			break;

		case MOVEABLE:
			if (it.moveable && getUniqueID() == 0) {
				return true;
			}
			break;
			/*
					case HASHEIGHT:
						if(it.height != 0 )
							return true;
						break;
			*/
		case BLOCKPROJECTILE:
			if (it.blockMissiles) {
				return true;
			}
			break;

		case BLOCKPATHFIND:
			if (it.blockPathfinder) {
				return true;
			}
			break;

		case HOOK_SOUTH:
			if (it.hookSouth) {
				return true;
			}
			break;

		case HOOK_EAST:
			if (it.hookEast) {
				return true;
			}
			break;

		case BLOCKINGANDNOTMOVEABLE:
			if (it.unpassable && (!it.moveable || getUniqueID() != 0)) {
				return true;
			}
			break;

		default:
			return false;
	}
	return false;
}

bool Item::isLocked() const {
	if (getActionID() != 0 || getUniqueID() != 0) {
		return true;
	}
	if (const Door* door = asDoor()) {
		return door->getDoorID() != 0;
	}
	return false;
}

std::pair<int, int> Item::getDrawOffset() const {
	ItemType& it = g_items[id];
	if (it.sprite != nullptr) {
		return it.sprite->getDrawOffset();
	}
	return std::make_pair(0, 0);
}

bool Item::hasLight() const {
	const ItemType& type = g_items.getItemType(id);
	if (type.sprite) {
		return type.sprite->hasLight();
	}
	return false;
}

SpriteLight Item::getLight() const {
	const ItemType& type = g_items.getItemType(id);
	if (type.sprite) {
		return type.sprite->getLight();
	}
	return SpriteLight { 0, 0 };
}

double Item::getWeight() const {
	ItemType& it = g_items[id];
	if (it.stackable) {
		return it.weight * std::max(1, (int)subtype);
	}

	return it.weight;
}

int Item::getHeight() const {
	// Simple implementation based on hasElevation flag
	const ItemType& it = g_items[id];
	// In Tibia, elevation is typically equal to a full tile step (sometimes variable).
	// RME's ItemType has 'hasElevation' bool.
	// If hasElevation is true, we assume standard elevation (8px usually in client draw logic terms, but logical step is 1).
	// For drawing: we want pixels. 8px is standard elevation step.
	return it.hasElevation ? 8 : 0;
}

void Item::setUniqueID(unsigned short n) {
	setAttribute(ATTR_UID, n);
}

void Item::setActionID(unsigned short n) {
	setAttribute(ATTR_AID, n);
}

void Item::setText(const std::string& str) {
	setAttribute(ATTR_TEXT, str);
}

void Item::setDescription(const std::string& str) {
	setAttribute(ATTR_DESC, str);
}

void Item::setTier(unsigned short n) {
	setAttribute(ATTR_TIER, n);
}

double Item::getWeight() {
	return const_cast<const Item*>(this)->getWeight();
}

bool Item::canHoldText() const {
	return isReadable() || canWriteText();
}

bool Item::canHoldDescription() const {
	return g_items[id].allowDistRead;
}

uint8_t Item::getMiniMapColor() const {
	GameSprite* spr = g_items[id].sprite;
	if (spr) {
		return spr->getMiniMapColor();
	}
	return 0;
}

GroundBrush* Item::getGroundBrush() const {
	ItemType& item_type = g_items.getItemType(id);
	if (item_type.isGroundTile() && item_type.brush && item_type.brush->is<GroundBrush>()) {
		return item_type.brush->as<GroundBrush>();
	}
	return nullptr;
}

TableBrush* Item::getTableBrush() const {
	ItemType& item_type = g_items.getItemType(id);
	if (item_type.isTable && item_type.brush && item_type.brush->is<TableBrush>()) {
		return item_type.brush->as<TableBrush>();
	}
	return nullptr;
}

CarpetBrush* Item::getCarpetBrush() const {
	ItemType& item_type = g_items.getItemType(id);
	if (item_type.isCarpet && item_type.brush && item_type.brush->is<CarpetBrush>()) {
		return item_type.brush->as<CarpetBrush>();
	}
	return nullptr;
}

DoorBrush* Item::getDoorBrush() const {
	ItemType& item_type = g_items.getItemType(id);
	if (!item_type.isWall || !item_type.isBrushDoor || !item_type.brush || !item_type.brush->is<WallBrush>()) {
		return nullptr;
	}

	DoorType door_type = item_type.brush->as<WallBrush>()->getDoorTypeFromID(id);
	DoorBrush* door_brush = nullptr;
	// Quite a horrible dependency on a global here, meh.
	switch (door_type) {
		case WALL_DOOR_NORMAL: {
			door_brush = g_brush_manager.normal_door_brush;
			break;
		}
		case WALL_DOOR_LOCKED: {
			door_brush = g_brush_manager.locked_door_brush;
			break;
		}
		case WALL_DOOR_QUEST: {
			door_brush = g_brush_manager.quest_door_brush;
			break;
		}
		case WALL_DOOR_MAGIC: {
			door_brush = g_brush_manager.magic_door_brush;
			break;
		}
		case WALL_DOOR_NORMAL_ALT: {
			door_brush = g_brush_manager.normal_door_alt_brush;
			break;
		}
		case WALL_ARCHWAY: {
			door_brush = g_brush_manager.archway_door_brush;
			break;
		}
		case WALL_WINDOW: {
			door_brush = g_brush_manager.window_door_brush;
			break;
		}
		case WALL_HATCH_WINDOW: {
			door_brush = g_brush_manager.hatch_door_brush;
			break;
		}
		default: {
			break;
		}
	}
	return door_brush;
}

WallBrush* Item::getWallBrush() const {
	ItemType& item_type = g_items.getItemType(id);
	if (item_type.isWall && item_type.brush && item_type.brush->is<WallBrush>()) {
		return item_type.brush->as<WallBrush>();
	}
	return nullptr;
}

BorderType Item::getWallAlignment() const {
	ItemType& it = g_items[id];
	if (!it.isWall) {
		return BORDER_NONE;
	}
	return it.border_alignment;
}

BorderType Item::getBorderAlignment() const {
	ItemType& it = g_items[id];
	return it.border_alignment;
}

// ============================================================================
// Static conversions

std::string_view Item::LiquidID2Name(uint16_t id) {
	static constexpr std::array<std::string_view, 44> liquid_names = {
		"None", // LIQUID_NONE (0)
		"Water", // LIQUID_WATER (1)
		"Blood", // LIQUID_BLOOD (2)
		"Beer", // LIQUID_BEER (3)
		"Slime", // LIQUID_SLIME (4)
		"Lemonade", // LIQUID_LEMONADE (5)
		"Milk", // LIQUID_MILK (6)
		"Manafluid", // LIQUID_MANAFLUID (7)
		"Ink", // LIQUID_INK (8)
		"Water", // LIQUID_WATER2 (9)
		"Lifefluid", // LIQUID_LIFEFLUID (10)
		"Oil", // LIQUID_OIL (11)
		"Slime", // LIQUID_SLIME2 (12)
		"Urine", // LIQUID_URINE (13)
		"Coconut Milk", // LIQUID_COCONUT_MILK (14)
		"Wine", // LIQUID_WINE (15)
		"Unknown", // 16
		"Unknown", // 17
		"Unknown", // 18
		"Mud", // LIQUID_MUD (19)
		"Unknown", // 20
		"Fruit Juice", // LIQUID_FRUIT_JUICE (21)
		"Unknown", // 22
		"Unknown", // 23
		"Unknown", // 24
		"Unknown", // 25
		"Lava", // LIQUID_LAVA (26)
		"Rum", // LIQUID_RUM (27)
		"Swamp", // LIQUID_SWAMP (28)
		"Unknown", // 29
		"Unknown", // 30
		"Unknown", // 31
		"Unknown", // 32
		"Unknown", // 33
		"Unknown", // 34
		"Tea", // LIQUID_TEA (35)
		"Unknown", // 36
		"Unknown", // 37
		"Unknown", // 38
		"Unknown", // 39
		"Unknown", // 40
		"Unknown", // 41
		"Unknown", // 42
		"Mead" // LIQUID_MEAD (43)
	};

	if (id < liquid_names.size()) {
		return liquid_names[id];
	}
	return "Unknown";
}

uint16_t Item::LiquidName2ID(std::string liquid) {
	to_lower_str(liquid);
	static const std::unordered_map<std::string, uint16_t> liquid_map = {
		{ "none", LIQUID_NONE },
		{ "water", LIQUID_WATER },
		{ "blood", LIQUID_BLOOD },
		{ "beer", LIQUID_BEER },
		{ "slime", LIQUID_SLIME },
		{ "lemonade", LIQUID_LEMONADE },
		{ "milk", LIQUID_MILK },
		{ "manafluid", LIQUID_MANAFLUID },
		{ "lifefluid", LIQUID_LIFEFLUID },
		{ "oil", LIQUID_OIL },
		{ "urine", LIQUID_URINE },
		{ "coconut milk", LIQUID_COCONUT_MILK },
		{ "wine", LIQUID_WINE },
		{ "mud", LIQUID_MUD },
		{ "fruit juice", LIQUID_FRUIT_JUICE },
		{ "lava", LIQUID_LAVA },
		{ "rum", LIQUID_RUM },
		{ "swamp", LIQUID_SWAMP },
		{ "ink", LIQUID_INK },
		{ "tea", LIQUID_TEA },
		{ "mead", LIQUID_MEAD }
	};

	auto it = liquid_map.find(liquid);
	if (it != liquid_map.end()) {
		return it->second;
	}
	return LIQUID_NONE;
}

// ============================================================================
// XML Saving & loading

std::unique_ptr<Item> Item::Create(pugi::xml_node xml) {
	pugi::xml_attribute attribute;

	int16_t id = 0;
	if ((attribute = xml.attribute("id"))) {
		id = attribute.as_ushort();
	}

	int16_t count = 1;
	if ((attribute = xml.attribute("count")) || (attribute = xml.attribute("subtype"))) {
		count = attribute.as_ushort();
	}

	return Create(id, count);
}
