#include "rendering/utilities/tile_describer.h"
#include "app/main.h"
#include "app/definitions.h"
#include "editor/editor.h"
#include "map/tile.h"
#include "game/spawn.h"
#include "game/creature.h"
#include "game/item.h"
#include "ui/gui.h"
#include "util/common.h" // for wxstr

wxString TileDescriber::GetDescription(Tile* tile, bool showSpawns, bool showCreatures) {
	wxString ss;
	if (tile) {
		auto appendSeparator = [&ss]() {
			if (!ss.empty()) {
				ss << " | ";
			}
		};
		if (tile->spawn && showSpawns) {
			appendSeparator();
			ss << "Spawn radius: " << tile->spawn->getSize();
		}
		if (tile->npc_spawn && showSpawns) {
			appendSeparator();
			ss << "NPC spawn radius: " << tile->npc_spawn->getSize();
		}
		if (tile->creature && showCreatures) {
			appendSeparator();
			ss << (tile->creature->isNpc() ? "NPC" : "Monster");
			ss << " \"" << wxstr(tile->creature->getName()) << "\" spawntime: " << tile->creature->getSpawnTime();
		} else if (Item* item = tile->getTopItem()) {
			appendSeparator();
			ss << "Item \"" << wxstr(item->getName()) << "\"";
			ss << " id:" << item->getID();
			ss << " cid:" << item->getClientID();
			if (item->getUniqueID()) {
				ss << " uid:" << item->getUniqueID();
			}
			if (item->getActionID()) {
				ss << " aid:" << item->getActionID();
			}
			if (item->hasWeight()) {
				wxString s;
				s.Printf("%.2f", item->getWeight());
				ss << " weight: " << s;
			}
		}
		if (!tile->getZones().empty()) {
			appendSeparator();
			ss << "Zones: ";
			bool first = true;
			Editor* editor = g_gui.GetCurrentEditor();
			for (uint16_t zone_id : tile->getZones()) {
				if (!first) {
					ss << ", ";
				}
				first = false;
				if (editor) {
					const std::string zone_name = editor->map.zones.findName(zone_id);
					ss << wxstr(zone_name.empty() ? std::to_string(zone_id) : zone_name);
				} else {
					ss << zone_id;
				}
			}
		}
		if (ss.empty()) {
			ss << "Nothing";
		}
	} else {
		ss << "Nothing";
	}
	return ss;
}
