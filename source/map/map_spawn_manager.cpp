#include "map/map_spawn_manager.h"
#include "map/map.h"
#include "map/map_region.h"

void MapSpawnManager::applySpawnCoverage(Map& map, Tile* tile, Spawn* spawn, bool create_missing_tiles, int delta, bool npc) {
	if (!tile || !spawn || delta == 0) {
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
				if (delta > 0) {
					tile_location->increaseNpcSpawnCount();
				} else if (tile_location->getNpcSpawnCount() > 0) {
					tile_location->decreaseNpcSpawnCount();
				}
			} else {
				if (delta > 0) {
					tile_location->increaseSpawnCount();
				} else if (tile_location->getSpawnCount() > 0) {
					tile_location->decreaseSpawnCount();
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
		applySpawnCoverage(map, tile, spawn, true, 1, false);
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
		applySpawnCoverage(map, tile, spawn, true, 1, true);
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
	applySpawnCoverage(map, tile, spawn, false, -1, false);
}

void MapSpawnManager::removeNpcSpawnInternal(Map& map, Tile* tile) {
	if (!tile || !tile->npc_spawn) {
		return;
	}

	Spawn* spawn = tile->npc_spawn.get();
	ASSERT(spawn);
	applySpawnCoverage(map, tile, spawn, false, -1, true);
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
	SpawnList list;
	if (!where) {
		return list;
	}

	TileLocation* tile_loc = where->getLocation();
	if (tile_loc) {
		if (tile_loc->getSpawnCount() > 0) {
			uint32_t found = 0;
			if (where->spawn) {
				++found;
				list.push_back(where->spawn.get());
			}

			// Scans the border tiles in an expanding square around the original spawn
			int z = where->getZ();
			int start_x = where->getX() - 1, end_x = where->getX() + 1;
			int start_y = where->getY() - 1, end_y = where->getY() + 1;

			auto checkTile = [&](int x, int y) {
				if (Tile* tile = map.getTile(x, y, z)) {
					if (tile->spawn) {
						int dx = std::abs(where->getX() - tile->getX());
						int dy = std::abs(where->getY() - tile->getY());
						if (dx <= tile->spawn->getSize() && dy <= tile->spawn->getSize()) {
							list.push_back(tile->spawn.get());
							++found;
						}
					}
				}
			};

			// Safety bound: limit search radius to prevent infinite expansion with stale spawn counts
			const int max_radius = std::max(map.getWidth(), map.getHeight());
			while (found < tile_loc->getSpawnCount()) {
				// Horizontal sides
				for (int x = start_x; x <= end_x; ++x) {
					checkTile(x, start_y);
					checkTile(x, end_y);
				}

				// Vertical sides (exclude corners already covered above)
				for (int y = start_y + 1; y < end_y; ++y) {
					checkTile(start_x, y);
					checkTile(end_x, y);
				}

				--start_x;
				--start_y;
				++end_x;
				++end_y;

				if ((end_x - start_x) / 2 > max_radius) {
					break;
				}
			}
		}
	}
	return list;
}

SpawnList MapSpawnManager::getNpcSpawnList(Map& map, Tile* where) {
	SpawnList list;
	if (!where) {
		return list;
	}

	TileLocation* tile_loc = where->getLocation();
	if (!tile_loc || tile_loc->getNpcSpawnCount() == 0) {
		return list;
	}

	uint32_t found = 0;
	if (where->npc_spawn) {
		++found;
		list.push_back(where->npc_spawn.get());
	}

	int z = where->getZ();
	int start_x = where->getX() - 1;
	int end_x = where->getX() + 1;
	int start_y = where->getY() - 1;
	int end_y = where->getY() + 1;

	auto checkTile = [&](int x, int y) {
		if (Tile* tile = map.getTile(x, y, z)) {
			if (tile->npc_spawn) {
				int dx = std::abs(where->getX() - tile->getX());
				int dy = std::abs(where->getY() - tile->getY());
				if (dx <= tile->npc_spawn->getSize() && dy <= tile->npc_spawn->getSize()) {
					list.push_back(tile->npc_spawn.get());
					++found;
				}
			}
		}
	};

	const int max_radius = std::max(map.getWidth(), map.getHeight());
	while (found < tile_loc->getNpcSpawnCount()) {
		for (int x = start_x; x <= end_x; ++x) {
			checkTile(x, start_y);
			checkTile(x, end_y);
		}

		for (int y = start_y + 1; y < end_y; ++y) {
			checkTile(start_x, y);
			checkTile(end_x, y);
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
