#include "map/map_spawn_manager.h"
#include "map/map.h"
#include "map/map_region.h"

#include <algorithm>
#include <cstdlib>

namespace {
	template <typename SpawnGetter, typename CountGetter>
	SpawnList getSpawnListImpl(Map& map, Tile* where, SpawnGetter get_spawn, CountGetter get_count) {
		SpawnList list;
		if (!where) {
			return list;
		}

		TileLocation* tile_loc = where->getLocation();
		if (!tile_loc) {
			return list;
		}

		const uint32_t target_count = get_count(*tile_loc);
		if (target_count == 0) {
			return list;
		}

		uint32_t found = 0;
		if (Spawn* spawn = get_spawn(*where)) {
			++found;
			list.push_back(spawn);
		}

		const int z = where->getZ();
		int start_x = where->getX() - 1;
		int end_x = where->getX() + 1;
		int start_y = where->getY() - 1;
		int end_y = where->getY() + 1;

		const auto check_tile = [&](int x, int y) {
			if (Tile* tile = map.getTile(x, y, z)) {
				if (Spawn* spawn = get_spawn(*tile)) {
					const int dx = std::abs(where->getX() - tile->getX());
					const int dy = std::abs(where->getY() - tile->getY());
					if (dx <= spawn->getSize() && dy <= spawn->getSize()) {
						list.push_back(spawn);
						++found;
					}
				}
			}
		};

		const int max_radius = std::max(map.getWidth(), map.getHeight());
		while (found < target_count) {
			for (int x = start_x; x <= end_x; ++x) {
				check_tile(x, start_y);
				check_tile(x, end_y);
			}

			for (int y = start_y + 1; y < end_y; ++y) {
				check_tile(start_x, y);
				check_tile(end_x, y);
			}

			--start_x;
			--start_y;
			++end_x;
			++end_y;

			if ((end_x - start_x) / 2 > max_radius) {
				break;
			}
		}

		return list;
	}
}

void MapSpawnManager::applySpawnCoverage(Map& map, Tile* tile, Spawn* spawn, bool create_missing_tiles, SpawnCoverageOperation operation, bool npc) {
	if (!tile || !spawn) {
		return;
	}

	const int z = tile->getZ();
	const int start_x = tile->getX() - spawn->getSize();
	const int start_y = tile->getY() - spawn->getSize();
	const int end_x = tile->getX() + spawn->getSize();
	const int end_y = tile->getY() + spawn->getSize();

	for (int y = start_y; y <= end_y; ++y) {
		for (int x = start_x; x <= end_x; ++x) {
			TileLocation* tile_location = create_missing_tiles ? map.createTileL(x, y, z) : map.getTileL(x, y, z);
			if (!tile_location) {
				continue;
			}

			if (npc) {
				switch (operation) {
					case SpawnCoverageOperation::Add:
						tile_location->increaseNpcSpawnCount();
						break;
					case SpawnCoverageOperation::Remove:
						if (tile_location->getNpcSpawnCount() > 0) {
							tile_location->decreaseNpcSpawnCount();
						}
						break;
				}
			} else {
				switch (operation) {
					case SpawnCoverageOperation::Add:
						tile_location->increaseSpawnCount();
						break;
					case SpawnCoverageOperation::Remove:
						if (tile_location->getSpawnCount() > 0) {
							tile_location->decreaseSpawnCount();
						}
						break;
				}
			}
		}
	}
}

bool MapSpawnManager::addSpawn(Map& map, Tile* tile) {
	if (!tile) {
		return false;
	}

	Spawn* spawn = tile->spawn.get();
	if (spawn) {
		applySpawnCoverage(map, tile, spawn, true, SpawnCoverageOperation::Add, false);
		map.spawns.addSpawn(tile);
		return true;
	}
	return false;
}

bool MapSpawnManager::addNpcSpawn(Map& map, Tile* tile) {
	if (!tile) {
		return false;
	}

	Spawn* spawn = tile->npc_spawn.get();
	if (spawn) {
		applySpawnCoverage(map, tile, spawn, true, SpawnCoverageOperation::Add, true);
		map.npc_spawns.addSpawn(tile);
		return true;
	}
	return false;
}

void MapSpawnManager::removeSpawnInternal(Map& map, Tile* tile) {
	if (!tile || !tile->spawn) {
		return;
	}

	Spawn* spawn = tile->spawn.get();
	ASSERT(spawn);
	applySpawnCoverage(map, tile, spawn, false, SpawnCoverageOperation::Remove, false);
}

void MapSpawnManager::removeNpcSpawnInternal(Map& map, Tile* tile) {
	if (!tile || !tile->npc_spawn) {
		return;
	}

	Spawn* spawn = tile->npc_spawn.get();
	ASSERT(spawn);
	applySpawnCoverage(map, tile, spawn, false, SpawnCoverageOperation::Remove, true);
}

void MapSpawnManager::removeSpawn(Map& map, Tile* tile) {
	if (!tile) {
		return;
	}

	if (tile->spawn) {
		removeSpawnInternal(map, tile);
		map.spawns.removeSpawn(tile);
	}
}

void MapSpawnManager::removeNpcSpawn(Map& map, Tile* tile) {
	if (!tile) {
		return;
	}

	if (tile->npc_spawn) {
		removeNpcSpawnInternal(map, tile);
		map.npc_spawns.removeSpawn(tile);
	}
}

SpawnList MapSpawnManager::getSpawnList(Map& map, Tile* where) {
	return getSpawnListImpl(map, where,
		[](Tile& tile) { return tile.spawn.get(); },
		[](TileLocation& tile_location) { return tile_location.getSpawnCount(); });
}

SpawnList MapSpawnManager::getNpcSpawnList(Map& map, Tile* where) {
	return getSpawnListImpl(map, where,
		[](Tile& tile) { return tile.npc_spawn.get(); },
		[](TileLocation& tile_location) { return tile_location.getNpcSpawnCount(); });
}
