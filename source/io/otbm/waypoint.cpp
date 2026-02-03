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
bool IOMapOTBM::loadWaypoints(Map& map, const FileName& dir) {
	std::string fn = (const char*)(dir.GetPath(wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME).mb_str(wxConvUTF8));
	fn += map.waypointfile;
	FileName filename(wxstr(fn));
	if (!filename.FileExists()) {
		return false;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(fn.c_str());
	if (!result) {
		return false;
	}
	return loadWaypoints(map, doc);
};
bool IOMapOTBM::loadWaypoints(Map& map, pugi::xml_document& doc) {
	return true;
};

bool IOMapOTBM::saveWaypoints(Map& map, const FileName& dir) {
	wxString filepath = dir.GetPath(wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME);
	filepath += wxString(map.waypointfile.c_str(), wxConvUTF8);

	// Create the XML file
	pugi::xml_document doc;
	if (saveWaypoints(map, doc)) {
		return doc.save_file(filepath.wc_str(), "\t", pugi::format_default, pugi::encoding_utf8);
	}
	return false;
}

bool IOMapOTBM::saveWaypoints(Map& map, pugi::xml_document& doc) {
	pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
	if (!decl) {
		return false;
	}

	decl.append_attribute("version") = "1.0";

	pugi::xml_node houseNodes = doc.append_child("waypoints");
	for (const auto& houseEntry : map.houses) {
		const House* house = houseEntry.second;
		pugi::xml_node houseNode = houseNodes.append_child("waypoint");

		houseNode.append_attribute("name") = house->name.c_str();
		houseNode.append_attribute("id") = house->getID();
		houseNode.append_attribute("icon") = house->getID();

		const Position& exitPosition = house->getExit();
		houseNode.append_attribute("x") = exitPosition.x;
		houseNode.append_attribute("y") = exitPosition.y;
		houseNode.append_attribute("z") = exitPosition.z;

		houseNode.append_attribute("townid") = house->townid;
	}
	return true;
}
