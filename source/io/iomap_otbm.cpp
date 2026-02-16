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
#include <wx/mstream.h>
#include <wx/datstrm.h>

#include <format>
#include <fstream>
#include <vector>
#include <spdlog/spdlog.h>

#include "app/settings.h"
#include "ext/pugixml.hpp"
#include "ui/gui.h"
#include "ui/dialog_util.h"

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
#include "io/map_xml_io.h"
#include "io/otbm/item_serialization_otbm.h"

// New specialized serialization classes
#include "io/otbm/header_serialization_otbm.h"
#include "io/otbm/town_serialization_otbm.h"
#include "io/otbm/waypoint_serialization_otbm.h"
#include "io/otbm/tile_serialization_otbm.h"

using attribute_t = uint8_t;
using flags_t = uint32_t;

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

/* Entry level calls */

bool IOMapOTBM::getVersionInfo(const FileName& filename, MapVersion& out_ver) {

	DiskNodeFileReadHandle f(nstr(filename.GetFullPath()), StringVector(1, "OTBM"));
	if (!f.isOk()) {
		return false;
	}
	return getVersionInfo(&f, out_ver);
}

bool IOMapOTBM::getVersionInfo(NodeFileReadHandle* f, MapVersion& out_ver) {
	return HeaderSerializationOTBM::getVersionInfo(f, out_ver);
}

bool IOMapOTBM::loadMapFromDisk(Map& map, const FileName& filename) {
	spdlog::debug("Loading OTBM map from disk: {}", filename.GetFullPath().ToStdString());
	std::ifstream file(nstr(filename.GetFullPath()), std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		spdlog::error("Couldn't open file for reading: {}", filename.GetFullPath().ToStdString());
		return false;
	}

	const std::streamsize size = file.tellg();
	if (size < 4) {
		spdlog::error("File is too short to be an OTBM file.");
		return false;
	}

	// Memory optimization: larger than 512MB, stream from disk
	if (size > 512 * 1024 * 1024) {
		file.close();
		DiskNodeFileReadHandle f(nstr(filename.GetFullPath()), StringVector(1, "OTBM"));
		if (!f.isOk()) {
			spdlog::error("{}", f.getErrorMessage());
			return false;
		}
		if (!loadMap(map, f)) {
			return false;
		}
	} else {
		file.seekg(0, std::ios::beg);
		std::vector<uint8_t> buffer(size);
		if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
			spdlog::error("Failed to read file.");
			return false;
		}

		if (memcmp(buffer.data(), "OTBM", 4) != 0 && memcmp(buffer.data(), "\0\0\0\0", 4) != 0) {
			spdlog::error("File magic number not recognized");
			return false;
		}

		MemoryNodeFileReadHandle f(buffer.data() + 4, size - 4);
		if (!loadMap(map, f)) {
			return false;
		}
	}

	// Read auxiliary files
	auto loadAux = [&](bool (*func)(Map&, const FileName&), const std::string& suffix, std::string& target) {
		if (!func(map, filename)) {
			spdlog::warn("Failed to load {}", suffix);
			target = nstr(filename.GetName()) + "-" + suffix + ".xml";
		}
	};

	loadAux(&MapXMLIO::loadHouses, "house", map.housefile);
	loadAux(&MapXMLIO::loadSpawns, "spawn", map.spawnfile);
	loadAux(&MapXMLIO::loadWaypoints, "waypoint", map.waypointfile);

	return true;
}

bool IOMapOTBM::loadMap(Map& map, const FileName& filename) {
	return loadMapFromDisk(map, filename);
}

bool IOMapOTBM::loadMapRoot(Map& map, NodeFileReadHandle& f, BinaryNode*& root, BinaryNode*& mapHeaderNode) {
	return HeaderSerializationOTBM::loadMapRoot(map, f, version, root, mapHeaderNode);
}

bool IOMapOTBM::readMapAttributes(Map& map, BinaryNode* mapHeaderNode) {
	return HeaderSerializationOTBM::readMapAttributes(map, mapHeaderNode);
}

void IOMapOTBM::readMapNodes(Map& map, NodeFileReadHandle& f, BinaryNode* mapHeaderNode) {
	spdlog::debug("Starting to read map nodes...");
	int nodes_loaded = 0;

	for (BinaryNode* mapNode = mapHeaderNode->getChild(); mapNode != nullptr; mapNode = mapNode->advance()) {
		if (++nodes_loaded % 2048 == 0) {
			g_gui.SetLoadDone(static_cast<int32_t>(100.0 * f.tell() / f.size()));
		}

		uint8_t node_type;
		if (!mapNode->getByte(node_type)) {
			spdlog::warn("Invalid map node encountered (failed to read type byte)");
			continue;
		}

		switch (node_type) {
			case OTBM_TILE_AREA:
				readTileArea(map, mapNode);
				break;
			case OTBM_TOWNS:
				readTowns(map, mapNode);
				break;
			case OTBM_WAYPOINTS:
				readWaypoints(map, mapNode);
				break;
			default:
				break;
		}
	}
}

void IOMapOTBM::readTileArea(Map& map, BinaryNode* mapNode) {
	TileSerializationOTBM::readTileArea(*this, map, mapNode);
}

void IOMapOTBM::readTowns(Map& map, BinaryNode* mapNode) {
	TownSerializationOTBM::readTowns(map, mapNode);
}

void IOMapOTBM::readWaypoints(Map& map, BinaryNode* mapNode) {
	WaypointSerializationOTBM::readWaypoints(map, mapNode);
}

bool IOMapOTBM::loadMap(Map& map, NodeFileReadHandle& f) {
	BinaryNode* root = nullptr;
	BinaryNode* mapHeaderNode = nullptr;

	if (!loadMapRoot(map, f, root, mapHeaderNode)) {
		return false;
	}

	if (!readMapAttributes(map, mapHeaderNode)) {
		return false;
	}
	readMapNodes(map, f, mapHeaderNode);

	if (!f.isOk()) {
		spdlog::warn(f.getErrorMessage());
	}
	return true;
}

bool IOMapOTBM::saveMapToDisk(Map& map, const FileName& identifier) {
	DiskNodeFileWriteHandle f(
		nstr(identifier.GetFullPath()),
		(g_settings.getInteger(Config::SAVE_WITH_OTB_MAGIC_NUMBER) ? "OTBM" : std::string(4, '\0'))
	);

	if (!f.isOk()) {
		spdlog::error("Can not open file {} for writing", identifier.GetFullPath().ToStdString());
		return false;
	}

	if (!saveMap(map, f)) {
		return false;
	}

	g_gui.SetLoadDone(99, "Saving spawns...");
	if (!MapXMLIO::saveSpawns(map, identifier)) {
		spdlog::error("Failed to save spawns!");
		return false;
	}

	g_gui.SetLoadDone(99, "Saving houses...");
	if (!MapXMLIO::saveHouses(map, identifier)) {
		spdlog::error("Failed to save houses!");
		return false;
	}

	return true;
}

bool IOMapOTBM::saveMap(Map& map, const FileName& identifier) {
	return saveMapToDisk(map, identifier);
}

bool IOMapOTBM::saveMap(Map& map, NodeFileWriteHandle& f) {
	const MapVersion mapVersion = map.getVersion();

	f.addNode(0);
	{
		f.addU32(mapVersion.otbm);
		f.addU16(map.width);
		f.addU16(map.height);
		f.addU32(g_items.MajorVersion);
		f.addU32(g_items.MinorVersion);

		f.addNode(OTBM_MAP_DATA);
		{
			f.addU8(OTBM_ATTR_DESCRIPTION);
			f.addString(std::format("Saved with {} {}", __RME_APPLICATION_NAME__, __RME_VERSION__));

			f.addU8(OTBM_ATTR_DESCRIPTION);
			f.addString(map.description);

			auto addExtFile = [&](uint8_t attr, const std::string& path) {
				FileName tmp(wxstr(path));
				f.addU8(attr);
				f.addString(nstr(tmp.GetFullName()));
			};

			addExtFile(OTBM_ATTR_EXT_SPAWN_FILE, map.spawnfile);
			addExtFile(OTBM_ATTR_EXT_HOUSE_FILE, map.housefile);

			writeTileData(map, f);
			writeTowns(map, f);

			if (writeWaypoints(map, f, mapVersion) == WriteResult::SuccessWithUnsupportedVersion) {
				DialogUtil::PopupDialog(g_gui.root, "Warning", "Waypoints were saved, but they are not supported in OTBM 2!\nIf your map fails to load, consider removing all waypoints and saving again.\n\nThis warning can be disabled in file->preferences.", wxOK);
			}
		}
		f.endNode();
	}
	f.endNode();

	return true;
}

void IOMapOTBM::writeTileData(const Map& map, NodeFileWriteHandle& f) {
	TileSerializationOTBM::writeTileData(*this, map, f);
}

void IOMapOTBM::writeTowns(const Map& map, NodeFileWriteHandle& f) {
	TownSerializationOTBM::writeTowns(map, f);
}

IOMapOTBM::WriteResult IOMapOTBM::writeWaypoints(const Map& map, NodeFileWriteHandle& f, MapVersion mapVersion) {
	return WaypointSerializationOTBM::writeWaypoints(map, f, mapVersion);
}

void IOMapOTBM::serializeTile_OTBM(const IOMapOTBM& iomap, Tile* save_tile, NodeFileWriteHandle& f) {
	TileSerializationOTBM::serializeTile_OTBM(iomap, save_tile, f);
}
