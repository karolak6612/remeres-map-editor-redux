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

#include <wx/wfstream.h>
#include <wx/tarstrm.h>
#include <wx/zstream.h>
#include <wx/mstream.h>
#include <wx/datstrm.h>

#include <format>
#include <fstream>
#include <vector>

#include "app/settings.h"
#include "ui/gui.h"
#include "ui/dialog_util.h" // Loadbar

#include "game/creatures.h"
#include "game/creature.h"
#include "map/map.h"
#include "map/map_region.h"
#include "map/tile.h"
#include "game/item.h"
#include "game/complexitem.h"
#include "game/town.h"
#include "brushes/wall/wall_brush.h"

#include "io/iomap_otbm.h"
#include "io/archive_io.h"
#include "io/map_xml_io.h"
#include "io/item_serialization_otbm.h"
#include <spdlog/spdlog.h>

using attribute_t = uint8_t;
using flags_t = uint32_t;

// H4X
void reform(Map* map, Tile* tile, Item* item) {
	/*
	int aid = item->getActionID();
	int id = item->getID();
	int uid = item->getUniqueID();

	if(item->isDoor()) {
		item->eraseAttribute("aid");
		item->setAttribute("keyid", aid);
	}

	if((item->isDoor()) && tile && tile->getHouseID()) {
		Door* self = static_cast<Door*>(item);
		House* house = map->houses.getHouse(tile->getHouseID());
		self->setDoorID(house->getEmptyDoorID());
	}
	*/
}

// Item OTBM operations delegated to ItemSerializationOTBM
std::unique_ptr<Item> Item::Create_OTBM(const IOMap& maphandle, BinaryNode* stream) {
	return ItemSerializationOTBM::createFromStream(maphandle, stream);
}

bool Item::readItemAttribute_OTBM(const IOMap& maphandle, OTBM_ItemAttribute attr, BinaryNode* stream) {
	return ItemSerializationOTBM::readAttribute(maphandle, attr, stream, *this);
}

bool Item::unserializeAttributes_OTBM(const IOMap& maphandle, BinaryNode* stream) {
	return ItemSerializationOTBM::unserializeAttributes(maphandle, stream, *this);
}

bool Item::unserializeItemNode_OTBM(const IOMap& maphandle, BinaryNode* node) {
	return ItemSerializationOTBM::unserializeItemNode(maphandle, node, *this);
}

void Item::serializeItemAttributes_OTBM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	ItemSerializationOTBM::serializeItemAttributes(maphandle, stream, *this);
}

void Item::serializeItemCompact_OTBM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	ItemSerializationOTBM::serializeItemCompact(maphandle, stream, *this);
}

bool Item::serializeItemNode_OTBM(const IOMap& maphandle, NodeFileWriteHandle& file) const {
	return ItemSerializationOTBM::serializeItemNode(maphandle, file, *this);
}

bool Teleport::readItemAttribute_OTBM(const IOMap& maphandle, OTBM_ItemAttribute attribute, BinaryNode* stream) {
	return ItemSerializationOTBM::readAttribute(maphandle, attribute, stream, *this);
}

void Teleport::serializeItemAttributes_OTBM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	ItemSerializationOTBM::serializeItemAttributes(maphandle, stream, *this);
}

bool Door::readItemAttribute_OTBM(const IOMap& maphandle, OTBM_ItemAttribute attribute, BinaryNode* stream) {
	return ItemSerializationOTBM::readAttribute(maphandle, attribute, stream, *this);
}

void Door::serializeItemAttributes_OTBM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	ItemSerializationOTBM::serializeItemAttributes(maphandle, stream, *this);
}

bool Depot::readItemAttribute_OTBM(const IOMap& maphandle, OTBM_ItemAttribute attribute, BinaryNode* stream) {
	return ItemSerializationOTBM::readAttribute(maphandle, attribute, stream, *this);
}

void Depot::serializeItemAttributes_OTBM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	ItemSerializationOTBM::serializeItemAttributes(maphandle, stream, *this);
}

bool Container::unserializeItemNode_OTBM(const IOMap& maphandle, BinaryNode* node) {
	return ItemSerializationOTBM::unserializeItemNode(maphandle, node, *this);
}

bool Container::serializeItemNode_OTBM(const IOMap& maphandle, NodeFileWriteHandle& file) const {
	return ItemSerializationOTBM::serializeItemNode(maphandle, file, *this);
}

bool Podium::readItemAttribute_OTBM(const IOMap& maphandle, OTBM_ItemAttribute attribute, BinaryNode* stream) {
	return ItemSerializationOTBM::readAttribute(maphandle, attribute, stream, *this);
}

void Podium::serializeItemAttributes_OTBM(const IOMap& maphandle, NodeFileWriteHandle& stream) const {
	ItemSerializationOTBM::serializeItemAttributes(maphandle, stream, *this);
}

/*
	OTBM_ROOTV1
	|
	|--- OTBM_MAP_DATA
	|	|
	|	|--- OTBM_TILE_AREA
	|	|	|--- OTBM_TILE
	|	|	|--- OTBM_TILE_SQUARE (not implemented)
	|	|	|--- OTBM_TILE_REF (not implemented)
	|	|	|--- OTBM_HOUSETILE
	|	|
	|	|--- OTBM_SPAWNS (not implemented)
	|	|	|--- OTBM_SPAWN_AREA (not implemented)
	|	|	|--- OTBM_MONSTER (not implemented)
	|	|
	|	|--- OTBM_TOWNS
	|		|--- OTBM_TOWN
	|
	|--- OTBM_ITEM_DEF (not implemented)
*/

bool IOMapOTBM::getVersionInfo(const FileName& filename, MapVersion& out_ver) {
#ifdef OTGZ_SUPPORT
	if (filename.GetExt() == "otgz") {
		ArchiveReader reader;
		if (!reader.open(nstr(filename.GetFullPath()))) {
			return false;
		}

		auto otbmBuffer = reader.extractFile("world/map.otbm");
		if (!otbmBuffer || otbmBuffer->size() < 4) {
			return false;
		}

		// Create a read handle on it (skip 4 byte magic)
		auto f = std::make_shared<MemoryNodeFileReadHandle>(otbmBuffer->data() + 4, otbmBuffer->size() - 4);
		return getVersionInfo(f.get(), out_ver);
	}
#endif

	// Just open a disk-based read handle
	DiskNodeFileReadHandle f(nstr(filename.GetFullPath()), StringVector(1, "OTBM"));
	if (!f.isOk()) {
		return false;
	}
	return getVersionInfo(&f, out_ver);
}

bool IOMapOTBM::getVersionInfo(NodeFileReadHandle* f, MapVersion& out_ver) {
	BinaryNode* root = f->getRootNode();
	if (!root) {
		return false;
	}

	root->skip(1); // Skip the type byte

	uint16_t u16;
	uint32_t u32;

	if (!root->getU32(u32)) { // Version
		return false;
	}
	out_ver.otbm = (MapVersionID)u32;

	root->getU16(u16); // map size X
	root->getU16(u16); // map size Y
	root->getU32(u32); // OTB major version

	if (!root->getU32(u32)) { // OTB minor version
		return false;
	}

	out_ver.client = ClientVersionID(u32);
	return true;
}

bool IOMapOTBM::loadMapFromOTGZ(Map& map, const FileName& filename) {
#ifdef OTGZ_SUPPORT
	ArchiveReader reader;
	if (!reader.open(nstr(filename.GetFullPath()))) {
		return false;
	}

	g_gui.SetLoadDone(0, "Decompressing archive...");

	// Load OTBM
	if (auto otbmBuffer = reader.extractFile("world/map.otbm")) {
		if (otbmBuffer->size() < 4) {
			return false;
		}

		g_gui.SetLoadDone(0, "Loading OTBM map...");
		MemoryNodeFileReadHandle f(otbmBuffer->data() + 4, otbmBuffer->size() - 4);
		if (!loadMap(map, f)) {
			error("Could not load OTBM file inside archive");
			return false;
		}
	} else {
		error("OTBM file not found inside archive.");
		return false;
	}

	// Load Houses
	if (auto houseBuffer = reader.extractFile("world/houses.xml")) {
		pugi::xml_document doc;
		if (doc.load_buffer(houseBuffer->data(), houseBuffer->size())) {
			if (!loadHouses(map, doc)) {
				warning("Failed to load houses.");
			}
		} else {
			warning("Failed to load houses due to XML parse error.");
		}
	}

	// Load Spawns
	if (auto spawnBuffer = reader.extractFile("world/spawns.xml")) {
		pugi::xml_document doc;
		if (doc.load_buffer(spawnBuffer->data(), spawnBuffer->size())) {
			if (!loadSpawns(map, doc)) {
				warning("Failed to load spawns.");
			}
		} else {
			warning("Failed to load spawns due to XML parse error.");
		}
	}

	return true;
#else
	return false;
#endif
}

bool IOMapOTBM::loadMapFromDisk(Map& map, const FileName& filename) {
	std::ifstream file(nstr(filename.GetFullPath()), std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		std::string err = std::format("Couldn't open file for reading: {}", filename.GetFullPath().ToStdString());
		error("%s", err.c_str());
		return false;
	}

	std::streamsize size = file.tellg();
	if (size < 4) {
		error("File is too short to be an OTBM file.");
		return false;
	}

	// Memory optimization/Safety:
	// For files larger than 512MB, stream from disk to avoid OOM on systems with limited RAM.
	// Otherwise, read the entire file into memory for faster parsing.
	if (size > 512 * 1024 * 1024) {
		file.close();
		DiskNodeFileReadHandle f(nstr(filename.GetFullPath()), StringVector(1, "OTBM"));
		if (!f.isOk()) {
			error("%s", wxstr(f.getErrorMessage()));
			return false;
		}
		if (!loadMap(map, f)) {
			return false;
		}
	} else {
		file.seekg(0, std::ios::beg);

		std::vector<uint8_t> buffer(size);
		if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
			error("Failed to read file.");
			return false;
		}

		// Verify magic bytes
		if (memcmp(buffer.data(), "OTBM", 4) != 0 && memcmp(buffer.data(), "\0\0\0\0", 4) != 0) {
			error("File magic number not recognized");
			return false;
		}

		// Create memory handle (skips first 4 bytes)
		MemoryNodeFileReadHandle f(buffer.data() + 4, size - 4);
		if (!loadMap(map, f)) {
			return false;
		}
	}

	// Read auxilliary files
	if (!loadHouses(map, filename)) {
		warning("Failed to load houses.");
		map.housefile = nstr(filename.GetName()) + "-house.xml";
	}
	if (!loadSpawns(map, filename)) {
		warning("Failed to load spawns.");
		map.spawnfile = nstr(filename.GetName()) + "-spawn.xml";
	}
	if (!loadWaypoints(map, filename)) {
		// just assume the map did not have this file before
		// warning("Failed to load waypoints.");
		map.waypointfile = nstr(filename.GetName()) + "-waypoint.xml";
	}
	return true;
}

bool IOMapOTBM::loadMap(Map& map, const FileName& filename) {
#ifdef OTGZ_SUPPORT
	if (filename.GetExt() == "otgz") {
		return loadMapFromOTGZ(map, filename);
	}
#endif
	return loadMapFromDisk(map, filename);
}

bool IOMapOTBM::loadMapRoot(Map& map, NodeFileReadHandle& f, BinaryNode*& root, BinaryNode*& mapHeaderNode) {
	root = f.getRootNode();
	if (!root) {
		error("Could not read root node.");
		return false;
	}
	root->skip(1); // Skip the type byte

	uint8_t u8;
	uint16_t u16;
	uint32_t u32;

	if (!root->getU32(u32)) {
		return false;
	}

	version.otbm = (MapVersionID)u32;

	if (version.otbm > MAP_OTBM_4) {
		// Failed to read version
		if (DialogUtil::PopupDialog("Map error", "The loaded map appears to be a OTBM format that is not supported by the editor."
												 "Do you still want to attempt to load the map?",
									wxYES | wxNO)
			== wxID_YES) {
			warning("Unsupported or damaged map version");
		} else {
			error("Unsupported OTBM version, could not load map");
			return false;
		}
	}

	if (!root->getU16(u16)) {
		return false;
	}

	map.width = u16;
	if (!root->getU16(u16)) {
		return false;
	}

	map.height = u16;

	if (!root->getU32(u32) || u32 > (unsigned long)g_items.MajorVersion) { // OTB major version
		if (DialogUtil::PopupDialog("Map error", "The loaded map appears to be a items.otb format that deviates from the "
												 "items.otb loaded by the editor. Do you still want to attempt to load the map?",
									wxYES | wxNO)
			== wxID_YES) {
			warning("Unsupported or damaged map version");
		} else {
			error("Outdated items.otb, could not load map");
			return false;
		}
	}

	if (!root->getU32(u32) || u32 > (unsigned long)g_items.MinorVersion) { // OTB minor version
		warning("This editor needs an updated items.otb version");
	}
	version.client = (ClientVersionID)u32;

	mapHeaderNode = root->getChild();
	if (mapHeaderNode == nullptr || !mapHeaderNode->getByte(u8) || u8 != OTBM_MAP_DATA) {
		error("Could not get root child node. Cannot recover from fatal error!");
		return false;
	}
	return true;
}

void IOMapOTBM::readMapAttributes(Map& map, BinaryNode* mapHeaderNode) {
	uint8_t attribute;
	while (mapHeaderNode->getU8(attribute)) {
		switch (attribute) {
			case OTBM_ATTR_DESCRIPTION: {
				if (!mapHeaderNode->getString(map.description)) {
					warning("Invalid map description tag");
				}
				break;
			}
			case OTBM_ATTR_EXT_SPAWN_FILE: {
				if (!mapHeaderNode->getString(map.spawnfile)) {
					warning("Invalid map spawnfile tag");
				}
				break;
			}
			case OTBM_ATTR_EXT_HOUSE_FILE: {
				if (!mapHeaderNode->getString(map.housefile)) {
					warning("Invalid map housefile tag");
				}
				break;
			}
			case OTBM_ATTR_EXT_SPAWN_NPC_FILE: {
				// compatibility: skip Canary RME NPC spawn file tag
				std::string stringToSkip;
				if (!mapHeaderNode->getString(stringToSkip)) {
					warning("Invalid map housefile tag");
				}
				break;
			}
			default: {
				warning("Unknown header node.");
				break;
			}
		}
	}
}

void IOMapOTBM::readMapNodes(Map& map, NodeFileReadHandle& f, BinaryNode* mapHeaderNode) {
	int nodes_loaded = 0;

	for (BinaryNode* mapNode = mapHeaderNode->getChild(); mapNode != nullptr; mapNode = mapNode->advance()) {
		++nodes_loaded;
		if (nodes_loaded % 2048 == 0) {
			g_gui.SetLoadDone(static_cast<int32_t>(100.0 * f.tell() / f.size()));
		}

		uint8_t node_type;
		if (!mapNode->getByte(node_type)) {
			warning("Invalid map node");
			continue;
		}
		if (node_type == OTBM_TILE_AREA) {
			readTileArea(map, mapNode);
		} else if (node_type == OTBM_TOWNS) {
			readTowns(map, mapNode);
		} else if (node_type == OTBM_WAYPOINTS) {
			readWaypoints(map, mapNode);
		}
	}
}

void IOMapOTBM::readTileArea(Map& map, BinaryNode* mapNode) {
	uint16_t base_x, base_y;
	uint8_t base_z;
	if (!mapNode->getU16(base_x) || !mapNode->getU16(base_y) || !mapNode->getU8(base_z)) {
		warning("Invalid map node, no base coordinate");
		return;
	}

	for (BinaryNode* tileNode = mapNode->getChild(); tileNode != nullptr; tileNode = tileNode->advance()) {
		Tile* tile = nullptr;
		uint8_t tile_type;
		if (!tileNode->getByte(tile_type)) {
			warning("Invalid tile type");
			continue;
		}
		if (tile_type == OTBM_TILE || tile_type == OTBM_HOUSETILE) {
			uint8_t x_offset, y_offset;
			if (!tileNode->getU8(x_offset) || !tileNode->getU8(y_offset)) {
				warning("Could not read position of tile");
				continue;
			}
			const Position pos(base_x + x_offset, base_y + y_offset, base_z);

			if (map.getTile(pos)) {
				warning(wxstr(std::format("Duplicate tile at {}:{}:{}, discarding duplicate", pos.x, pos.y, pos.z)));
				continue;
			}

			tile = map.createTile(pos.x, pos.y, pos.z);
			House* house = nullptr;
			if (tile_type == OTBM_HOUSETILE) {
				uint32_t house_id;
				if (!tileNode->getU32(house_id)) {
					warning("House tile without house data, discarding tile");
					continue;
				}
				if (house_id) {
					house = map.houses.getHouse(house_id);
					if (!house) {
						auto new_house = std::make_unique<House>(map);
						house = new_house.get();
						new_house->setID(house_id);
						map.houses.addHouse(std::move(new_house));
					}
				} else {
					warning("Invalid house id from tile %d:%d:%d", pos.x, pos.y, pos.z);
				}
			}

			uint8_t attribute;
			while (tileNode->getU8(attribute)) {
				switch (attribute) {
					case OTBM_ATTR_TILE_FLAGS: {
						uint32_t flags = 0;
						if (!tileNode->getU32(flags)) {
							warning(wxstr(std::format("Invalid tile flags of tile on {}:{}:{}", pos.x, pos.y, pos.z)));
						}
						tile->setMapFlags(flags);
						break;
					}
					case OTBM_ATTR_ITEM: {
						auto item = ItemSerializationOTBM::createFromStream(*this, tileNode);
						if (item == nullptr) {
							warning(wxstr(std::format("Invalid item at tile {}:{}:{}", pos.x, pos.y, pos.z)));
						}
						tile->addItem(std::move(item));
						break;
					}
					default: {
						warning(wxstr(std::format("Unknown tile attribute at {}:{}:{}", pos.x, pos.y, pos.z)));
						break;
					}
				}
			}

			for (BinaryNode* itemNode = tileNode->getChild(); itemNode != nullptr; itemNode = itemNode->advance()) {
				std::unique_ptr<Item> item;
				uint8_t item_type;
				if (!itemNode->getByte(item_type)) {
					warning(wxstr(std::format("Unknown item type {}:{}:{}", pos.x, pos.y, pos.z)));
					continue;
				}
				if (item_type == OTBM_ITEM) {
					item = ItemSerializationOTBM::createFromStream(*this, itemNode);
					if (item) {
						if (!ItemSerializationOTBM::unserializeItemNode(*this, itemNode, *item)) {
							warning(wxstr(std::format("Couldn't unserialize item attributes at {}:{}:{}", pos.x, pos.y, pos.z)));
						}
						// reform(&map, tile, item.get());
						tile->addItem(std::move(item));
					}
				} else {
					warning("Unknown type of tile child node");
				}
			}

			tile->update();
			if (house) {
				house->addTile(tile);
			}

		} else {
			warning("Unknown type of tile node");
		}
	}
}

void IOMapOTBM::readTowns(Map& map, BinaryNode* mapNode) {
	for (BinaryNode* townNode = mapNode->getChild(); townNode != nullptr; townNode = townNode->advance()) {
		Town* town = nullptr;
		uint8_t town_type;
		if (!townNode->getByte(town_type)) {
			warning("Invalid town type (1)");
			continue;
		}
		if (town_type != OTBM_TOWN) {
			warning("Invalid town type (2)");
			continue;
		}
		uint32_t town_id;
		if (!townNode->getU32(town_id)) {
			warning("Invalid town id");
			continue;
		}

		town = map.towns.getTown(town_id);
		if (town) {
			warning(wxstr(std::format("Duplicate town id {}, discarding duplicate", town_id)));
			continue;
		} else {
			auto new_town = std::make_unique<Town>(town_id);
			town = new_town.get();
			if (!map.towns.addTown(std::move(new_town))) {
				continue;
			}
		}
		std::string town_name;
		if (!townNode->getString(town_name)) {
			warning("Invalid town name");
			continue;
		}
		town->setName(town_name);
		Position pos;
		uint16_t x;
		uint16_t y;
		uint8_t z;
		if (!townNode->getU16(x) || !townNode->getU16(y) || !townNode->getU8(z)) {
			warning("Invalid town temple position");
			continue;
		}
		pos.x = x;
		pos.y = y;
		pos.z = z;
		town->setTemplePosition(pos);
		map.getOrCreateTile(pos)->getLocation()->increaseTownCount();
	}
}

void IOMapOTBM::readWaypoints(Map& map, BinaryNode* mapNode) {
	for (BinaryNode* waypointNode = mapNode->getChild(); waypointNode != nullptr; waypointNode = waypointNode->advance()) {
		uint8_t waypoint_type;
		if (!waypointNode->getByte(waypoint_type)) {
			warning("Invalid waypoint type (1)");
			continue;
		}
		if (waypoint_type != OTBM_WAYPOINT) {
			warning("Invalid waypoint type (2)");
			continue;
		}

		Waypoint wp;

		if (!waypointNode->getString(wp.name)) {
			warning("Invalid waypoint name");
			continue;
		}
		uint16_t x;
		uint16_t y;
		uint8_t z;
		if (!waypointNode->getU16(x) || !waypointNode->getU16(y) || !waypointNode->getU8(z)) {
			warning("Invalid waypoint position");
			continue;
		}
		wp.pos.x = x;
		wp.pos.y = y;
		wp.pos.z = z;

		map.waypoints.addWaypoint(std::make_unique<Waypoint>(wp));
	}
}

bool IOMapOTBM::loadMap(Map& map, NodeFileReadHandle& f) {
	BinaryNode* root = nullptr;
	BinaryNode* mapHeaderNode = nullptr;

	if (!loadMapRoot(map, f, root, mapHeaderNode)) {
		return false;
	}

	readMapAttributes(map, mapHeaderNode);

	readMapNodes(map, f, mapHeaderNode);

	if (!f.isOk()) {
		warning(wxstr(f.getErrorMessage()).wc_str());
	}
	return true;
}

bool IOMapOTBM::loadSpawns(Map& map, const FileName& dir) {
	return MapXMLIO::loadSpawns(map, dir);
}

bool IOMapOTBM::loadSpawns(Map& map, pugi::xml_document& doc) {
	return MapXMLIO::loadSpawns(map, doc);
}

bool IOMapOTBM::loadHouses(Map& map, const FileName& dir) {
	return MapXMLIO::loadHouses(map, dir);
}

bool IOMapOTBM::loadHouses(Map& map, pugi::xml_document& doc) {
	return MapXMLIO::loadHouses(map, doc);
}

bool IOMapOTBM::loadWaypoints(Map& map, const FileName& dir) {
	return MapXMLIO::loadWaypoints(map, dir);
}

bool IOMapOTBM::loadWaypoints(Map& map, pugi::xml_document& doc) {
	return MapXMLIO::loadWaypoints(map, doc);
}

bool IOMapOTBM::saveSpawns(Map& map, const FileName& dir) {
	return MapXMLIO::saveSpawns(map, dir);
}

bool IOMapOTBM::saveSpawns(Map& map, pugi::xml_document& doc) {
	return MapXMLIO::saveSpawns(map, doc);
}

bool IOMapOTBM::saveHouses(Map& map, const FileName& dir) {
	return MapXMLIO::saveHouses(map, dir);
}

bool IOMapOTBM::saveHouses(Map& map, pugi::xml_document& doc) {
	return MapXMLIO::saveHouses(map, doc);
}

bool IOMapOTBM::saveWaypoints(Map& map, const FileName& dir) {
	return MapXMLIO::saveWaypoints(map, dir);
}

bool IOMapOTBM::saveWaypoints(Map& map, pugi::xml_document& doc) {
	return MapXMLIO::saveWaypoints(map, doc);
}

bool IOMapOTBM::saveMapToOTGZ(Map& map, const FileName& identifier) {
#ifdef OTGZ_SUPPORT
	ArchiveWriter writer;
	if (!writer.open(nstr(identifier.GetFullPath()))) {
		return false;
	}

	g_gui.SetLoadDone(0, "Saving spawns...");

	pugi::xml_document spawnDoc;
	if (saveSpawns(map, spawnDoc)) {
		std::ostringstream streamData;
		spawnDoc.save(streamData, "", pugi::format_raw, pugi::encoding_utf8);
		std::string xmlData = streamData.str();
		writer.addFile("world/spawns.xml", std::span(reinterpret_cast<const uint8_t*>(xmlData.data()), xmlData.size()));
	}

	g_gui.SetLoadDone(0, "Saving houses...");

	pugi::xml_document houseDoc;
	if (saveHouses(map, houseDoc)) {
		std::ostringstream streamData;
		houseDoc.save(streamData, "", pugi::format_raw, pugi::encoding_utf8);
		std::string xmlData = streamData.str();
		writer.addFile("world/houses.xml", std::span(reinterpret_cast<const uint8_t*>(xmlData.data()), xmlData.size()));
	}

	g_gui.SetLoadDone(0, "Saving OTBM map...");

	MemoryNodeFileWriteHandle otbmWriter;
	saveMap(map, otbmWriter);

	g_gui.SetLoadDone(75, "Compressing...");

	std::vector<uint8_t> otbmData;
	otbmData.reserve(otbmWriter.getSize() + 4);
	const char* magic = "OTBM";
	otbmData.insert(otbmData.end(), magic, magic + 4);
	otbmData.insert(otbmData.end(), otbmWriter.getMemory(), otbmWriter.getMemory() + otbmWriter.getSize());

	writer.addFile("world/map.otbm", otbmData);

	g_gui.DestroyLoadBar();
	return true;
#else
	return false;
#endif
}

bool IOMapOTBM::saveMapToDisk(Map& map, const FileName& identifier) {
	DiskNodeFileWriteHandle f(
		nstr(identifier.GetFullPath()),
		(g_settings.getInteger(Config::SAVE_WITH_OTB_MAGIC_NUMBER) ? "OTBM" : std::string(4, '\0'))
	);

	if (!f.isOk()) {
		error("Can not open file %s for writing", (const char*)identifier.GetFullPath().mb_str(wxConvUTF8));
		return false;
	}

	if (!saveMap(map, f)) {
		return false;
	}

	g_gui.SetLoadDone(99, "Saving spawns...");
	saveSpawns(map, identifier);

	g_gui.SetLoadDone(99, "Saving houses...");
	saveHouses(map, identifier);

	return true;
}

bool IOMapOTBM::saveMap(Map& map, const FileName& identifier) {
#ifdef OTGZ_SUPPORT
	if (identifier.GetExt() == "otgz") {
		return saveMapToOTGZ(map, identifier);
	}
#endif
	return saveMapToDisk(map, identifier);
}

bool IOMapOTBM::saveMap(Map& map, NodeFileWriteHandle& f) {
	/* STOP!
	 * Before you even think about modifying this, please reconsider.
	 * while adding stuff to the binary format may be "cool", you'll
	 * inevitably make it incompatible with any future releases of
	 * the map editor, meaning you cannot reuse your map. Before you
	 * try to modify this, PLEASE consider using an external file
	 * like spawns.xml or houses.xml, as that will be MUCH easier
	 * to port to newer versions of the editor than a custom binary
	 * format.
	 */

	bool waypointsWarning = false;

	FileName tmpName;
	MapVersion mapVersion = map.getVersion();

	f.addNode(0);
	{
		f.addU32(mapVersion.otbm); // Version

		f.addU16(map.width);
		f.addU16(map.height);

		f.addU32(g_items.MajorVersion);
		f.addU32(g_items.MinorVersion);

		f.addNode(OTBM_MAP_DATA);
		{
			f.addByte(OTBM_ATTR_DESCRIPTION);
			// Neither SimOne's nor OpenTibia cares for additional description tags
			f.addString("Saved with " + __RME_APPLICATION_NAME__ + " " + __RME_VERSION__);

			f.addU8(OTBM_ATTR_DESCRIPTION);
			f.addString(map.description);

			tmpName.Assign(wxstr(map.spawnfile));
			f.addU8(OTBM_ATTR_EXT_SPAWN_FILE);
			f.addString(nstr(tmpName.GetFullName()));

			tmpName.Assign(wxstr(map.housefile));
			f.addU8(OTBM_ATTR_EXT_HOUSE_FILE);
			f.addString(nstr(tmpName.GetFullName()));

			writeTileData(map, f);
			writeTowns(map, f);
			waypointsWarning = writeWaypoints(map, f, mapVersion);
		}
		f.endNode();
	}
	f.endNode();

	if (waypointsWarning) {
		DialogUtil::PopupDialog(g_gui.root, "Warning", "Waypoints were saved, but they are not supported in OTBM 2!\nIf your map fails to load, consider removing all waypoints and saving again.\n\nThis warning can be disabled in file->preferences.", wxOK);
	}
	return true;
}

void IOMapOTBM::writeTileData(const Map& map, NodeFileWriteHandle& f) {
	const IOMapOTBM& self = *this;
	uint32_t tiles_saved = 0;
	bool first = true;

	int local_x = -1, local_y = -1, local_z = -1;

	// Iterate tiles in spatial order to optimize OTBM node grouping
	auto sorted_cells = map.getGrid().getSortedCells();
	for (const auto& sorted_cell : sorted_cells) {
		SpatialHashGrid::GridCell* cell = sorted_cell.cell;
		if (!cell) {
			continue;
		}

		for (int i = 0; i < SpatialHashGrid::NODES_IN_CELL; ++i) {
			MapNode* node = cell->nodes[i].get();
			if (!node) {
				continue;
			}

			for (int j = 0; j < MAP_LAYERS; ++j) {
				Floor* floor = node->getFloor(j);
				if (!floor) {
					continue;
				}

				for (int k = 0; k < SpatialHashGrid::TILES_PER_NODE; ++k) {
					TileLocation& loc = floor->locs[k];
					Tile* save_tile = loc.get();

					if (!save_tile) {
						continue;
					}

					// Update progressbar
					++tiles_saved;
					if (tiles_saved % 8192 == 0) {
						uint64_t total_tiles = map.getTileCount();
						if (total_tiles > 0) {
							int progress = std::min(100, int(tiles_saved / double(total_tiles) * 100.0));
							g_gui.SetLoadDone(progress);
						}
					}

					// Is it an empty tile that we can skip? (Leftovers...)
					if (save_tile->size() == 0) {
						continue;
					}

					const Position& pos = save_tile->getPosition();

					// Decide if new node should be created
					if (pos.x < local_x || pos.x >= local_x + 256 || pos.y < local_y || pos.y >= local_y + 256 || pos.z != local_z) {
						// End last node
						if (!first) {
							f.endNode();
						}
						first = false;

						// Start new node
						f.addNode(OTBM_TILE_AREA);
						f.addU16(local_x = pos.x & 0xFF00);
						f.addU16(local_y = pos.y & 0xFF00);
						f.addU8(local_z = pos.z);
					}

					serializeTile_OTBM(save_tile, f, self);
				}
			}
		}
	}

	// Only close the last node if one has actually been created
	if (!first) {
		f.endNode();
	}
}

void IOMapOTBM::writeTowns(const Map& map, NodeFileWriteHandle& f) {
	f.addNode(OTBM_TOWNS);
	for (const auto& townEntry : map.towns) {
		Town* town = townEntry.second.get();
		const Position& townPosition = town->getTemplePosition();
		f.addNode(OTBM_TOWN);
		f.addU32(town->getID());
		f.addString(town->getName());
		f.addU16(townPosition.x);
		f.addU16(townPosition.y);
		f.addU8(townPosition.z);
		f.endNode();
	}
	f.endNode();
}

bool IOMapOTBM::writeWaypoints(const Map& map, NodeFileWriteHandle& f, MapVersion mapVersion) {
	bool waypointsWarning = false;
	bool supportWaypoints = mapVersion.otbm >= MAP_OTBM_3;
	if (supportWaypoints || !map.waypoints.waypoints.empty()) {
		if (!supportWaypoints) {
			waypointsWarning = true;
		}

		f.addNode(OTBM_WAYPOINTS);
		for (const auto& waypointEntry : map.waypoints) {
			Waypoint* waypoint = waypointEntry.second.get();
			f.addNode(OTBM_WAYPOINT);
			f.addString(waypoint->name);
			f.addU16(waypoint->pos.x);
			f.addU16(waypoint->pos.y);
			f.addU8(waypoint->pos.z);
			f.endNode();
		}
		f.endNode();
	}
	return waypointsWarning;
}

void IOMapOTBM::serializeTile_OTBM(Tile* save_tile, NodeFileWriteHandle& f, const IOMapOTBM& self) {
	f.addNode(save_tile->isHouseTile() ? OTBM_HOUSETILE : OTBM_TILE);

	f.addU8(save_tile->getX() & 0xFF);
	f.addU8(save_tile->getY() & 0xFF);

	if (save_tile->isHouseTile()) {
		f.addU32(save_tile->getHouseID());
	}

	if (save_tile->getMapFlags()) {
		f.addByte(OTBM_ATTR_TILE_FLAGS);
		f.addU32(save_tile->getMapFlags());
	}

	if (save_tile->ground) {
		Item* ground = save_tile->ground.get();
		if (ground->isMetaItem()) {
			// Do nothing, we don't save metaitems...
		} else if (ground->hasBorderEquivalent()) {
			bool found = false;
			for (const auto& item : save_tile->items) {
				if (item->getGroundEquivalent() == ground->getID()) {
					// Found equivalent
					found = true;
					break;
				}
			}

			if (!found) {
				ItemSerializationOTBM::serializeItemNode(self, f, *ground);
			}
		} else if (ground->isComplex()) {
			ItemSerializationOTBM::serializeItemNode(self, f, *ground);
		} else {
			f.addByte(OTBM_ATTR_ITEM);
			ItemSerializationOTBM::serializeItemCompact(self, f, *ground);
		}
	}

	for (const auto& item : save_tile->items) {
		if (!item->isMetaItem()) {
			ItemSerializationOTBM::serializeItemNode(self, f, *item);
		}
	}

	f.endNode();
}
