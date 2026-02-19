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

#include <sstream>
#include <algorithm>
#include <ranges>
#include <unordered_set>
#include <unordered_map>
#include <spdlog/spdlog.h>

Map::Map() :
	BaseMap(),
	width(512),
	height(512),
	houses(*this),
	has_changed(false),
	unnamed(false),
	waypoints(*this) {
	spdlog::info("Map created [Map={}]", static_cast<void*>(this));
	// Earliest version possible
	// Caller is responsible for converting us to proper version
	mapVersion.otbm = MAP_OTBM_1;
	mapVersion.client = OTB_VERSION_NONE;
}

void Map::initializeEmpty() {
	spdlog::info("Map::initializeEmpty [Map={}]", static_cast<void*>(this));
	height = 2048;
	width = 2048;

	static int unnamed_counter = 0;

	std::string sname = "Untitled-" + std::to_string(++unnamed_counter);
	name = sname + ".otbm";
	spawnfile = sname + "-spawn.xml";
	housefile = sname + "-house.xml";
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
	if (mapVersion.client == to.client) {
		// Only OTBM version differs
		// No changes necessary
		mapVersion = to;
		return true;
	}

	/* TODO

	if(to.otbm == MAP_OTBM_4 && to.client < CLIENT_VERSION_850)
		return false;

	/* Legacy conversion logic removed */
#if 0
	if(mapVersion.client >= CLIENT_VERSION_760 && to.client < CLIENT_VERSION_760)
		convert(getReplacementMapFrom760To740(), showdialog);

	if(mapVersion.client < CLIENT_VERSION_810 && to.client >= CLIENT_VERSION_810)
		convert(getReplacementMapFrom800To810(), showdialog);

	if(mapVersion.client == CLIENT_VERSION_854_BAD && to.client >= CLIENT_VERSION_854)
		convert(getReplacementMapFrom854To854(), showdialog);
#endif
	mapVersion = to;

	return true;
}

bool Map::convert(const ConversionMap& rm, bool showdialog) {
	if (showdialog) {
		g_gui.CreateLoadBar("Converting map ...");
	}

	std::unordered_map<const std::vector<uint16_t>*, std::unordered_set<uint16_t>> mtm_lookups;
	for (const auto& entry : rm.mtm) {
		mtm_lookups.emplace(&entry.first, std::unordered_set<uint16_t>(entry.first.begin(), entry.first.end()));
	}

	uint64_t tiles_done = 0;
	std::vector<uint16_t> id_list;

	// std::ofstream conversions("converted_items.txt");

	for (auto& tile_loc : tiles()) {
		Tile* tile = tile_loc.get();
		ASSERT(tile);

		if (tile->empty()) {
			continue;
		}

		// id_list try MTM conversion
		id_list.clear();
		id_list.reserve(tile->items.size() + 1);

		if (tile->ground) {
			id_list.push_back(tile->ground->getID());
		}

		auto border_view = tile->items | std::views::filter([](const auto& i) { return i->isBorder(); });
		auto border_ids = border_view | std::views::transform([](const auto& i) { return i->getID(); });
		id_list.insert(id_list.end(), border_ids.begin(), border_ids.end());

		std::ranges::sort(id_list);

		ConversionMap::MTM::const_iterator cfmtm = rm.mtm.end();

		while (!id_list.empty()) {
			cfmtm = rm.mtm.find(id_list);
			if (cfmtm != rm.mtm.end()) {
				break;
			}
			id_list.pop_back();
		}

		// Keep track of how many items have been inserted at the bottom
		size_t inserted_items = 0;

		if (cfmtm != rm.mtm.end()) {
			const std::vector<uint16_t>& v = cfmtm->first;
			const auto& ids_to_remove = mtm_lookups.at(&v);

			if (tile->ground && ids_to_remove.contains(tile->ground->getID())) {
				tile->ground.reset();
			}

			auto part_iter = std::stable_partition(tile->items.begin(), tile->items.end(), [&ids_to_remove](const std::unique_ptr<Item>& item) {
				return !ids_to_remove.contains(item->getID());
			});

			tile->items.erase(part_iter, tile->items.end());

			const std::vector<uint16_t>& new_items = cfmtm->second;
			for (uint16_t new_id : new_items) {
				std::unique_ptr<Item> item = Item::Create(new_id);
				if (item->isGroundTile()) {
					tile->ground = std::move(item);
				} else {
					tile->items.insert(tile->items.begin(), std::move(item));
					++inserted_items;
				}
			}
		}

		if (tile->ground) {
			ConversionMap::STM::const_iterator cfstm = rm.stm.find(tile->ground->getID());
			if (cfstm != rm.stm.end()) {
				uint16_t aid = tile->ground->getActionID();
				uint16_t uid = tile->ground->getUniqueID();
				tile->ground.reset();

				const std::vector<uint16_t>& v = cfstm->second;
				// conversions << "Converted " << tile->getX() << ":" << tile->getY() << ":" << tile->getZ() << " " << id << " -> ";
				for (uint16_t new_id : v) {
					std::unique_ptr<Item> item = Item::Create(new_id);
					// conversions << *iit << " ";
					if (item->isGroundTile()) {
						item->setActionID(aid);
						item->setUniqueID(uid);
						tile->addItem(std::move(item));
					} else {
						tile->items.insert(tile->items.begin(), std::move(item));
						++inserted_items;
					}
				}
				// conversions << std::endl;
			}
		}

		for (auto replace_item_iter = tile->items.begin() + inserted_items; replace_item_iter != tile->items.end();) {
			uint16_t id = (*replace_item_iter)->getID();
			ConversionMap::STM::const_iterator cf = rm.stm.find(id);
			if (cf != rm.stm.end()) {
				// uint16_t aid = (*replace_item_iter)->getActionID();
				// uint16_t uid = (*replace_item_iter)->getUniqueID();

				replace_item_iter = tile->items.erase(replace_item_iter);
				const std::vector<uint16_t>& v = cf->second;
				for (uint16_t new_id : v) {
					replace_item_iter = tile->items.insert(replace_item_iter, Item::Create(new_id));
					// conversions << "Converted " << tile->getX() << ":" << tile->getY() << ":" << tile->getZ() << " " << id << " -> " << *iit << std::endl;
					++replace_item_iter;
				}
			} else {
				++replace_item_iter;
			}
		}

		++tiles_done;
		if (showdialog && tiles_done % 0x10000 == 0) {
			g_gui.SetLoadDone(static_cast<int>(tiles_done * 100.0 / static_cast<double>(getTileCount())));
		}
	}

	if (showdialog) {
		g_gui.DestroyLoadBar();
	}

	return true;
}

void Map::cleanInvalidTiles(bool showdialog) {
	if (showdialog) {
		g_gui.CreateLoadBar("Removing invalid tiles...");
	}

	uint64_t tiles_done = 0;

	for (auto& tile_loc : tiles()) {
		Tile* tile = tile_loc.get();
		ASSERT(tile);

		if (tile->empty()) {
			continue;
		}

		// Use std::erase_if from C++20 for cleanup
		std::erase_if(tile->items, [](const std::unique_ptr<Item>& item) {
			return !g_items.typeExists(item->getID());
		});

		++tiles_done;
		if (showdialog && tiles_done % 0x10000 == 0) {
			g_gui.SetLoadDone(static_cast<int>(tiles_done * 100.0 / static_cast<double>(getTileCount())));
		}
	}

	if (showdialog) {
		g_gui.DestroyLoadBar();
	}
}

void Map::convertHouseTiles(uint32_t fromId, uint32_t toId) {
	g_gui.CreateLoadBar("Converting house tiles...");
	uint64_t tiles_done = 0;

	for (auto& tile_loc : tiles()) {
		Tile* tile = tile_loc.get();
		ASSERT(tile);

		uint32_t houseId = tile->getHouseID();
		if (houseId == 0 || houseId != fromId) {
			continue;
		}

		tile->setHouseID(toId);
		++tiles_done;
		if (tiles_done % 0x10000 == 0) {
			g_gui.SetLoadDone(static_cast<int>(tiles_done * 100.0 / static_cast<double>(getTileCount())));
		}
	}

	g_gui.DestroyLoadBar();
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

void Map::setWaypointFilename(const std::string& new_waypointfile) {
	waypointfile = new_waypointfile;
	unnamed = false;
}

bool Map::addSpawn(Tile* tile) {
	Spawn* spawn = tile->spawn.get();
	if (spawn) {
		int z = tile->getZ();
		int start_x = tile->getX() - spawn->getSize();
		int start_y = tile->getY() - spawn->getSize();
		int end_x = tile->getX() + spawn->getSize();
		int end_y = tile->getY() + spawn->getSize();

		for (int y : std::views::iota(start_y, end_y + 1)) {
			for (int x : std::views::iota(start_x, end_x + 1)) {
				TileLocation* ctile_loc = createTileL(x, y, z);
				ctile_loc->increaseSpawnCount();
			}
		}
		spawns.addSpawn(tile);
		return true;
	}
	return false;
}

void Map::removeSpawnInternal(Tile* tile) {
	Spawn* spawn = tile->spawn.get();
	ASSERT(spawn);

	int z = tile->getZ();
	int start_x = tile->getX() - spawn->getSize();
	int start_y = tile->getY() - spawn->getSize();
	int end_x = tile->getX() + spawn->getSize();
	int end_y = tile->getY() + spawn->getSize();

	for (int y : std::views::iota(start_y, end_y + 1)) {
		for (int x : std::views::iota(start_x, end_x + 1)) {
			TileLocation* ctile_loc = getTileL(x, y, z);
			if (ctile_loc != nullptr && ctile_loc->getSpawnCount() > 0) {
				ctile_loc->decreaseSpawnCount();
			}
		}
	}
}

void Map::removeSpawn(Tile* tile) {
	if (tile->spawn) {
		removeSpawnInternal(tile);
		spawns.removeSpawn(tile);
	}
}

SpawnList Map::getSpawnList(Tile* where) {
	SpawnList list;
	TileLocation* tile_loc = where->getLocation();
	if (tile_loc) {
		if (tile_loc->getSpawnCount() > 0) {
			uint32_t found = 0;
			if (where->spawn) {
				++found;
				list.push_back(where->spawn.get());
			}

			// Scans the border tiles in an expanding square around the original spawn
			int z = where->getZ();
			int start_x = where->getX() - 1, end_x = where->getX() + 1;
			int start_y = where->getY() - 1, end_y = where->getY() + 1;

			while (found != tile_loc->getSpawnCount()) {
				// Top and Bottom edges
				for (int x : std::views::iota(start_x, end_x + 1)) {
					Tile* tile = getTile(x, start_y, z);
					if (tile && tile->spawn) {
						list.push_back(tile->spawn.get());
						++found;
					}
					tile = getTile(x, end_y, z);
					if (tile && tile->spawn) {
						list.push_back(tile->spawn.get());
						++found;
					}
				}

				// Left and Right edges (excluding corners already checked)
				for (int y : std::views::iota(start_y + 1, end_y)) {
					Tile* tile = getTile(start_x, y, z);
					if (tile && tile->spawn) {
						list.push_back(tile->spawn.get());
						++found;
					}
					tile = getTile(end_x, y, z);
					if (tile && tile->spawn) {
						list.push_back(tile->spawn.get());
						++found;
					}
				}
				--start_x, --start_y;
				++end_x, ++end_y;
			}
		}
	}
	return list;
}
