//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "brushes/zone/zone_brush.h"
#include "brushes/managers/brush_manager.h"
#include "map/map.h"
#include "map/tile.h"

ZoneBrush::ZoneBrush() :
	Brush() {
}

ZoneBrush::~ZoneBrush() {
}

int ZoneBrush::getLookID() const {
	return 0;
}

std::string ZoneBrush::getName() const {
	return "Zone Brush";
}

bool ZoneBrush::canDraw(BaseMap* map, const Position& position) const {
	(void)map;
	(void)position;
	return !g_brush_manager.GetSelectedZone().empty();
}

void ZoneBrush::draw(BaseMap* map, Tile* tile, void* parameter) {
	(void)parameter;
	ASSERT(tile);

	const std::string_view zone_name = g_brush_manager.GetSelectedZone();
	if (zone_name.empty()) {
		return;
	}

	auto* concrete_map = dynamic_cast<Map*>(map);
	if (!concrete_map) {
		return;
	}

	const uint16_t zone_id = concrete_map->zones.ensureZone(std::string { zone_name });
	tile->addZone(zone_id);
}

void ZoneBrush::undraw(BaseMap* map, Tile* tile) {
	ASSERT(tile);

	const auto* concrete_map = dynamic_cast<Map*>(map);
	if (!concrete_map) {
		return;
	}

	const std::string_view zone_name = g_brush_manager.GetSelectedZone();
	if (zone_name.empty()) {
		tile->clearZones();
		return;
	}

	if (const auto zone_id = concrete_map->zones.findId(std::string { zone_name })) {
		tile->removeZone(*zone_id);
	}
}
