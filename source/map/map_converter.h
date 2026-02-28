#ifndef RME_MAP_CONVERTER_H_
#define RME_MAP_CONVERTER_H_

#include <cstdint>
#include "map/map.h"

class MapConverter {
public:
	static bool convert(Map& map, MapVersion to, bool showdialog = false);
	static bool convert(Map& map, const ConversionMap& cm, bool showdialog = false);
	static void cleanInvalidTiles(Map& map, bool showdialog = false);
	static void convertHouseTiles(Map& map, uint32_t fromId, uint32_t toId);
};

#endif
