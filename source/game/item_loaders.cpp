#include "app/main.h"
#include "game/item_loaders.h"
#include "game/items.h"
#include "game/item.h"
#include "io/filehandle.h"
#include "ext/pugixml.hpp"
#include "ui/gui.h"

#include <array>
#include <format>
#include <limits>
#include <unordered_map>
#include <string_view>
#include <cstring>

namespace {

	// Helper functions for parsing attributes
	void parseItemTypeAttribute(ItemType& it, std::string_view value) {
		static const std::unordered_map<std::string_view, void (*)(ItemType&)> parsers = {
			{ "depot", [](ItemType& i) { i.type = ITEM_TYPE_DEPOT; } },
			{ "mailbox", [](ItemType& i) { i.type = ITEM_TYPE_MAILBOX; } },
			{ "trashholder", [](ItemType& i) { i.type = ITEM_TYPE_TRASHHOLDER; } },
			{ "container", [](ItemType& i) { i.type = ITEM_TYPE_CONTAINER; } },
			{ "door", [](ItemType& i) { i.type = ITEM_TYPE_DOOR; } },
			{ "magicfield", [](ItemType& i) { i.group = ITEM_GROUP_MAGICFIELD; i.type = ITEM_TYPE_MAGICFIELD; } },
			{ "teleport", [](ItemType& i) { i.type = ITEM_TYPE_TELEPORT; } },
			{ "bed", [](ItemType& i) { i.type = ITEM_TYPE_BED; } },
			{ "key", [](ItemType& i) { i.type = ITEM_TYPE_KEY; } },
			{ "podium", [](ItemType& i) { i.type = ITEM_TYPE_PODIUM; } },
		};

		if (auto it_parser = parsers.find(value); it_parser != parsers.end()) {
			it_parser->second(it);
		}
	}

	void parseSlotTypeAttribute(ItemType& it, std::string_view value) {
		static const std::unordered_map<std::string_view, void (*)(ItemType&)> parsers = {
			{ "head", [](ItemType& i) { i.slot_position |= SLOTP_HEAD; } },
			{ "body", [](ItemType& i) { i.slot_position |= SLOTP_ARMOR; } },
			{ "legs", [](ItemType& i) { i.slot_position |= SLOTP_LEGS; } },
			{ "feet", [](ItemType& i) { i.slot_position |= SLOTP_FEET; } },
			{ "backpack", [](ItemType& i) { i.slot_position |= SLOTP_BACKPACK; } },
			{ "two-handed", [](ItemType& i) { i.slot_position |= SLOTP_TWO_HAND; } },
			{ "right-hand", [](ItemType& i) { i.slot_position &= ~SLOTP_LEFT; } },
			{ "left-hand", [](ItemType& i) { i.slot_position &= ~SLOTP_RIGHT; } },
			{ "necklace", [](ItemType& i) { i.slot_position |= SLOTP_NECKLACE; } },
			{ "ring", [](ItemType& i) { i.slot_position |= SLOTP_RING; } },
			{ "ammo", [](ItemType& i) { i.slot_position |= SLOTP_AMMO; } },
			{ "hand", [](ItemType& i) { i.slot_position |= SLOTP_HAND; } },
		};

		if (auto it_parser = parsers.find(value); it_parser != parsers.end()) {
			it_parser->second(it);
		}
	}

	void parseWeaponTypeAttribute(ItemType& it, std::string_view value) {
		static const std::unordered_map<std::string_view, void (*)(ItemType&)> parsers = {
			{ "sword", [](ItemType& i) { i.weapon_type = WEAPON_SWORD; } },
			{ "club", [](ItemType& i) { i.weapon_type = WEAPON_CLUB; } },
			{ "axe", [](ItemType& i) { i.weapon_type = WEAPON_AXE; } },
			{ "shield", [](ItemType& i) { i.weapon_type = WEAPON_SHIELD; } },
			{ "distance", [](ItemType& i) { i.weapon_type = WEAPON_DISTANCE; } },
			{ "wand", [](ItemType& i) { i.weapon_type = WEAPON_WAND; } },
			{ "ammunition", [](ItemType& i) { i.weapon_type = WEAPON_AMMO; } },
		};

		if (auto it_parser = parsers.find(value); it_parser != parsers.end()) {
			it_parser->second(it);
		}
	}

	void parseFloorChangeAttribute(ItemType& it, std::string_view value) {
		static const std::unordered_map<std::string_view, void (*)(ItemType&)> parsers = {
			{ "down", [](ItemType& i) { i.floorChangeDown = true; i.floorChange = true; } },
			{ "north", [](ItemType& i) { i.floorChangeNorth = true; i.floorChange = true; } },
			{ "south", [](ItemType& i) { i.floorChangeSouth = true; i.floorChange = true; } },
			{ "west", [](ItemType& i) { i.floorChangeWest = true; i.floorChange = true; } },
			{ "east", [](ItemType& i) { i.floorChangeEast = true; i.floorChange = true; } },
			{ "northex", [](ItemType& i) { i.floorChange = true; } },
			{ "southex", [](ItemType& i) { i.floorChange = true; } },
			{ "westex", [](ItemType& i) { i.floorChange = true; } },
			{ "eastex", [](ItemType& i) { i.floorChange = true; } },
			{ "southalt", [](ItemType& i) { i.floorChange = true; } },
			{ "eastalt", [](ItemType& i) { i.floorChange = true; } },
		};

		if (auto it_parser = parsers.find(value); it_parser != parsers.end()) {
			it_parser->second(it);
		}
	}

} // namespace

bool OtbLoader::load(ItemDatabase& db, BinaryNode* itemNode, OtbFileFormatVersion version, wxString& error, std::vector<std::string>& warnings) {
	uint8_t u8;

	for (; itemNode != nullptr; itemNode = itemNode->advance()) {
		if (!itemNode->getU8(u8)) {
			// Invalid!
			warnings.push_back("Invalid item type encountered...");
			continue;
		}

		if (ItemGroup_t(u8) == ITEM_GROUP_DEPRECATED) {
			continue;
		}

		ItemType t;
		t.group = ItemGroup_t(u8);

		static const auto group_handlers = [] {
			std::array<void (*)(ItemType&, OtbFileFormatVersion), ITEM_GROUP_LAST + 1> h {};
			h.fill([](ItemType&, OtbFileFormatVersion) {});
			h[ITEM_GROUP_DOOR] = [](ItemType& i, [[maybe_unused]] OtbFileFormatVersion v) { i.type = ITEM_TYPE_DOOR; };
			h[ITEM_GROUP_CONTAINER] = [](ItemType& i, [[maybe_unused]] OtbFileFormatVersion v) { i.type = ITEM_TYPE_CONTAINER; };
			h[ITEM_GROUP_RUNE] = [](ItemType& i, [[maybe_unused]] OtbFileFormatVersion v) { i.client_chargeable = true; };
			h[ITEM_GROUP_TELEPORT] = [](ItemType& i, [[maybe_unused]] OtbFileFormatVersion v) { i.type = ITEM_TYPE_TELEPORT; };
			h[ITEM_GROUP_MAGICFIELD] = [](ItemType& i, [[maybe_unused]] OtbFileFormatVersion v) { i.type = ITEM_TYPE_MAGICFIELD; };
			h[ITEM_GROUP_PODIUM] = [](ItemType& i, OtbFileFormatVersion v) {
				if (v >= OtbFileFormatVersion::V3) {
					i.type = ITEM_TYPE_PODIUM;
				}
			};
			return h;
		}();

		if (t.group <= ITEM_GROUP_LAST) {
			group_handlers[t.group](t, version);
		} else {
			warnings.push_back("Unknown item group declaration");
		}

		uint32_t flags;
		if (itemNode->getU32(flags)) {
			t.unpassable = ((flags & FLAG_UNPASSABLE) == FLAG_UNPASSABLE);
			t.blockMissiles = ((flags & FLAG_BLOCK_MISSILES) == FLAG_BLOCK_MISSILES);
			t.blockPathfinder = ((flags & FLAG_BLOCK_PATHFINDER) == FLAG_BLOCK_PATHFINDER);
			t.hasElevation = ((flags & FLAG_HAS_ELEVATION) == FLAG_HAS_ELEVATION);
			t.pickupable = ((flags & FLAG_PICKUPABLE) == FLAG_PICKUPABLE);
			t.moveable = ((flags & FLAG_MOVEABLE) == FLAG_MOVEABLE);
			t.stackable = ((flags & FLAG_STACKABLE) == FLAG_STACKABLE);
			t.floorChangeDown = ((flags & FLAG_FLOORCHANGEDOWN) == FLAG_FLOORCHANGEDOWN);
			t.floorChangeNorth = ((flags & FLAG_FLOORCHANGENORTH) == FLAG_FLOORCHANGENORTH);
			t.floorChangeEast = ((flags & FLAG_FLOORCHANGEEAST) == FLAG_FLOORCHANGEEAST);
			t.floorChangeSouth = ((flags & FLAG_FLOORCHANGESOUTH) == FLAG_FLOORCHANGESOUTH);
			t.floorChangeWest = ((flags & FLAG_FLOORCHANGEWEST) == FLAG_FLOORCHANGEWEST);
			t.floorChange = t.floorChangeDown || t.floorChangeNorth || t.floorChangeEast || t.floorChangeSouth || t.floorChangeWest;

			// The OTB `FLAG_ALWAYSONTOP` is mapped to the editor's `alwaysOnBottom` property.
			t.alwaysOnBottom = ((flags & FLAG_ALWAYSONTOP) == FLAG_ALWAYSONTOP);
			t.isHangable = ((flags & FLAG_HANGABLE) == FLAG_HANGABLE);
			t.hookEast = ((flags & FLAG_HOOK_EAST) == FLAG_HOOK_EAST);
			t.hookSouth = ((flags & FLAG_HOOK_SOUTH) == FLAG_HOOK_SOUTH);
			t.allowDistRead = ((flags & FLAG_ALLOWDISTREAD) == FLAG_ALLOWDISTREAD);
			t.rotable = ((flags & FLAG_ROTABLE) == FLAG_ROTABLE);
			t.canReadText = ((flags & FLAG_READABLE) == FLAG_READABLE);

			if (version >= OtbFileFormatVersion::V3) {
				t.client_chargeable = ((flags & FLAG_CLIENTCHARGES) == FLAG_CLIENTCHARGES);
				t.ignoreLook = ((flags & FLAG_IGNORE_LOOK) == FLAG_IGNORE_LOOK);
			}
		}

		using AttributeHandler = bool (*)(ItemDatabase&, ItemType&, BinaryNode*, uint16_t, wxString&, std::vector<std::string>&);
		static const auto handlers = [] {
			std::array<AttributeHandler, std::numeric_limits<uint8_t>::max() + 1> h {};
			h.fill([](ItemDatabase&, ItemType&, BinaryNode* node, uint16_t len, [[maybe_unused]] wxString&, [[maybe_unused]] std::vector<std::string>&) {
				node->skip(len);
				return true;
			});

			h[ITEM_ATTR_SERVERID] = [](ItemDatabase& db, ItemType& it, BinaryNode* node, uint16_t len, wxString& err, std::vector<std::string>& warnings) {
				if (len != sizeof(uint16_t)) {
					err = std::format("items.otb: Unexpected data length of server id block (Should be {} bytes)", sizeof(uint16_t));
					return false;
				}
				if (!node->getU16(it.id)) {
					warnings.push_back("Invalid item type property (serverID)");
					return true;
				}
				if (db.max_item_id < it.id) {
					db.max_item_id = it.id;
				}
				return true;
			};

			h[ITEM_ATTR_CLIENTID] = [](ItemDatabase&, ItemType& it, BinaryNode* node, uint16_t len, wxString& err, std::vector<std::string>& warnings) {
				if (len != sizeof(uint16_t)) {
					err = std::format("items.otb: Unexpected data length of client id block (Should be {} bytes)", sizeof(uint16_t));
					return false;
				}
				if (!node->getU16(it.clientID)) {
					warnings.push_back("Invalid item type property (clientID)");
				}
				it.sprite = static_cast<GameSprite*>(g_gui.gfx.getSprite(it.clientID));
				return true;
			};

			h[ITEM_ATTR_SPEED] = [](ItemDatabase&, ItemType& it, BinaryNode* node, uint16_t len, wxString& err, std::vector<std::string>& warnings) {
				if (len != sizeof(uint16_t)) {
					err = std::format("items.otb: Unexpected data length of speed block (Should be {} bytes)", sizeof(uint16_t));
					return false;
				}
				uint16_t speed = 0;
				if (!node->getU16(speed)) {
					warnings.push_back("Invalid item type property (speed)");
				}
				it.way_speed = speed;
				return true;
			};

			h[ITEM_ATTR_LIGHT2] = [](ItemDatabase&, ItemType&, BinaryNode* node, uint16_t len, [[maybe_unused]] wxString&, std::vector<std::string>& warnings) {
				const size_t expected_len = 4; // sizeof(lightBlock2)
				if (len != expected_len) {
					warnings.push_back(std::format("items.otb: Unexpected data length of item light (2) block (Should be {} bytes)", expected_len));
					return true;
				}
				if (!node->skip(4)) {
					warnings.push_back("Invalid item type property (light2)");
				}
				return true;
			};

			h[ITEM_ATTR_TOPORDER] = [](ItemDatabase&, ItemType& it, BinaryNode* node, uint16_t len, [[maybe_unused]] wxString&, std::vector<std::string>& warnings) {
				if (len != sizeof(uint8_t)) {
					warnings.push_back("items.otb: Unexpected data length of item toporder block (Should be 1 byte)");
					return true;
				}
				uint8_t u8_top = 0;
				if (!node->getU8(u8_top)) {
					warnings.push_back("Invalid item type property (topOrder)");
				}
				it.alwaysOnTopOrder = u8_top;
				return true;
			};

			h[ITEM_ATTR_NAME] = [](ItemDatabase&, ItemType& it, BinaryNode* node, uint16_t len, [[maybe_unused]] wxString&, std::vector<std::string>& warnings) {
				if (len >= 128) {
					warnings.push_back("items.otb: Unexpected data length of item name block (Should be < 128 bytes)");
					return true;
				}
				uint8_t name[128] = { 0 };
				if (!node->getRAW(name, len)) {
					warnings.push_back("Invalid item type property (name)");
				}
				it.name = reinterpret_cast<const char*>(name);
				return true;
			};

			h[ITEM_ATTR_DESCR] = [](ItemDatabase&, ItemType& it, BinaryNode* node, uint16_t len, [[maybe_unused]] wxString&, std::vector<std::string>& warnings) {
				if (len >= 128) {
					warnings.push_back("items.otb: Unexpected data length of item descr block (Should be < 128 bytes)");
					return true;
				}
				uint8_t description[128] = { 0 };
				if (!node->getRAW(description, len)) {
					warnings.push_back("Invalid item type property (description)");
				}
				it.description = reinterpret_cast<const char*>(description);
				return true;
			};

			h[ITEM_ATTR_MAXITEMS] = [](ItemDatabase&, ItemType& it, BinaryNode* node, uint16_t len, [[maybe_unused]] wxString&, std::vector<std::string>& warnings) {
				if (len != sizeof(uint16_t)) {
					warnings.push_back("items.otb: Unexpected data length of item volume block (Should be 2 bytes)");
					return true;
				}
				if (!node->getU16(it.volume)) {
					warnings.push_back("Invalid item type property (volume)");
				}
				return true;
			};

			h[ITEM_ATTR_WEIGHT] = [](ItemDatabase&, ItemType& it, BinaryNode* node, uint16_t len, [[maybe_unused]] wxString&, std::vector<std::string>& warnings) {
				if (len != sizeof(double)) {
					warnings.push_back("items.otb: Unexpected data length of item weight block (Should be 8 bytes)");
					return true;
				}
				double weight;
				if (!node->getRAW(reinterpret_cast<uint8_t*>(&weight), sizeof(double))) {
					warnings.push_back("Invalid item type property (weight)");
					return true;
				}
				it.weight = static_cast<float>(weight);
				return true;
			};

			h[ITEM_ATTR_ROTATETO] = [](ItemDatabase&, ItemType& it, BinaryNode* node, uint16_t len, [[maybe_unused]] wxString&, std::vector<std::string>& warnings) {
				if (len != sizeof(uint16_t)) {
					warnings.push_back("items.otb: Unexpected data length of item rotateTo block (Should be 2 bytes)");
					return true;
				}
				uint16_t rotate = 0;
				if (!node->getU16(rotate)) {
					warnings.push_back("Invalid item type property (rotateTo)");
				}
				it.rotateTo = rotate;
				return true;
			};

			h[ITEM_ATTR_WRITEABLE3] = [](ItemDatabase&, ItemType& it, BinaryNode* node, uint16_t len, [[maybe_unused]] wxString&, std::vector<std::string>& warnings) {
				const size_t expected_len = 4; // sizeof(writeableBlock3)
				if (len != expected_len) {
					warnings.push_back(std::format("items.otb: Unexpected data length of item writeable (3) block (Should be {} bytes)", expected_len));
					return true;
				}
				uint16_t readOnlyID = 0;
				uint16_t maxTextLen = 0;
				if (!node->getU16(readOnlyID)) {
					warnings.push_back("Invalid item type property (writeable3_readOnlyID)");
				}
				if (!node->getU16(maxTextLen)) {
					warnings.push_back("Invalid item type property (writeable3_maxTextLen)");
				}
				it.maxTextLen = maxTextLen;
				return true;
			};

			h[ITEM_ATTR_CLASSIFICATION] = [](ItemDatabase&, ItemType& it, BinaryNode* node, uint16_t len, [[maybe_unused]] wxString&, std::vector<std::string>& warnings) {
				if (len != sizeof(uint8_t)) {
					warnings.push_back("items.otb: Unexpected data length of item classification block (Should be 1 byte)");
					return true;
				}
				uint8_t cls = 0;
				if (!node->getU8(cls)) {
					warnings.push_back("Invalid item type property (classification)");
				}
				it.classification = cls;
				return true;
			};

			return h;
		}();

		uint8_t attribute;
		while (itemNode->getU8(attribute)) {
			uint16_t datalen;
			if (!itemNode->getU16(datalen)) {
				warnings.push_back("Invalid item type property (premature end)");
				break;
			}

			if (!handlers[attribute](db, t, itemNode, datalen, error, warnings)) {
				return false;
			}
		}

		// items is public in ItemDatabase
		if (t.id < db.items.size() && db.items[t.id].id != 0) {
			warnings.push_back("items.otb: Duplicate items");
		}

		if (static_cast<size_t>(t.id) >= db.items.size()) {
			db.items.resize(static_cast<size_t>(t.id) + 1);
		}
		db.items[t.id] = std::move(t);
	}
	return true;
}

void XmlLoader::loadAttributes(ItemType& it, pugi::xml_node node) {
	using ParserFunc = void (*)(ItemType&, pugi::xml_node);
	static const std::unordered_map<std::string_view, ParserFunc> parsers = {
		{ "type", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 std::string val = attr.as_string();
				 to_lower_str(val);
				 parseItemTypeAttribute(it, val);
			 }
		 } },
		{ "name", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 it.name = attr.as_string();
			 }
		 } },
		{ "description", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 it.description = attr.as_string();
			 }
		 } },
		{ "speed", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 it.way_speed = attr.as_uint();
			 }
		 } },
		{ "weight", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 it.weight = attr.as_int() / 100.f;
			 }
		 } },
		{ "armor", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 it.armor = attr.as_int();
			 }
		 } },
		{ "defense", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 it.defense = attr.as_int();
			 }
		 } },
		{ "slottype", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 std::string val = attr.as_string();
				 to_lower_str(val);
				 parseSlotTypeAttribute(it, val);
			 }
		 } },
		{ "weapontype", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 std::string val = attr.as_string();
				 to_lower_str(val);
				 parseWeaponTypeAttribute(it, val);
			 }
		 } },
		{ "rotateto", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 it.rotateTo = attr.as_ushort();
			 }
		 } },
		{ "containersize", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 it.volume = attr.as_ushort();
			 }
		 } },
		{ "readable", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 it.canReadText = attr.as_bool();
			 }
		 } },
		{ "writeable", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 it.canWriteText = it.canReadText = attr.as_bool();
			 }
		 } },
		{ "decayto", [](ItemType& it, [[maybe_unused]] pugi::xml_node node) {
			 it.decays = true;
		 } },
		{ "maxtextlen", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 it.maxTextLen = attr.as_ushort();
				 it.canReadText = it.maxTextLen > 0;
			 }
		 } },
		{ "maxtextlength", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 it.maxTextLen = attr.as_ushort();
				 it.canReadText = it.maxTextLen > 0;
			 }
		 } },
		{ "allowdistread", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 it.allowDistRead = attr.as_bool();
			 }
		 } },
		{ "charges", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 it.charges = attr.as_uint();
				 it.extra_chargeable = true;
			 }
		 } },
		{ "floorchange", [](ItemType& it, pugi::xml_node node) {
			 if (auto attr = node.attribute("value")) {
				 std::string val = attr.as_string();
				 to_lower_str(val);
				 parseFloorChangeAttribute(it, val);
			 }
		 } }
	};

	for (auto attributeNode : node.children()) {
		if (auto attr = attributeNode.attribute("key")) {
			std::string key = attr.as_string();
			to_lower_str(key);
			if (auto it_parser = parsers.find(key); it_parser != parsers.end()) {
				it_parser->second(it, attributeNode);
			}
		}
	}
}
