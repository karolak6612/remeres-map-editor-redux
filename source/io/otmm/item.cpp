//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "io/iomap_otmm.h"
#include "game/item.h"
#include "game/complexitem.h"
#include "io/filehandle.h"
#include "map/map.h"
#include "ui/gui.h"

// ============================================================================
// Item

Item* Item::Create_OTMM(const IOMap& maphandle, BinaryNode* stream) {
	uint16_t _id;
	if (!stream->getU16(_id)) {
		return nullptr;
	}

	return Item::Create(_id);
}

bool Item::readItemAttribute_OTMM(const IOMap& maphandle, OTMM_ItemAttribute attr, BinaryNode* stream) {
	switch (attr) {
		case OTMM_ATTR_SUBTYPE: {
			uint16_t subtype;
			if (!stream->getU16(subtype)) {
				return false;
			}

			setSubtype(subtype & 0xf);
		} break;
		case OTMM_ATTR_ACTION_ID: {
			uint16_t aid;
			if (!stream->getU16(aid)) {
				return false;
			}
			setActionID(aid);
		} break;
		case OTMM_ATTR_UNIQUE_ID: {
			uint16_t uid;
			if (!stream->getU16(uid)) {
				return false;
			}

			setUniqueID(uid);
		} break;
		case OTMM_ATTR_TEXT: {
			std::string text;
			if (!stream->getString(text)) {
				return false;
			}

			setText(text);
		} break;
		case OTMM_ATTR_DESC: {
			std::string text;
			if (!stream->getString(text)) {
				return false;
			}

			setDescription(text);
		} break;

		// If otb structure changes....
		case OTMM_ATTR_DEPOT_ID: {
			return stream->skip(2);
		} break;
		case OTMM_ATTR_DOOR_ID: {
			return stream->skip(1);
		} break;
		case OTMM_ATTR_TELE_DEST: {
			return stream->skip(5);
		} break;
		default: {
			return false;
		} break;
	}

	return true;
}

bool Item::unserializeAttributes_OTMM(const IOMap& maphandle, BinaryNode* stream) {
	uint8_t attribute;
	while (stream->getU8(attribute)) {
		if (!readItemAttribute_OTMM(maphandle, OTMM_ItemAttribute(attribute), stream)) {
			return false;
		}
	}
	return true;
}

bool Item::unserializeItemNode_OTMM(const IOMap& maphandle, BinaryNode* node) {
	return unserializeAttributes_OTMM(maphandle, node);
}

void Item::serializeItemAttributes_OTMM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	if (getSubtype() > 0) {
		stream.addU8(OTMM_ATTR_SUBTYPE);
		stream.addU16(getSubtype());
	}

	if (getActionID()) {
		stream.addU8(OTMM_ATTR_ACTION_ID);
		stream.addU16(getActionID());
	}

	if (getUniqueID()) {
		stream.addU8(OTMM_ATTR_UNIQUE_ID);
		stream.addU16(getUniqueID());
	}

	if (getText().length() > 0) {
		stream.addU8(OTMM_ATTR_TEXT);
		stream.addString(getText());
	}
}

void Item::serializeItemCompact_OTMM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	stream.addU16(id);
}

bool Item::serializeItemNode_OTMM(const IOMap& maphandle, NodeFileWriteHandle& f) const {
	f.addNode(OTMM_ITEM);
	f.addU16(id);
	serializeItemAttributes_OTMM(maphandle, f);
	f.endNode();

	return true;
}

// ============================================================================
// Teleport

bool Teleport::readItemAttribute_OTMM(const IOMap& maphandle, OTMM_ItemAttribute attribute, BinaryNode* stream) {
	if (attribute == OTMM_ATTR_TELE_DEST) {
		uint16_t x = 0;
		uint16_t y = 0;
		uint8_t z = 0;
		if (!stream->getU16(x) || !stream->getU16(y) || !stream->getU8(z)) {
			return false;
		}

		destination = Position(x, y, z);
		return true;
	} else {
		return Item::readItemAttribute_OTMM(maphandle, attribute, stream);
	}
}

void Teleport::serializeItemAttributes_OTMM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	Item::serializeItemAttributes_OTMM(maphandle, stream);

	stream.addByte(OTMM_ATTR_TELE_DEST);
	stream.addU16(destination.x);
	stream.addU16(destination.y);
	stream.addU8(destination.z);
}

// ============================================================================
// Door

bool Door::readItemAttribute_OTMM(const IOMap& maphandle, OTMM_ItemAttribute attribute, BinaryNode* stream) {
	if (attribute == OTMM_ATTR_DOOR_ID) {
		uint8_t id = 0;
		if (!stream->getU8(id)) {
			return false;
		}
		doorid = id;
		return true;
	} else {
		return Item::readItemAttribute_OTMM(maphandle, attribute, stream);
	}
}

void Door::serializeItemAttributes_OTMM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	Item::serializeItemAttributes_OTMM(maphandle, stream);
	if (doorid) {
		stream.addByte(OTMM_ATTR_DOOR_ID);
		stream.addU8(doorid);
	}
}

// ============================================================================
// Depots

bool Depot::readItemAttribute_OTMM(const IOMap& maphandle, OTMM_ItemAttribute attribute, BinaryNode* stream) {
	if (attribute == OTMM_ATTR_DEPOT_ID) {
		uint16_t id = 0;
		if (!stream->getU16(id)) {
			return false;
		}
		depotid = id;
		return true;
	} else {
		return Item::readItemAttribute_OTMM(maphandle, attribute, stream);
	}
}

void Depot::serializeItemAttributes_OTMM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	Item::serializeItemAttributes_OTMM(maphandle, stream);
	if (depotid) {
		stream.addByte(OTMM_ATTR_DEPOT_ID);
		stream.addU16(depotid);
	}
}

// ============================================================================
// Container

bool Container::unserializeItemNode_OTMM(const IOMap& maphandle, BinaryNode* node) {
	bool ret = Item::unserializeAttributes_OTMM(maphandle, node);

	if (ret) {
		BinaryNode* child = node->getChild();
		if (child) {
			do {
				uint8_t type;
				if (!child->getByte(type)) {
					return false;
				}
				// load container items
				if (type == OTMM_ITEM) {
					Item* item = Item::Create_OTMM(maphandle, child);
					if (!item) {
						return false;
					}
					if (!item->unserializeItemNode_OTMM(maphandle, child)) {
						delete item;
						return false;
					}
					contents.push_back(item);
				} else {
					// corrupted file data!
					return false;
				}
			} while (child->advance());
		}
		return true;
	}
	return false;
}

bool Container::serializeItemNode_OTMM(const IOMap& maphandle, NodeFileWriteHandle& f) const {
	f.addNode(OTMM_ITEM);

	f.addU16(id);
	serializeItemAttributes_OTMM(maphandle, f);

	for (ItemVector::const_iterator it = contents.begin(); it != contents.end(); ++it) {
		(*it)->serializeItemNode_OTMM(maphandle, f);
	}
	f.endNode();
	return true;
}
