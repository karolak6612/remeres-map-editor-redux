#include "palette/hardcoded_palette_registry.h"

#include "app/main.h"
#include "palette/house/house_palette.h"
#include "palette/palette_waypoints.h"
#include "palette/spawn_palette.h"

namespace {

PalettePanel* createHousePanel(wxWindow* parent) {
	return newd HousePalette(parent);
}

PalettePanel* createSpawnPanel(wxWindow* parent) {
	return newd SpawnPalettePanel(parent);
}

PalettePanel* createWaypointPanel(wxWindow* parent) {
	return newd WaypointPalettePanel(parent);
}

void updateHouseMap(PalettePanel* panel, Map* map) {
	if (auto* housePanel = dynamic_cast<HousePalette*>(panel)) {
		housePanel->SetMap(map);
	}
}

void updateSpawnMap(PalettePanel*, Map*) {
}

void updateWaypointMap(PalettePanel* panel, Map* map) {
	if (auto* waypointPanel = dynamic_cast<WaypointPalettePanel*>(panel)) {
		waypointPanel->SetMap(map);
	}
}

} // namespace

const std::vector<HardcodedPaletteProvider>& GetHardcodedPaletteProviders() {
	static const std::vector<HardcodedPaletteProvider> providers {
		{ "House", createHousePanel, updateHouseMap },
		{ "Spawn", createSpawnPanel, updateSpawnMap },
		{ "Waypoint", createWaypointPanel, updateWaypointMap },
	};
	return providers;
}
