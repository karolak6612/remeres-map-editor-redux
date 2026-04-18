//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "brushes/spawn/npc_spawn_brush.h"
#include "game/spawn.h"
#include "map/basemap.h"

NpcSpawnBrush::NpcSpawnBrush() :
	Brush() {
}

NpcSpawnBrush::~NpcSpawnBrush() {
}

int NpcSpawnBrush::getLookID() const {
	return 0;
}

std::string NpcSpawnBrush::getName() const {
	return "NPC Spawn Brush";
}

bool NpcSpawnBrush::canDraw(BaseMap* map, const Position& position) const {
	if (const Tile* tile = map->getTile(position); tile && tile->npc_spawn) {
		return false;
	}
	return true;
}

void NpcSpawnBrush::undraw(BaseMap* map, Tile* tile) {
	(void)map;
	tile->npc_spawn.reset();
}

void NpcSpawnBrush::draw(BaseMap* map, Tile* tile, void* parameter) {
	(void)map;
	ASSERT(tile);
	ASSERT(parameter);
	if (tile->npc_spawn == nullptr) {
		tile->npc_spawn = std::make_unique<Spawn>(std::max(1, *static_cast<int*>(parameter)));
	}
}
