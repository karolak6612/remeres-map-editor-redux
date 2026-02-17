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
 * @file item.h
 * @brief Base class for all game items.
 *
 * Defines the Item class, which represents individual objects in the game world,
 * such as weapons, furniture, and ground tiles.
 */

#ifndef RME_ITEM_H_
#define RME_ITEM_H_

#include "game/items.h"
#include <string_view>
#include "io/iomap_otbm.h"
// #include "io/iomap_otmm.h"
#include "game/item_attributes.h"
#include "brushes/doodad/doodad_brush.h"
#include "brushes/raw/raw_brush.h"

class Creature;
class Border;
class Tile;

struct SpriteLight;

/**
 * @brief Enumeration of specific item properties.
 *
 * Used to query boolean flags about an item's behavior.
 */
enum ITEMPROPERTY {
	BLOCKSOLID, ///< Blocks solid objects (creatures).
	HASHEIGHT, ///< Has elevation (elevation > 0).
	BLOCKPROJECTILE, ///< Blocks projectiles.
	BLOCKPATHFIND, ///< Blocks pathfinding.
	PROTECTIONZONE, ///< Acts as a protection zone.
	HOOK_SOUTH, ///< Has a south-facing hook.
	HOOK_EAST, ///< Has an east-facing hook.
	MOVEABLE, ///< Can be moved by players.
	BLOCKINGANDNOTMOVEABLE /* Blocks movement and cannot be moved. */
};

/**
 * @brief Types of liquid splashes.
 *
 * Defines the color and behavior of fluid containers and splashes.
 */
enum SplashType {
	LIQUID_NONE = 0,
	LIQUID_WATER = 1,
	LIQUID_BLOOD = 2,
	LIQUID_BEER = 3,
	LIQUID_SLIME = 4,
	LIQUID_LEMONADE = 5,
	LIQUID_MILK = 6,
	LIQUID_MANAFLUID = 7,
	LIQUID_INK = 8,
	LIQUID_WATER2 = 9,
	LIQUID_LIFEFLUID = 10,
	LIQUID_OIL = 11,
	LIQUID_SLIME2 = 12,
	LIQUID_URINE = 13,
	LIQUID_COCONUT_MILK = 14,
	LIQUID_WINE = 15,
	LIQUID_MUD = 19,
	LIQUID_FRUIT_JUICE = 21,
	LIQUID_LAVA = 26,
	LIQUID_RUM = 27,
	LIQUID_SWAMP = 28,
	LIQUID_TEA = 35,
	LIQUID_MEAD = 43,

	LIQUID_FIRST = LIQUID_WATER,
	LIQUID_LAST = LIQUID_MEAD
};

IMPLEMENT_INCREMENT_OP(SplashType)

/**
 * @brief Represents an item in the game world.
 *
 * Items are the main content of tiles. This class handles item attributes,
 * serialization, and property queries. It serves as a base class for
 * specialized items like containers or teleports.
 */
class Item : public ItemAttributes {
public:
	// Attribute keys
	inline static const std::string ATTR_UID = "uid";
	inline static const std::string ATTR_AID = "aid";
	inline static const std::string ATTR_TEXT = "text";
	inline static const std::string ATTR_DESC = "desc";
	inline static const std::string ATTR_TIER = "tier";

	// Factory member to create item of right type based on type
	/**
	 * @brief Factory method to create an item by type ID.
	 * @param _type The item type ID (server ID).
	 * @param _subtype The subtype (count, charges, fluid type).
	 * @return Unique pointer to the new Item.
	 */
	static std::unique_ptr<Item> Create(uint16_t _type, uint16_t _subtype = 0xFFFF);

	/**
	 * @brief Factory method to create an item from an XML node.
	 * @param node The XML node configuration.
	 * @return Unique pointer to the new Item.
	 */
	static std::unique_ptr<Item> Create(pugi::xml_node node);

	/**
	 * @brief Factory method to create an item from OTBM stream.
	 * @param maphandle Map I/O context.
	 * @param stream Binary stream to read from.
	 * @return Unique pointer to the new Item.
	 */
	static std::unique_ptr<Item> Create_OTBM(const IOMap& maphandle, BinaryNode* stream);
	// static std::unique_ptr<Item> Create_OTMM(const IOMap& maphandle, BinaryNode* stream);

public:
	// Constructor for items
	/**
	 * @brief Constructs an item with a specific type and count.
	 * @param _type The item type ID.
	 * @param _count The item subtype/count.
	 */
	Item(unsigned short _type, unsigned short _count);

	virtual ~Item();

	// Deep copy thingy
	/**
	 * @brief Creates a deep copy of this item.
	 * @return Unique pointer to the copy.
	 */
	virtual std::unique_ptr<Item> deepCopy() const;

	// Get memory footprint size
	/**
	 * @brief Calculates the memory usage of this item.
	 * @return Size in bytes.
	 */
	uint32_t memsize() const;

	virtual class Container* asContainer() {
		return nullptr;
	}
	virtual const class Container* asContainer() const {
		return nullptr;
	}
	virtual class Teleport* asTeleport() {
		return nullptr;
	}
	virtual const class Teleport* asTeleport() const {
		return nullptr;
	}
	virtual class TrashHolder* asTrashHolder() {
		return nullptr;
	}
	virtual const class TrashHolder* asTrashHolder() const {
		return nullptr;
	}
	virtual class Mailbox* asMailbox() {
		return nullptr;
	}
	virtual const class Mailbox* asMailbox() const {
		return nullptr;
	}
	virtual class Door* asDoor() {
		return nullptr;
	}
	virtual const class Door* asDoor() const {
		return nullptr;
	}
	virtual class MagicField* asMagicField() {
		return nullptr;
	}
	virtual const class MagicField* asMagicField() const {
		return nullptr;
	}

	// OTBM map interface
	// Serialize and unserialize (for save/load)
	// Used internally
	/**
	 * @brief Reads a single OTBM attribute.
	 * @param maphandle Map I/O context.
	 * @param attr Attribute ID.
	 * @param stream Data stream.
	 * @return true if handled.
	 */
	virtual bool readItemAttribute_OTBM(const IOMap& maphandle, OTBM_ItemAttribute attr, BinaryNode* stream);

	/**
	 * @brief Unserializes attributes from OTBM stream.
	 * @param maphandle Map I/O context.
	 * @param stream Data stream.
	 * @return true on success.
	 */
	virtual bool unserializeAttributes_OTBM(const IOMap& maphandle, BinaryNode* stream);

	/**
	 * @brief Unserializes an item node from OTBM.
	 * @param maphandle Map I/O context.
	 * @param node Binary node.
	 * @return true on success.
	 */
	virtual bool unserializeItemNode_OTBM(const IOMap& maphandle, BinaryNode* node);

	// Will return a node containing this item
	/**
	 * @brief Serializes this item to an OTBM node.
	 * @param maphandle Map I/O context.
	 * @param f File write handle.
	 * @return true on success.
	 */
	virtual bool serializeItemNode_OTBM(const IOMap& maphandle, NodeFileWriteHandle& f) const;
	// Will write this item to the stream supplied in the argument

	/**
	 * @brief Serializes item in compact format (without children).
	 * @param maphandle Map I/O context.
	 * @param f File write handle.
	 */
	virtual void serializeItemCompact_OTBM(const IOMap& maphandle, NodeFileWriteHandle& f) const;

	/**
	 * @brief Serializes item attributes.
	 * @param maphandle Map I/O context.
	 * @param f File write handle.
	 */
	virtual void serializeItemAttributes_OTBM(const IOMap& maphandle, NodeFileWriteHandle& f) const;

	// OTMM map interface
	/*
	// Serialize and unserialize (for save/load)
	// Used internally
	virtual bool readItemAttribute_OTMM(const IOMap& maphandle, OTMM_ItemAttribute attr, BinaryNode* stream);
	virtual bool unserializeAttributes_OTMM(const IOMap& maphandle, BinaryNode* stream);
	virtual bool unserializeItemNode_OTMM(const IOMap& maphandle, BinaryNode* node);

	// Will return a node containing this item
	virtual bool serializeItemNode_OTMM(const IOMap& maphandle, NodeFileWriteHandle& f) const;
	// Will write this item to the stream supplied in the argument
	virtual void serializeItemCompact_OTMM(const IOMap& maphandle, NodeFileWriteHandle& f) const;
	virtual void serializeItemAttributes_OTMM(const IOMap& maphandle, NodeFileWriteHandle& f) const;
	*/

	// Static conversions
	/**
	 * @brief Converts liquid ID to string name.
	 * @param id Liquid ID.
	 * @return Liquid name.
	 */
	static std::string LiquidID2Name(uint16_t id);

	/**
	 * @brief Converts liquid name to ID.
	 * @param id Liquid name string.
	 * @return Liquid ID.
	 */
	static uint16_t LiquidName2ID(std::string id);

	// IDs
	/**
	 * @brief Gets the item's server ID.
	 * @return The item ID.
	 */
	uint16_t getID() const {
		return id;
	}

	/**
	 * @brief Gets the item's client ID (for display).
	 * @return The client ID.
	 */
	uint16_t getClientID() const {
		return g_items[id].clientID;
	}
	// NOTE: This is very volatile, do NOT use this unless you know exactly what you're doing
	// which you probably don't so avoid it like the plague!
	/**
	 * @brief Sets the item ID directly.
	 * @warning This is dangerous and can break item state if not used carefully.
	 * @param id New item ID.
	 */
	void setID(uint16_t id);

	/**
	 * @brief Checks if the item type exists in the definitions.
	 * @return true if valid type.
	 */
	bool typeExists() const {
		return g_items.typeExists(id);
	}

	// Usual attributes
	virtual double getWeight() const;
	int getAttack() const {
		return g_items[id].attack;
	}
	int getArmor() const {
		return g_items[id].armor;
	}
	int getDefense() const {
		return g_items[id].defense;
	}
	uint16_t getSlotPosition() const {
		return g_items[id].slot_position;
	}
	uint8_t getWeaponType() const {
		return g_items[id].weapon_type;
	}
	uint8_t getClassification() const {
		return g_items[id].classification;
	} // 12.81

	// Item g_settings
	bool canHoldText() const;
	bool canHoldDescription() const;
	bool isReadable() const {
		return g_items[id].canReadText;
	}
	bool canWriteText() const {
		return g_items[id].canWriteText;
	}
	uint32_t getMaxWriteLength() const {
		return g_items[id].maxTextLen;
	}

	/**
	 * @brief Gets the generic brush associated with this item.
	 * @return Pointer to Brush.
	 */
	Brush* getBrush() const {
		return g_items[id].brush;
	}
	GroundBrush* getGroundBrush() const;
	WallBrush* getWallBrush() const;
	DoorBrush* getDoorBrush() const;
	TableBrush* getTableBrush() const;
	CarpetBrush* getCarpetBrush() const;
	Brush* getDoodadBrush() const {
		return g_items[id].doodad_brush;
	} // This is not necessarily a doodad brush
	RAWBrush* getRAWBrush() const {
		return g_items[id].raw_brush;
	}
	bool hasCollectionBrush() const {
		return g_items[id].collection_brush;
	}
	uint16_t getGroundEquivalent() const {
		return g_items[id].ground_equivalent;
	}
	uint16_t hasBorderEquivalent() const {
		return g_items[id].has_equivalent;
	}
	uint32_t getBorderGroup() const {
		return g_items[id].border_group;
	}

	// Drawing related
	/**
	 * @brief Gets the minimap color index.
	 * @return Color index.
	 */
	uint8_t getMiniMapColor() const;

	/**
	 * @brief Gets the height of the item in pixels/units.
	 * @return Height.
	 */
	int getHeight() const;

	/**
	 * @brief Gets drawing offset.
	 * @return (x, y) offset pair.
	 */
	std::pair<int, int> getDrawOffset() const;

	/**
	 * @brief Checks if the item emits light.
	 * @return true if it has a light source.
	 */
	bool hasLight() const;

	/**
	 * @brief Gets the light properties.
	 * @return SpriteLight struct.
	 */
	SpriteLight getLight() const;

	// Item types
	/**
	 * @brief Checks if the item has a specific property.
	 * @param prop The property to check.
	 * @return true if property is set.
	 */
	bool hasProperty(enum ITEMPROPERTY prop) const;

	/**
	 * @brief Checks if the item blocks movement.
	 * @return true if blocking.
	 */
	bool isBlocking() const {
		return g_items[id].unpassable;
	}

	/**
	 * @brief Checks if the item is stackable.
	 * @return true if stackable.
	 */
	bool isStackable() const {
		return g_items[id].stackable;
	}
	bool isClientCharged() const {
		return g_items[id].isClientCharged();
	}
	bool isExtraCharged() const {
		return g_items[id].isExtraCharged();
	}
	bool isCharged() const {
		return isClientCharged() || isExtraCharged();
	}
	bool isFluidContainer() const {
		return (g_items[id].isFluidContainer());
	}
	bool isAlwaysOnBottom() const {
		return g_items[id].alwaysOnBottom;
	}
	int getTopOrder() const {
		return g_items[id].alwaysOnTopOrder;
	}
	bool isGroundTile() const {
		return g_items[id].isGroundTile();
	}
	bool isSplash() const {
		return g_items[id].isSplash();
	}
	bool isMagicField() const {
		return g_items[id].isMagicField();
	}
	bool isTeleport() const {
		return g_items[id].isTeleport();
	}
	bool isNotMoveable() const {
		return !g_items[id].moveable;
	}
	bool isMoveable() const {
		return g_items[id].moveable;
	}
	bool isPickupable() const {
		return g_items[id].pickupable;
	}
	// bool isWeapon() const {return (g_items[id].weaponType != WEAPON_NONE && g_items[id].weaponType != WEAPON_AMMO);}
	// bool isUseable() const {return g_items[id].useable;}
	bool isHangable() const {
		return g_items[id].isHangable;
	}
	bool isRoteable() const {
		const ItemType& it = g_items[id];
		return it.rotable && it.rotateTo;
	}
	void doRotate() {
		if (isRoteable()) {
			id = g_items[id].rotateTo;
		}
	}
	bool hasCharges() const {
		return g_items[id].charges != 0;
	}
	bool isBorder() const {
		return g_items[id].isBorder;
	}
	bool isOptionalBorder() const {
		return g_items[id].isOptionalBorder;
	}
	bool isWall() const {
		return g_items[id].isWall;
	}
	bool isDoor() const {
		return g_items[id].isDoor();
	}
	bool isOpen() const {
		return g_items[id].isOpen;
	}
	bool isBrushDoor() const {
		return g_items[id].isBrushDoor;
	}
	bool isTable() const {
		return g_items[id].isTable;
	}
	bool isCarpet() const {
		return g_items[id].isCarpet;
	}
	bool isMetaItem() const {
		return g_items[id].isMetaItem();
	}

	// Logic for UI overlays
	/**
	 * @brief Checks if the item is locked (UI state).
	 * @return true if locked.
	 */
	virtual bool isLocked() const;

	// Slot-based Item Types
	bool isWeapon() const {
		uint8_t weaponType = g_items[id].weapon_type;
		return weaponType != WEAPON_NONE && weaponType != WEAPON_AMMO;
	}
	bool isAmmunition() const {
		return g_items[id].weapon_type == WEAPON_AMMO;
	}
	bool isWearableEquipment() const { // Determine if the item is wearable piece of armor
		uint16_t slotPosition = g_items[id].slot_position;
		return slotPosition & SLOTP_HEAD || slotPosition & SLOTP_NECKLACE ||
			// slotPosition & SLOTP_BACKPACK || // handled as container in properties window
			slotPosition & SLOTP_ARMOR || slotPosition & SLOTP_LEGS || slotPosition & SLOTP_FEET || slotPosition & SLOTP_RING || (slotPosition & SLOTP_AMMO && !isAmmunition()); // light sources that give stats
	}

	// Wall alignment (vertical, horizontal, pole, corner)
	BorderType getWallAlignment() const;
	// Border aligment (south, west etc.)
	BorderType getBorderAlignment() const;

	// Get the name!
	/**
	 * @brief Gets the item name.
	 * @return Name string view.
	 */
	std::string_view getName() const {
		return g_items[id].name;
	}

	/**
	 * @brief Gets full name including editor suffix.
	 * @return Full name string.
	 */
	const std::string getFullName() const {
		return g_items[id].name + g_items[id].editorsuffix;
	}

	// Selection
	/**
	 * @brief Checks if item is selected.
	 * @return true if selected.
	 */
	bool isSelected() const {
		return selected;
	}
	void select() {
		selected = true;
	}
	void deselect() {
		selected = false;
	}
	void toggleSelection() {
		selected = !selected;
	}

	// Item properties!
	/**
	 * @brief Checks if item has complex attributes requiring full save.
	 * @return true if complex.
	 */
	virtual bool isComplex() const {
		return attributes && attributes->size();
	} // If this item requires full save (not compact)

	// Weight
	bool hasWeight() {
		return isPickupable();
	}
	virtual double getWeight();

	// Subtype (count, fluid, charges)
	/**
	 * @brief Gets the item count or subtype value.
	 * @return Count/subtype.
	 */
	int getCount() const;

	/**
	 * @brief Gets the raw subtype value.
	 * @return Subtype.
	 */
	uint16_t getSubtype() const;

	/**
	 * @brief Sets the subtype value.
	 * @param n New subtype.
	 */
	void setSubtype(uint16_t n);

	/**
	 * @brief Checks if item uses subtype.
	 * @return true if applicable.
	 */
	bool hasSubtype() const;

	// Unique ID
	/**
	 * @brief Sets the unique ID attribute.
	 * @param n Unique ID.
	 */
	void setUniqueID(uint16_t n);

	/**
	 * @brief Gets the unique ID.
	 * @return Unique ID.
	 */
	uint16_t getUniqueID() const;

	// Action ID
	/**
	 * @brief Sets the action ID attribute.
	 * @param n Action ID.
	 */
	void setActionID(uint16_t n);

	/**
	 * @brief Gets the action ID.
	 * @return Action ID.
	 */
	uint16_t getActionID() const;

	// Tier (12.81)
	void setTier(uint16_t n);
	uint16_t getTier() const;

	// Text
	/**
	 * @brief Sets the text content of the item (e.g., sign or book).
	 * @param str Text content.
	 */
	void setText(const std::string& str);

	/**
	 * @brief Gets the text content.
	 * @return Text string view.
	 */
	std::string_view getText() const;

	// Description
	/**
	 * @brief Sets the description attribute.
	 * @param str Description text.
	 */
	void setDescription(const std::string& str);

	/**
	 * @brief Gets the description attribute.
	 * @return Description string view.
	 */
	std::string_view getDescription() const;

protected:
	uint16_t id; // the same id as in ItemType
	// Subtype is either fluid type, count, subtype or charges
	uint16_t subtype;
	bool selected;

private:
	Item& operator=(const Item& i); // Can't copy
	Item(const Item& i); // Can't copy-construct
	Item& operator==(const Item& i); // Can't compare
};

using ItemVector = std::vector<Item*>;
using ItemList = std::list<Item*>;

Item* transformItem(Item* old_item, uint16_t new_id, Tile* parent = nullptr);

inline int Item::getCount() const {
	if (isStackable() || isExtraCharged() || isClientCharged()) {
		return subtype;
	}
	return 1;
}

inline uint16_t Item::getUniqueID() const {
	const int32_t* a = getIntegerAttribute(ATTR_UID);
	if (a) {
		return *a;
	}
	return 0;
}

inline uint16_t Item::getActionID() const {
	const int32_t* a = getIntegerAttribute(ATTR_AID);
	if (a) {
		return *a;
	}
	return 0;
}

inline uint16_t Item::getTier() const {
	const int32_t* a = getIntegerAttribute(ATTR_TIER);
	if (a) {
		return *a;
	}
	return 0;
}

inline std::string_view Item::getText() const {
	const std::string* a = getStringAttribute(ATTR_TEXT);
	if (a) {
		return *a;
	}
	return {};
}

inline std::string_view Item::getDescription() const {
	const std::string* a = getStringAttribute(ATTR_DESC);
	if (a) {
		return *a;
	}
	return {};
}

#endif
