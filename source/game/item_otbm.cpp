#include "game/item.h"
#include "game/complexitem.h"
#include "game/creature.h"
#include "game/items.h"
#include "io/iomap_otbm.h"
#include "map/map.h"
#include "brushes/wall/wall_brush.h"

// ============================================================================
// Item

Item* Item::Create_OTBM(const IOMap& maphandle, BinaryNode* stream) {
	uint16_t _id;
	if (!stream->getU16(_id)) {
		return nullptr;
	}

	uint8_t _count = 0;

	const ItemType& iType = g_items[_id];
	if (maphandle.version.otbm == MAP_OTBM_1) {
		if (iType.stackable || iType.isSplash() || iType.isFluidContainer()) {
			stream->getU8(_count);
		}
	}
	return Item::Create(_id, _count);
}

bool Item::readItemAttribute_OTBM(const IOMap& maphandle, OTBM_ItemAttribute attr, BinaryNode* stream) {
	switch (attr) {
		case OTBM_ATTR_COUNT: {
			uint8_t subtype;
			if (!stream->getU8(subtype)) {
				return false;
			}
			setSubtype(subtype);
			break;
		}
		case OTBM_ATTR_ACTION_ID: {
			uint16_t aid;
			if (!stream->getU16(aid)) {
				return false;
			}
			setActionID(aid);
			break;
		}
		case OTBM_ATTR_UNIQUE_ID: {
			uint16_t uid;
			if (!stream->getU16(uid)) {
				return false;
			}
			setUniqueID(uid);
			break;
		}
		case OTBM_ATTR_CHARGES: {
			uint16_t charges;
			if (!stream->getU16(charges)) {
				return false;
			}
			setSubtype(charges);
			break;
		}
		case OTBM_ATTR_TEXT: {
			std::string text;
			if (!stream->getString(text)) {
				return false;
			}
			setText(text);
			break;
		}
		case OTBM_ATTR_DESC: {
			std::string text;
			if (!stream->getString(text)) {
				return false;
			}
			setDescription(text);
			break;
		}
		case OTBM_ATTR_RUNE_CHARGES: {
			uint8_t subtype;
			if (!stream->getU8(subtype)) {
				return false;
			}
			setSubtype(subtype);
			break;
		}
		case OTBM_ATTR_TIER: {
			uint8_t tier;
			if (!stream->getU8(tier)) {
				return false;
			}
			setTier(static_cast<uint16_t>(tier));
			break;
		}

		// The following *should* be handled in the derived classes
		// However, we still need to handle them here since otherwise things
		// will break horribly
		case OTBM_ATTR_DEPOT_ID:
			return stream->skip(2);
		case OTBM_ATTR_HOUSEDOORID:
			return stream->skip(1);
		case OTBM_ATTR_TELE_DEST:
			return stream->skip(5);
		case OTBM_ATTR_PODIUMOUTFIT:
			return stream->skip(15);
		default:
			return false;
	}
	return true;
}

bool Item::unserializeAttributes_OTBM(const IOMap& maphandle, BinaryNode* stream) {
	uint8_t attribute;
	while (stream->getU8(attribute)) {
		if (attribute == OTBM_ATTR_ATTRIBUTE_MAP) {
			if (!ItemAttributes::unserializeAttributeMap(maphandle, stream)) {
				return false;
			}
		} else if (!readItemAttribute_OTBM(maphandle, static_cast<OTBM_ItemAttribute>(attribute), stream)) {
			return false;
		}
	}
	return true;
}

bool Item::unserializeItemNode_OTBM(const IOMap& maphandle, BinaryNode* node) {
	return unserializeAttributes_OTBM(maphandle, node);
}

void Item::serializeItemAttributes_OTBM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	if (maphandle.version.otbm >= MAP_OTBM_2) {
		const ItemType& iType = g_items[id];
		if (iType.stackable || iType.isSplash() || iType.isFluidContainer()) {
			stream.addU8(OTBM_ATTR_COUNT);
			stream.addU8(getSubtype());
		}
	}

	if (maphandle.version.otbm >= MAP_OTBM_4) {
		if (attributes && !attributes->empty()) {
			stream.addU8(OTBM_ATTR_ATTRIBUTE_MAP);
			serializeAttributeMap(maphandle, stream);
		}
	} else {
		if (g_items.MinorVersion >= CLIENT_VERSION_820 && isCharged()) {
			stream.addU8(OTBM_ATTR_CHARGES);
			stream.addU16(getSubtype());
		}

		uint16_t actionId = getActionID();
		if (actionId > 0) {
			stream.addU8(OTBM_ATTR_ACTION_ID);
			stream.addU16(actionId);
		}

		uint16_t uniqueId = getUniqueID();
		if (uniqueId > 0) {
			stream.addU8(OTBM_ATTR_UNIQUE_ID);
			stream.addU16(uniqueId);
		}

		std::string text(getText());
		if (!text.empty()) {
			stream.addU8(OTBM_ATTR_TEXT);
			stream.addString(text);
		}

		std::string description(getDescription());
		if (!description.empty()) {
			stream.addU8(OTBM_ATTR_DESC);
			stream.addString(description);
		}

		uint16_t tier = getTier();
		if (tier > 0) {
			stream.addU8(OTBM_ATTR_TIER);
			stream.addU8(static_cast<uint8_t>(tier));
		}
	}
}

void Item::serializeItemCompact_OTBM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	stream.addU16(id);
}

bool Item::serializeItemNode_OTBM(const IOMap& maphandle, NodeFileWriteHandle& file) const {
	file.addNode(OTBM_ITEM);
	file.addU16(id);
	if (maphandle.version.otbm == MAP_OTBM_1) {
		const ItemType& iType = g_items[id];
		if (iType.stackable || iType.isSplash() || iType.isFluidContainer()) {
			file.addU8(getSubtype());
		}
	}
	serializeItemAttributes_OTBM(maphandle, file);
	file.endNode();
	return true;
}

// ============================================================================
// Teleport

bool Teleport::readItemAttribute_OTBM(const IOMap& maphandle, OTBM_ItemAttribute attribute, BinaryNode* stream) {
	if (OTBM_ATTR_TELE_DEST == attribute) {
		uint16_t x, y;
		uint8_t z;
		if (!stream->getU16(x) || !stream->getU16(y) || !stream->getU8(z)) {
			return false;
		}
		destination = Position(x, y, z);
		return true;
	} else {
		return Item::readItemAttribute_OTBM(maphandle, attribute, stream);
	}
}

void Teleport::serializeItemAttributes_OTBM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	Item::serializeItemAttributes_OTBM(maphandle, stream);

	stream.addByte(OTBM_ATTR_TELE_DEST);
	stream.addU16(destination.x);
	stream.addU16(destination.y);
	stream.addU8(destination.z);
}

// ============================================================================
// Door

bool Door::readItemAttribute_OTBM(const IOMap& maphandle, OTBM_ItemAttribute attribute, BinaryNode* stream) {
	if (OTBM_ATTR_HOUSEDOORID == attribute) {
		uint8_t id = 0;
		if (!stream->getU8(id)) {
			return false;
		}
		doorId = id;
		return true;
	} else {
		return Item::readItemAttribute_OTBM(maphandle, attribute, stream);
	}
}

void Door::serializeItemAttributes_OTBM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	Item::serializeItemAttributes_OTBM(maphandle, stream);
	if (doorId) {
		stream.addByte(OTBM_ATTR_HOUSEDOORID);
		stream.addU8(doorId);
	}
}

// ============================================================================
// Depots

bool Depot::readItemAttribute_OTBM(const IOMap& maphandle, OTBM_ItemAttribute attribute, BinaryNode* stream) {
	if (OTBM_ATTR_DEPOT_ID == attribute) {
		uint16_t id = 0;
		if (!stream->getU16(id)) {
			return false;
		}
		depotId = id;
		return true;
	} else {
		return Item::readItemAttribute_OTBM(maphandle, attribute, stream);
	}
}

void Depot::serializeItemAttributes_OTBM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	Item::serializeItemAttributes_OTBM(maphandle, stream);
	if (depotId) {
		stream.addByte(OTBM_ATTR_DEPOT_ID);
		stream.addU16(depotId);
	}
}

// ============================================================================
// Container

bool Container::unserializeItemNode_OTBM(const IOMap& maphandle, BinaryNode* node) {
	if (!Item::unserializeAttributes_OTBM(maphandle, node)) {
		return false;
	}

	BinaryNode* child = node->getChild();
	if (child) {
		do {
			uint8_t type;
			if (!child->getByte(type)) {
				return false;
			}

			if (type != OTBM_ITEM) {
				return false;
			}

			Item* item = Item::Create_OTBM(maphandle, child);
			if (!item) {
				return false;
			}

			if (!item->unserializeItemNode_OTBM(maphandle, child)) {
				delete item;
				return false;
			}

			contents.push_back(item);
		} while (child->advance());
	}
	return true;
}

bool Container::serializeItemNode_OTBM(const IOMap& maphandle, NodeFileWriteHandle& file) const {
	file.addNode(OTBM_ITEM);
	file.addU16(id);
	if (maphandle.version.otbm == MAP_OTBM_1) {
		// In the ludicrous event that an item is a container AND stackable, we have to do this. :p
		const ItemType& iType = g_items[id];
		if (iType.stackable || iType.isSplash() || iType.isFluidContainer()) {
			file.addU8(getSubtype());
		}
	}

	serializeItemAttributes_OTBM(maphandle, file);
	for (Item* item : contents) {
		item->serializeItemNode_OTBM(maphandle, file);
	}

	file.endNode();
	return true;
}

// ============================================================================
// Podium

bool Podium::readItemAttribute_OTBM(const IOMap& maphandle, OTBM_ItemAttribute attribute, BinaryNode* stream) {
	if (OTBM_ATTR_PODIUMOUTFIT == attribute) {
		uint8_t flags;
		uint8_t direction;

		uint16_t lookType;
		uint8_t lookHead;
		uint8_t lookBody;
		uint8_t lookLegs;
		uint8_t lookFeet;
		uint8_t lookAddon;

		uint16_t lookMount;
		uint8_t lookMountHead;
		uint8_t lookMountBody;
		uint8_t lookMountLegs;
		uint8_t lookMountFeet;

		if (
			// podium settings
			stream->getU8(flags) && stream->getU8(direction) &&

			// outfit
			stream->getU16(lookType) && stream->getU8(lookHead) && stream->getU8(lookBody) && stream->getU8(lookLegs) && stream->getU8(lookFeet) && stream->getU8(lookAddon) &&

			// mount
			stream->getU16(lookMount) && stream->getU8(lookMountHead) && stream->getU8(lookMountBody) && stream->getU8(lookMountLegs) && stream->getU8(lookMountFeet)
		) { //"if" condition ends here

			setShowOutfit((flags & PODIUM_SHOW_OUTFIT) != 0);
			setShowMount((flags & PODIUM_SHOW_MOUNT) != 0);
			setShowPlatform((flags & PODIUM_SHOW_PLATFORM) != 0);

			setDirection(static_cast<Direction>(direction));

			struct Outfit newOutfit = Outfit();
			newOutfit.lookType = static_cast<int>(lookType);
			newOutfit.lookHead = static_cast<int>(lookHead);
			newOutfit.lookBody = static_cast<int>(lookBody);
			newOutfit.lookLegs = static_cast<int>(lookLegs);
			newOutfit.lookFeet = static_cast<int>(lookFeet);
			newOutfit.lookAddon = static_cast<int>(lookAddon);
			newOutfit.lookMount = static_cast<int>(lookMount);
			newOutfit.lookMountHead = static_cast<int>(lookMountHead);
			newOutfit.lookMountBody = static_cast<int>(lookMountBody);
			newOutfit.lookMountLegs = static_cast<int>(lookMountLegs);
			newOutfit.lookMountFeet = static_cast<int>(lookMountFeet);
			setOutfit(newOutfit);
			return true;
		}
		return false;
	} else {
		return Item::readItemAttribute_OTBM(maphandle, attribute, stream);
	}
}

void Podium::serializeItemAttributes_OTBM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	Item::serializeItemAttributes_OTBM(maphandle, stream);

	uint8_t flags = PODIUM_SHOW_OUTFIT * static_cast<uint8_t>(showOutfit) + PODIUM_SHOW_MOUNT * static_cast<uint8_t>(showMount) + PODIUM_SHOW_PLATFORM * static_cast<uint8_t>(showPlatform);

	stream.addByte(OTBM_ATTR_PODIUMOUTFIT);
	stream.addU8(flags);
	stream.addU8(direction);

	stream.addU16(outfit.lookType);
	stream.addU8(outfit.lookHead);
	stream.addU8(outfit.lookBody);
	stream.addU8(outfit.lookLegs);
	stream.addU8(outfit.lookFeet);
	stream.addU8(outfit.lookAddon);

	stream.addU16(outfit.lookMount);
	stream.addU8(outfit.lookMountHead);
	stream.addU8(outfit.lookMountBody);
	stream.addU8(outfit.lookMountLegs);
	stream.addU8(outfit.lookMountFeet);
}
