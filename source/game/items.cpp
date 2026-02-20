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
#include <string_view>
#include <map>
#include <unordered_map>
#include <array>
#include <format>
#include <limits>

#include "game/materials.h"
#include "app/managers/version_manager.h"
#include "ui/gui.h"
#include <string.h> // memcpy

#include "game/items.h"
#include "game/item.h"
#include "game/item_loaders.h"

ItemDatabase g_items;

ItemType::ItemType() :
	sprite(nullptr),
	id(0),
	clientID(0),
	brush(nullptr),
	doodad_brush(nullptr),
	collection_brush(nullptr),
	raw_brush(nullptr),
	is_metaitem(false),
	has_raw(false),
	in_other_tileset(false),
	group(ITEM_GROUP_NONE),
	type(ITEM_TYPE_NONE),
	volume(0),
	maxTextLen(0),
	slot_position(SLOTP_HAND),
	weapon_type(WEAPON_NONE),
	ground_equivalent(0),
	border_group(0),
	has_equivalent(false),
	wall_hate_me(false),
	name(""),
	editorsuffix(""),
	description(""),
	weight(0.0f),
	attack(0),
	defense(0),
	armor(0),
	charges(0),
	client_chargeable(false),
	extra_chargeable(false),
	ignoreLook(false),
	isHangable(false),
	hookEast(false),
	hookSouth(false),
	canReadText(false),
	canWriteText(false),
	allowDistRead(false),
	replaceable(true),
	decays(false),
	stackable(false),
	moveable(true),
	alwaysOnBottom(false),
	pickupable(false),
	rotable(false),
	isBorder(false),
	isOptionalBorder(false),
	isWall(false),
	isBrushDoor(false),
	isOpen(false),
	isLocked(false),
	isTable(false),
	isCarpet(false),
	floorChangeDown(false),
	floorChangeNorth(false),
	floorChangeSouth(false),
	floorChangeEast(false),
	floorChangeWest(false),
	floorChange(false),
	unpassable(false),
	blockPickupable(false),
	blockMissiles(false),
	blockPathfinder(false),
	hasElevation(false),
	is_tooltipable(false),
	alwaysOnTopOrder(0),
	rotateTo(0),
	way_speed(100),
	border_alignment(BORDER_NONE) {
	////
}

ItemType::~ItemType() {
	////
}

void ItemType::updateTooltipable() {
	is_tooltipable = isContainer() || isDoor() || isTeleport();
}

bool ItemType::isFloorChange() const {
	return floorChange || floorChangeDown || floorChangeNorth || floorChangeSouth || floorChangeEast || floorChangeWest;
}

ItemDatabase::ItemDatabase() :
	// Version information
	MajorVersion(0),
	MinorVersion(0),
	BuildNumber(0),

	// Count of GameSprite types
	item_count(0),
	effect_count(0),
	monster_count(0),
	distance_count(0),

	minclientID(0),
	maxclientID(0),

	max_item_id(0) {
	////
}

void ItemDatabase::clear() {
	items.clear();
}

bool ItemDatabase::loadFromOtbGeneric(BinaryNode* itemNode, OtbFileFormatVersion version, wxString& error, std::vector<std::string>& warnings) {
	return OtbLoader::load(*this, itemNode, version, error, warnings);
}

bool ItemDatabase::loadFromOtb(const FileName& datafile, wxString& error, std::vector<std::string>& warnings) {
	std::string filename = datafile.GetFullPath().ToStdString();
	DiskNodeFileReadHandle f(filename, StringVector(1, "OTBI"));

	if (!f.isOk()) {
		error = std::format("Couldn't open file \"{}\": {}", filename, f.getErrorMessage());
		return false;
	}

	BinaryNode* root = f.getRootNode();

#define safe_get(node, func, ...)               \
	do {                                        \
		if (!node->get##func(__VA_ARGS__)) {    \
			error = wxstr(f.getErrorMessage()); \
			return false;                       \
		}                                       \
	} while (false)

	// Read root flags
	root->skip(1); // Type info
	// uint32_t flags =

	root->skip(4); // Unused?

	uint8_t attr;
	safe_get(root, U8, attr);
	if (attr == ROOT_ATTR_VERSION) {
		uint16_t datalen;
		if (!root->getU16(datalen) || datalen != 4 + 4 + 4 + 1 * 128) {
			error = "items.otb: Size of version header is invalid, updated .otb version?";
			return false;
		}
		safe_get(root, U32, MajorVersion); // items otb format file version
		safe_get(root, U32, MinorVersion); // client version
		safe_get(root, U32, BuildNumber); // revision
		std::string csd;
		csd.resize(128);

		if (!root->getRAW((uint8_t*)csd.data(), 128)) { // CSDVersion ??
			error = wxstr(f.getErrorMessage());
			return false;
		}
	} else {
		error = "Expected ROOT_ATTR_VERSION as first node of items.otb!";
	}

	if (g_settings.getInteger(Config::CHECK_SIGNATURES)) {
		if (g_version.GetCurrentVersion().getOTBVersion().format_version != MajorVersion) {
			error = std::format("Unsupported items.otb version (version {})", MajorVersion);
			return false;
		}
	}

	BinaryNode* itemNode = root->getChild();
	OtbFileFormatVersion version = OtbFileFormatVersion::V1;
	switch (MajorVersion) {
		case 1:
			version = OtbFileFormatVersion::V1;
			break;
		case 2:
			version = OtbFileFormatVersion::V2;
			break;
		case 3:
			version = OtbFileFormatVersion::V3;
			break;
		default:
			error = std::format("items.otb: Unsupported version ({})", MajorVersion);
			return false;
	}

	bool res = loadFromOtbGeneric(itemNode, version, error, warnings);
	if (res) {
		updateAllTooltipableFlags();
	}
	return res;
}

bool ItemDatabase::loadItemFromGameXml(pugi::xml_node itemNode, int id) {
	if (g_version.GetCurrentVersion().getProtocolID() < CLIENT_VERSION_980 && id > 20000 && id < 20100) {
		itemNode = itemNode.next_sibling();
		return true;
	} else if (id > 30000 && id < 30100) {
		itemNode = itemNode.next_sibling();
		return true;
	}

	ItemType& it = getItemType(id);

	it.name = itemNode.attribute("name").as_string();
	it.editorsuffix = itemNode.attribute("editorsuffix").as_string();

	XmlLoader::loadAttributes(it, itemNode);

	return true;
}
bool ItemDatabase::loadFromGameXml(const FileName& identifier, wxString& error, std::vector<std::string>& warnings) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(identifier.GetFullPath().mb_str());
	if (!result) {
		error = "Could not load items.xml (Syntax error?)";
		return false;
	}

	pugi::xml_node node = doc.child("items");
	if (!node) {
		error = "items.xml, invalid root node.";
		return false;
	}

	for (pugi::xml_node itemNode = node.first_child(); itemNode; itemNode = itemNode.next_sibling()) {
		if (as_lower_str(itemNode.name()) != "item") {
			continue;
		}

		uint16_t fromId = 0;
		uint16_t toId = 0;
		if (const pugi::xml_attribute attribute = itemNode.attribute("id")) {
			fromId = toId = attribute.as_ushort();
		} else {
			fromId = itemNode.attribute("fromid").as_ushort();
			toId = itemNode.attribute("toid").as_ushort();
		}

		if (fromId == 0 || toId == 0) {
			error = "Could not read item id from item node.";
			return false;
		}

		for (uint16_t id = fromId; id <= toId; ++id) {
			if (!loadItemFromGameXml(itemNode, id)) {
				return false;
			}
		}
	}
	updateAllTooltipableFlags();
	return true;
}

bool ItemDatabase::loadMetaItem(pugi::xml_node node) {
	if (const pugi::xml_attribute attribute = node.attribute("id")) {
		const uint16_t id = attribute.as_ushort();
		if (id == 0 || (id < items.size() && items[id])) {
			return false;
		}

		if (id >= items.size()) {
			items.resize(id + 1);
		}
		items[id] = std::make_unique<ItemType>();
		items[id]->is_metaitem = true;
		items[id]->id = id;
		return true;
	}
	return false;
}

ItemType& ItemDatabase::getItemType(int id) {
	if (static_cast<size_t>(id) < items.size()) {
		if (auto& it = items[id]) {
			return *it;
		}
	}
	static thread_local ItemType dummyItemType; // use this for invalid ids
	dummyItemType = ItemType(); // Reset to clean state
	return dummyItemType;
}

bool ItemDatabase::typeExists(int id) const {
	return static_cast<size_t>(id) < items.size() && items[id] != nullptr;
}

void ItemDatabase::updateAllTooltipableFlags() {
	for (auto& it_ptr : items) {
		if (it_ptr) {
			it_ptr->updateTooltipable();
		}
	}
}
