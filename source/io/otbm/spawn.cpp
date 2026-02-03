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
#include "map/tile.h"
#include "game/item.h"
#include "game/complexitem.h"
#include "game/town.h"
#include "brushes/wall/wall_brush.h"

#include "io/otbm/map.h"
#include <spdlog/spdlog.h>

using attribute_t = uint8_t;
using flags_t = uint32_t;
bool IOMapOTBM::loadSpawns(Map& map, const FileName& dir) {
	std::string fn = (const char*)(dir.GetPath(wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME).mb_str(wxConvUTF8));
	fn += map.spawnfile;

	FileName filename(wxstr(fn));
	if (!filename.FileExists()) {
		warnings.push_back("IOMapOTBM::loadSpawns: File not found.");
		return false;
	}

	// has to be declared again as encoding-specific characters break loading there
	std::string encoded_path = (const char*)(dir.GetPath(wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME).mb_str(wxConvWhateverWorks));
	encoded_path += map.spawnfile;
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(encoded_path.c_str());
	if (!result) {
		warnings.push_back("IOMapOTBM::loadSpawns: File loading error.");
		return false;
	}
	return loadSpawns(map, doc);
}

bool IOMapOTBM::loadSpawns(Map& map, pugi::xml_document& doc) {
	pugi::xml_node node = doc.child("spawns");
	if (!node) {
		warnings.push_back("IOMapOTBM::loadSpawns: Invalid rootheader.");
		return false;
	}

	for (pugi::xml_node spawnNode = node.first_child(); spawnNode; spawnNode = spawnNode.next_sibling()) {
		if (as_lower_str(spawnNode.name()) != "spawn") {
			continue;
		}

		Position spawnPosition;
		spawnPosition.x = spawnNode.attribute("centerx").as_int();
		spawnPosition.y = spawnNode.attribute("centery").as_int();
		spawnPosition.z = spawnNode.attribute("centerz").as_int();

		if (spawnPosition.x == 0 || spawnPosition.y == 0) {
			warning("Bad position data on one spawn, discarding...");
			continue;
		}

		int32_t radius = spawnNode.attribute("radius").as_int();
		if (radius < 1) {
			warning("Couldn't read radius of spawn.. discarding spawn...");
			continue;
		}

		Tile* tile = map.getTile(spawnPosition);
		if (tile && tile->spawn) {
			warning("Duplicate spawn on position %d:%d:%d\n", tile->getX(), tile->getY(), tile->getZ());
			continue;
		}

		Spawn* spawn = newd Spawn(radius);
		if (!tile) {
			tile = map.allocator(map.createTileL(spawnPosition));
			map.setTile(spawnPosition, tile);
		}

		tile->spawn = spawn;
		map.addSpawn(tile);

		for (pugi::xml_node creatureNode = spawnNode.first_child(); creatureNode; creatureNode = creatureNode.next_sibling()) {
			const std::string& creatureNodeName = as_lower_str(creatureNode.name());
			if (creatureNodeName != "monster" && creatureNodeName != "npc") {
				continue;
			}

			bool isNpc = creatureNodeName == "npc";
			const std::string& name = creatureNode.attribute("name").as_string();
			if (name.empty()) {
				wxString err;
				err << "Bad creature position data, discarding creature at spawn " << spawnPosition.x << ":" << spawnPosition.y << ":" << spawnPosition.z << " due missing name.";
				warnings.Add(err);
				break;
			}

			int32_t spawntime = creatureNode.attribute("spawntime").as_int();
			if (spawntime == 0) {
				spawntime = g_settings.getInteger(Config::DEFAULT_SPAWNTIME);
			}

			Direction direction = SOUTH;
			int dir = creatureNode.attribute("direction").as_int(-1);
			if (dir >= DIRECTION_FIRST && dir <= DIRECTION_LAST) {
				direction = (Direction)dir;
			}

			Position creaturePosition(spawnPosition);

			pugi::xml_attribute xAttribute = creatureNode.attribute("x");
			pugi::xml_attribute yAttribute = creatureNode.attribute("y");
			if (!xAttribute || !yAttribute) {
				wxString err;
				err << "Bad creature position data, discarding creature \"" << name << "\" at spawn " << creaturePosition.x << ":" << creaturePosition.y << ":" << creaturePosition.z << " due to invalid position.";
				warnings.Add(err);
				break;
			}

			creaturePosition.x += xAttribute.as_int();
			creaturePosition.y += yAttribute.as_int();

			radius = std::max<int32_t>(radius, std::abs(creaturePosition.x - spawnPosition.x));
			radius = std::max<int32_t>(radius, std::abs(creaturePosition.y - spawnPosition.y));
			radius = std::min<int32_t>(radius, g_settings.getInteger(Config::MAX_SPAWN_RADIUS));

			Tile* creatureTile;
			if (creaturePosition == spawnPosition) {
				creatureTile = tile;
			} else {
				creatureTile = map.getTile(creaturePosition);
			}

			if (!creatureTile) {
				wxString err;
				err << "Discarding creature \"" << name << "\" at " << creaturePosition.x << ":" << creaturePosition.y << ":" << creaturePosition.z << " due to invalid position.";
				warnings.Add(err);
				break;
			}

			if (creatureTile->creature) {
				wxString err;
				err << "Duplicate creature \"" << name << "\" at " << creaturePosition.x << ":" << creaturePosition.y << ":" << creaturePosition.z << " was discarded.";
				warnings.Add(err);
				break;
			}

			CreatureType* type = g_creatures[name];
			if (!type) {
				type = g_creatures.addMissingCreatureType(name, isNpc);
			} else {
				if (type->outfit.lookType == 130) {
					spdlog::info("Found existing creature in map: '{}' with lookType 130", name);
				}
			}

			Creature* creature = newd Creature(type);
			creature->setDirection(direction);
			creature->setSpawnTime(spawntime);
			creatureTile->creature = creature;

			if (creatureTile->getLocation()->getSpawnCount() == 0) {
				// No spawn, create a newd one
				ASSERT(creatureTile->spawn == nullptr);
				Spawn* spawn = newd Spawn(5);
				creatureTile->spawn = spawn;
				map.addSpawn(creatureTile);
			}
		}
	}
	return true;
}

bool IOMapOTBM::saveSpawns(Map& map, const FileName& dir) {
	wxString filepath = dir.GetPath(wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME);
	filepath += wxString(map.spawnfile.c_str(), wxConvUTF8);

	// Create the XML file
	pugi::xml_document doc;
	if (saveSpawns(map, doc)) {
		return doc.save_file(filepath.wc_str(), "\t", pugi::format_default, pugi::encoding_utf8);
	}
	return false;
}

bool IOMapOTBM::saveSpawns(Map& map, pugi::xml_document& doc) {
	pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
	if (!decl) {
		return false;
	}

	decl.append_attribute("version") = "1.0";

	CreatureList creatureList;

	pugi::xml_node spawnNodes = doc.append_child("spawns");
	for (const auto& spawnPosition : map.spawns) {
		Tile* tile = map.getTile(spawnPosition);
		if (tile == nullptr) {
			continue;
		}

		Spawn* spawn = tile->spawn;
		ASSERT(spawn);

		pugi::xml_node spawnNode = spawnNodes.append_child("spawn");

		spawnNode.append_attribute("centerx") = spawnPosition.x;
		spawnNode.append_attribute("centery") = spawnPosition.y;
		spawnNode.append_attribute("centerz") = spawnPosition.z;

		int32_t radius = spawn->getSize();
		spawnNode.append_attribute("radius") = radius;

		for (int32_t y = -radius; y <= radius; ++y) {
			for (int32_t x = -radius; x <= radius; ++x) {
				Tile* creature_tile = map.getTile(spawnPosition + Position(x, y, 0));
				if (creature_tile) {
					Creature* creature = creature_tile->creature;
					if (creature && !creature->isSaved()) {
						pugi::xml_node creatureNode = spawnNode.append_child(creature->isNpc() ? "npc" : "monster");

						creatureNode.append_attribute("name") = creature->getName().c_str();
						creatureNode.append_attribute("x") = x;
						creatureNode.append_attribute("y") = y;
						creatureNode.append_attribute("z") = spawnPosition.z;
						creatureNode.append_attribute("spawntime") = creature->getSpawnTime();
						if (creature->getDirection() != NORTH) {
							creatureNode.append_attribute("direction") = creature->getDirection();
						}

						// Mark as saved
						creature->save();
						creatureList.push_back(creature);
					}
				}
			}
		}
	}

	for (Creature* creature : creatureList) {
		creature->reset();
	}
	return true;
}
