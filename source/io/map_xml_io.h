//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_MAP_XML_IO_H_
#define RME_MAP_XML_IO_H_

#include "util/common.h"
#include <string>
#include <vector>
#include "ext/pugixml.hpp"

class Map;

/**
 * @brief Helper class to handle XML auxiliary map files (Houses, Spawns, Waypoints)
 */
class MapXMLIO {
public:
	// Spawns
	static bool loadSpawns(Map& map, const FileName& dir);
	static bool loadSpawns(Map& map, pugi::xml_document& doc);
	static bool saveSpawns(const Map& map, const FileName& dir);
	static bool saveSpawns(const Map& map, pugi::xml_document& doc);

	// Houses
	static bool loadHouses(Map& map, const FileName& dir);
	static bool loadHouses(Map& map, pugi::xml_document& doc);
	static bool saveHouses(const Map& map, const FileName& dir);
	static bool saveHouses(const Map& map, pugi::xml_document& doc);

	// Waypoints
	static bool loadWaypoints(Map& map, const FileName& dir);
	static bool loadWaypoints(Map& map, pugi::xml_document& doc);
	static bool saveWaypoints(const Map& map, const FileName& dir);
	static bool saveWaypoints(const Map& map, pugi::xml_document& doc);

	static std::pair<std::string, std::string> NormalizeMapFilePaths(const FileName& dir, const std::string& filename);
	static bool loadWaypoints(Map& map, pugi::xml_node node);
};

#endif // RME_MAP_XML_IO_H_
