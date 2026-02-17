#include "header_serialization_otbm.h"

#include "map/map.h"
#include "ui/dialog_util.h"
#include <spdlog/spdlog.h>
#include <format>

bool HeaderSerializationOTBM::getVersionInfo(NodeFileReadHandle* f, MapVersion& out_ver) {
	BinaryNode* root = f->getRootNode();
	if (!root) {
		return false;
	}

	if (!root->skip(1)) { // Skip the type byte
		return false;
	}

	uint32_t u32;

	if (!root->getU32(u32)) { // Version
		return false;
	}
	out_ver.otbm = static_cast<MapVersionID>(u32);

	uint16_t u16;
	if (!root->getU16(u16) || !root->getU16(u16) || !root->getU32(u32)) {
		return false;
	}

	if (!root->getU32(u32)) { // OTB minor version
		return false;
	}

	out_ver.client = static_cast<ClientVersionID>(u32);
	return true;
}

bool HeaderSerializationOTBM::loadMapRoot(Map& map, NodeFileReadHandle& f, MapVersion& version, BinaryNode*& root, BinaryNode*& mapHeaderNode) {
	root = f.getRootNode();
	if (!root) {
		spdlog::error("Could not read root node.");
		return false;
	}
	if (!root->skip(1)) { // Skip the type byte
		return false;
	}

	uint32_t u32;
	if (!root->getU32(u32)) {
		return false;
	}

	version.otbm = static_cast<MapVersionID>(u32);

	if (version.otbm > MAP_OTBM_4) {
		// Failed to read version
		if (DialogUtil::PopupDialog("Map error", "The loaded map appears to be a OTBM format that is not supported by the editor.\n"
												 "Do you still want to attempt to load the map?",
									wxYES | wxNO)
			== wxID_YES) {
			spdlog::warn("Unsupported or damaged map version: {}", static_cast<int>(version.otbm));
		} else {
			spdlog::error("Unsupported OTBM version {}, could not load map", static_cast<int>(version.otbm));
			return false;
		}
	}

	uint16_t u16;
	if (!root->getU16(u16)) {
		return false;
	}
	map.width = u16;

	if (!root->getU16(u16)) {
		return false;
	}
	map.height = u16;

	if (!root->getU32(u32)) {
		return false;
	}

	if (u32 > static_cast<uint32_t>(g_items.MajorVersion)) {
		if (DialogUtil::PopupDialog("Map error", "The loaded map appears to be a items.otb format that deviates from the "
												 "items.otb loaded by the editor. Do you still want to attempt to load the map?",
									wxYES | wxNO)
			== wxID_YES) {
			spdlog::warn("Unsupported or damaged OTB major version: {}", u32);
		} else {
			spdlog::error("Outdated items.otb (major {}), could not load map", u32);
			return false;
		}
	}

	if (!root->getU32(u32)) {
		return false;
	}

	if (u32 > static_cast<uint32_t>(g_items.MinorVersion)) {
		spdlog::warn("This editor needs an updated items.otb version (found minor {})", u32);
	}
	version.client = static_cast<ClientVersionID>(u32);

	mapHeaderNode = root->getChild();
	uint8_t u8;
	if (mapHeaderNode == nullptr || !mapHeaderNode->getByte(u8) || u8 != OTBM_MAP_DATA) {
		spdlog::error("Could not get root child node (OTBM_MAP_DATA). Cannot recover from fatal error!");
		return false;
	}
	return true;
}

bool HeaderSerializationOTBM::readMapAttributes(Map& map, BinaryNode* mapHeaderNode) {
	uint8_t attribute;
	while (mapHeaderNode->getU8(attribute)) {
		switch (attribute) {
			case OTBM_ATTR_DESCRIPTION: {
				if (!mapHeaderNode->getString(map.description)) {
					spdlog::warn("Invalid map description tag");
					return false;
				}
				break;
			}
			case OTBM_ATTR_EXT_SPAWN_FILE: {
				if (!mapHeaderNode->getString(map.spawnfile)) {
					spdlog::warn("Invalid map spawnfile tag");
					return false;
				}
				break;
			}
			case OTBM_ATTR_EXT_HOUSE_FILE: {
				if (!mapHeaderNode->getString(map.housefile)) {
					spdlog::warn("Invalid map housefile tag");
					return false;
				}
				break;
			}
			case OTBM_ATTR_EXT_SPAWN_NPC_FILE: {
				// compatibility: skip Canary RME NPC spawn file tag
				std::string stringToSkip;
				if (!mapHeaderNode->getString(stringToSkip)) {
					spdlog::warn("Invalid map NPC spawnfile tag");
					return false;
				}
				break;
			}
			default: {
				spdlog::warn("Unknown header attribute: {}", static_cast<int>(attribute));
				return false;
			}
		}
	}
	return true;
}
