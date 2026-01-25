#include "game/item_metadata.h"
#include "ext/pugixml.hpp"
#include <iostream>
#include <fstream>
#include "game/items.h" // For ItemDatabase to check validity if needed

ItemMetadataManager::ItemMetadataManager() {
}

ItemMetadataManager::~ItemMetadataManager() {
}

bool ItemMetadataManager::load(const std::string& filename) {
	currentFilename = filename;
	metadataMap.clear();

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename.c_str());

	if (!result) {
		// If file not found, apply legacy defaults once
		if (result.status == pugi::status_file_not_found) {
			applyLegacyDefaults();
			// Optional: Save immediately to create the file?
			// save(filename);
			return true;
		}
		return false;
	}

	pugi::xml_node root = doc.child("technical-items");
	if (!root) {
		return false;
	}

	for (pugi::xml_node node = root.child("item"); node; node = node.next_sibling("item")) {
		uint16_t id = node.attribute("id").as_uint();
		if (id == 0) {
			continue;
		}

		ItemMetadata meta;

		// Parse Tech Type
		std::string typeStr = node.attribute("tech-type").as_string();
		if (typeStr == "invisible-stairs") {
			meta.techType = TechItemType::INVISIBLE_STAIRS;
		} else if (typeStr == "invisible-walkable") {
			meta.techType = TechItemType::INVISIBLE_WALKABLE;
		} else if (typeStr == "invisible-wall") {
			meta.techType = TechItemType::INVISIBLE_WALL;
		} else if (typeStr == "primal-light") {
			meta.techType = TechItemType::PRIMAL_LIGHT;
		} else if (typeStr == "client-id-zero") {
			meta.techType = TechItemType::CLIENT_ID_ZERO;
		} else {
			meta.techType = TechItemType::NONE;
		}

		// Parse Disguise
		meta.disguiseID = node.attribute("disguise-id").as_uint();

		if (!meta.isEmpty()) {
			metadataMap[id] = meta;
		}
	}

	return true;
}

bool ItemMetadataManager::save(const std::string& filename) {
	pugi::xml_document doc;
	pugi::xml_node root = doc.append_child("technical-items");

	for (const auto& pair : metadataMap) {
		if (pair.second.isEmpty()) {
			continue;
		}

		pugi::xml_node node = root.append_child("item");
		node.append_attribute("id").set_value(pair.first);

		switch (pair.second.techType) {
			case TechItemType::INVISIBLE_STAIRS:
				node.append_attribute("tech-type").set_value("invisible-stairs");
				break;
			case TechItemType::INVISIBLE_WALKABLE:
				node.append_attribute("tech-type").set_value("invisible-walkable");
				break;
			case TechItemType::INVISIBLE_WALL:
				node.append_attribute("tech-type").set_value("invisible-wall");
				break;
			case TechItemType::PRIMAL_LIGHT:
				node.append_attribute("tech-type").set_value("primal-light");
				break;
			case TechItemType::CLIENT_ID_ZERO:
				node.append_attribute("tech-type").set_value("client-id-zero");
				break;
			default:
				break;
		}

		if (pair.second.disguiseID != 0) {
			node.append_attribute("disguise-id").set_value(pair.second.disguiseID);
		}
	}

	return doc.save_file(filename.c_str());
}

const ItemMetadata& ItemMetadataManager::getMetadata(uint16_t id) const {
	static ItemMetadata emptyMeta;
	auto it = metadataMap.find(id);
	if (it != metadataMap.end()) {
		return it->second;
	}
	return emptyMeta;
}

void ItemMetadataManager::setMetadata(uint16_t id, const ItemMetadata& metadata) {
	if (metadata.isEmpty()) {
		removeMetadata(id);
	} else {
		metadataMap[id] = metadata;
	}
}

void ItemMetadataManager::removeMetadata(uint16_t id) {
	metadataMap.erase(id);
}

void ItemMetadataManager::applyLegacyDefaults() {
	// Hardcoded hacks from map_drawer.cpp / item_drawer.cpp

	// Yellow invisible stairs tile (459 -> Client 469)
	// Note: We key by Server ID here, assuming standard mapping.
	// Ideally we would look up by Client ID if we wanted exact parity,
	// but the system typically works with Server IDs.
	// The previous code checked CLIENT IDs (469, 470, 2187)

	// Scan all items to find matching client IDs?
	// This is expensive but only happens once on first run.

	for (size_t i = 100; i <= g_items.getMaxID(); ++i) {
		if (!g_items.typeExists(i)) {
			continue;
		}

		const ItemType& it = g_items.getItemType(i);
		uint16_t cid = it.clientID;

		if (it.id == 0) {
			// Red invalid client id handled dynamically usually,
			// but if we want to flag it:
			// metadataMap[i].techType = TechItemType::CLIENT_ID_ZERO;
			continue;
		}

		if (cid == 469) { // Yellow invisible stairs
			metadataMap[i].techType = TechItemType::INVISIBLE_STAIRS;
		} else if (cid == 470 || cid == 17970 || cid == 20028 || cid == 34168) { // Red invisible walkable
			metadataMap[i].techType = TechItemType::INVISIBLE_WALKABLE;
		} else if (cid == 2187) { // Cyan invisible wall
			metadataMap[i].techType = TechItemType::INVISIBLE_WALL;
		} else if ((cid >= 39092 && cid <= 39100) || cid == 39236 || cid == 39367 || cid == 39368) {
			metadataMap[i].techType = TechItemType::PRIMAL_LIGHT;
		}
	}
}
