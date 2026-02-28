#include "map/map_spawn_manager.h"
#include "map/map.h"
#include "map/map_region.h"

bool MapSpawnManager::addSpawn(Map& map, Tile* tile) {
	if (!tile) {
		return false;
	}

	Spawn* spawn = tile->spawn.get();
	if (spawn) {
		int z = tile->getZ();
		int start_x = tile->getX() - spawn->getSize();
		int start_y = tile->getY() - spawn->getSize();
		int end_x = tile->getX() + spawn->getSize();
		int end_y = tile->getY() + spawn->getSize();

		for (int y = start_y; y <= end_y; ++y) {
			for (int x = start_x; x <= end_x; ++x) {
				TileLocation* ctile_loc = map.createTileL(x, y, z);
				ctile_loc->increaseSpawnCount();
			}
		}
		map.spawns.addSpawn(tile);
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

	int z = tile->getZ();
	int start_x = tile->getX() - spawn->getSize();
	int start_y = tile->getY() - spawn->getSize();
	int end_x = tile->getX() + spawn->getSize();
	int end_y = tile->getY() + spawn->getSize();

	for (int y = start_y; y <= end_y; ++y) {
		for (int x = start_x; x <= end_x; ++x) {
			TileLocation* ctile_loc = map.getTileL(x, y, z);
			if (ctile_loc != nullptr && ctile_loc->getSpawnCount() > 0) {
				ctile_loc->decreaseSpawnCount();
			}
		}
	}
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
