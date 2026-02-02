//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "io/iomap_otbm.h"
#include "map/map.h"

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
