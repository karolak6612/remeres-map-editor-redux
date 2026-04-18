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

#include "brushes/spawn/spawn_brush.h"
#include "app/settings.h"
#include "brushes/creature/creature_brush.h"
#include "map/basemap.h"
#include "game/spawn.h"
#include "map/tile.h"
#include "ui/gui.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

namespace {
void populateMonsterSpawnArea(BaseMap* map, Tile* center_tile, int radius) {
	auto* palette = g_gui.GetPalette();
	if (!palette) {
		return;
	}

	auto* selected_brush = dynamic_cast<CreatureBrush*>(palette->GetSelectedCreatureBrush());
	if (!selected_brush || !selected_brush->getType() || selected_brush->getType()->isNpc) {
		return;
	}

	const int density = std::clamp(g_settings.getInteger(Config::SPAWN_MONSTER_DENSITY), 0, 100);
	if (density <= 0) {
		return;
	}

	const int side = radius * 2 + 1;
	const int target_count = static_cast<int>(std::ceil((side * side) * (density / 100.0)));
	if (target_count <= 0) {
		return;
	}

	std::vector<Position> positions;
	positions.reserve(static_cast<size_t>(side * side));
	for (int dx = -radius; dx <= radius; ++dx) {
		for (int dy = -radius; dy <= radius; ++dy) {
			positions.emplace_back(center_tile->getPosition().x + dx, center_tile->getPosition().y + dy, center_tile->getPosition().z);
		}
	}

	std::mt19937 rng(std::random_device {}());
	std::ranges::shuffle(positions, rng);

	int placed = 0;
	for (const auto& position : positions) {
		if (placed >= target_count) {
			break;
		}

		Tile* tile = map->getTile(position);
		if (!tile || !tile->ground || tile->creature) {
			continue;
		}

		selected_brush->draw_creature(map, tile);
		if (tile->creature) {
			++placed;
		}
	}
}
}

//=============================================================================
// Spawn brush

SpawnBrush::SpawnBrush() :
	Brush() {
	////
}

SpawnBrush::~SpawnBrush() {
	////
}

int SpawnBrush::getLookID() const {
	return 0;
}

std::string SpawnBrush::getName() const {
	return "Spawn Brush";
}

bool SpawnBrush::canDraw(BaseMap* map, const Position& position) const {
	Tile* tile = map->getTile(position);
	if (tile) {
		if (tile->spawn) {
			return false;
		}
	}
	return true;
}

void SpawnBrush::undraw(BaseMap* map, Tile* tile) {
	tile->spawn.reset();
}

void SpawnBrush::draw(BaseMap* map, Tile* tile, void* parameter) {
	ASSERT(tile);
	ASSERT(parameter); // Should contain an int which is the size of the newd spawn
	if (tile->spawn == nullptr) {
		const int radius = std::max(1, *(int*)parameter);
		tile->spawn = std::make_unique<Spawn>(radius);
		populateMonsterSpawnArea(map, tile, radius);
	}
}
