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

#include "ui/gui.h" // loadbar

#include "map/map.h"
#include "map/map_converter.h"
#include "map/map_spawn_manager.h"

#include <sstream>
#include <algorithm>
#include <ranges>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include <format>
#include <spdlog/spdlog.h>

bool ZoneRegistry::addZone(const std::string& name, uint16_t id) {
	if (name.empty() || id == 0) {
		return false;
	}

	if (const auto existing = findId(name); existing && *existing != id) {
		return false;
	}

	if (const auto existing_name = findName(id); !existing_name.empty() && existing_name != name) {
		return false;
	}

	name_to_id[name] = id;
	id_to_name[id] = name;
	return true;
}

bool ZoneRegistry::removeZone(uint16_t id) {
	const auto it = id_to_name.find(id);
	if (it == id_to_name.end()) {
		return false;
	}
	name_to_id.erase(it->second);
	id_to_name.erase(it);
	return true;
}

void ZoneRegistry::clear() {
	name_to_id.clear();
	id_to_name.clear();
}

std::optional<uint16_t> ZoneRegistry::findId(const std::string& name) const {
	if (const auto it = name_to_id.find(name); it != name_to_id.end()) {
		return it->second;
	}
	return std::nullopt;
}

std::string ZoneRegistry::findName(uint16_t id) const {
	if (const auto it = id_to_name.find(id); it != id_to_name.end()) {
		return it->second;
	}
	return {};
}

uint16_t ZoneRegistry::ensureZone(const std::string& name) {
	if (const auto existing = findId(name)) {
		return *existing;
	}

	const uint16_t id = nextFreeId();
	addZone(name, id);
	return id;
}

uint16_t ZoneRegistry::nextFreeId() const {
	uint16_t candidate = 1;
	while (id_to_name.contains(candidate)) {
		++candidate;
	}
	return candidate;
}

namespace {
	static std::atomic_uint64_t g_nextMapGeneration { 1 };
}

Map::Map() :
	BaseMap(),
	width(512),
	height(512),
	houses(*this),
	has_changed(false),
	unnamed(false),
	waypoints(*this),
	generation(g_nextMapGeneration.fetch_add(1, std::memory_order_relaxed)) {
	spdlog::info("Map created [Map={}]", static_cast<void*>(this));
	// Earliest version possible
	// Caller is responsible for converting us to proper version
	mapVersion.otbm = MAP_OTBM_1;
	mapVersion.client = OTB_VERSION_NONE;
}

void Map::initializeEmpty() {
	spdlog::info("Map::initializeEmpty [Map={}]", static_cast<void*>(this));
	generation = g_nextMapGeneration.fetch_add(1, std::memory_order_relaxed);
	height = 2048;
	width = 2048;

	static int unnamed_counter = 0;

	std::string sname = std::format("Untitled-{}", ++unnamed_counter);
	name = sname + ".otbm";
	spawnfile = sname + "-spawn.xml";
	spawnnpcfile = sname + "-npc.xml";
	housefile = sname + "-house.xml";
	zonefile = sname + "-zones.xml";
	waypointfile = sname + "-waypoint.xml";
	description = "No map description available.";
	unnamed = true;

	doChange();
}

Map::~Map() {
	spdlog::info("Map destroying [Map={}]", static_cast<void*>(this));
}

bool Map::open(const std::string& file) {
	if (file == filename) {
		return true; // Do not reopen ourselves!
	}

	generation = g_nextMapGeneration.fetch_add(1, std::memory_order_relaxed);
	tilecount = 0;

	IOMapOTBM maploader(getVersion());

	bool success = maploader.loadMap(*this, wxstr(file));

	mapVersion = maploader.version;

	warnings = maploader.getWarnings();

	if (!success) {
		error = maploader.getError();
		return false;
	}

	has_changed = false;

	wxFileName fn = wxstr(file);
	filename = fn.GetFullPath().mb_str(wxConvUTF8);
	name = fn.GetFullName().mb_str(wxConvUTF8);

	// convert(getReplacementMapClassic(), true);

	return true;
}

bool Map::convert(MapVersion to, bool showdialog) {
	return MapConverter::convert(*this, to, showdialog);
}

bool Map::convert(const ConversionMap& rm, bool showdialog) {
	return MapConverter::convert(*this, rm, showdialog);
}

void Map::cleanInvalidTiles(bool showdialog) {
	MapConverter::cleanInvalidTiles(*this, showdialog);
}

void Map::cleanInvalidZones(bool showdialog) {
	MapConverter::cleanInvalidZones(*this, showdialog);
}

void Map::convertHouseTiles(uint32_t fromId, uint32_t toId) {
	MapConverter::convertHouseTiles(*this, fromId, toId);
}

MapVersion Map::getVersion() const {
	return mapVersion;
}

bool Map::hasChanged() const {
	return has_changed;
}

bool Map::doChange() {
	bool doupdate = !has_changed;
	has_changed = true;
	return doupdate;
}

bool Map::clearChanges() {
	bool doupdate = has_changed;
	has_changed = false;
	return doupdate;
}

bool Map::hasFile() const {
	return filename != "";
}

void Map::setWidth(int new_width) {
	width = std::clamp(new_width, 64, 65000);
}

void Map::setHeight(int new_height) {
	height = std::clamp(new_height, 64, 65000);
}
void Map::setMapDescription(const std::string& new_description) {
	description = new_description;
}

void Map::setHouseFilename(const std::string& new_housefile) {
	housefile = new_housefile;
	unnamed = false;
}

void Map::setSpawnFilename(const std::string& new_spawnfile) {
	spawnfile = new_spawnfile;
	unnamed = false;
}

void Map::setSpawnNpcFilename(const std::string& new_spawnnpcfile) {
	spawnnpcfile = new_spawnnpcfile;
	unnamed = false;
}

void Map::setZoneFilename(const std::string& new_zonefile) {
	zonefile = new_zonefile;
	unnamed = false;
}

void Map::setWaypointFilename(const std::string& new_waypointfile) {
	waypointfile = new_waypointfile;
	unnamed = false;
}

bool Map::addSpawn(Tile* tile) {
	return MapSpawnManager::addSpawn(*this, tile);
}

bool Map::addNpcSpawn(Tile* tile) {
	return MapSpawnManager::addNpcSpawn(*this, tile);
}

void Map::removeSpawn(Tile* tile) {
	MapSpawnManager::removeSpawn(*this, tile);
}

void Map::removeNpcSpawn(Tile* tile) {
	MapSpawnManager::removeNpcSpawn(*this, tile);
}

SpawnList Map::getSpawnList(Tile* where) {
	return MapSpawnManager::getSpawnList(*this, where);
}

SpawnList Map::getNpcSpawnList(Tile* where) {
	return MapSpawnManager::getNpcSpawnList(*this, where);
}
