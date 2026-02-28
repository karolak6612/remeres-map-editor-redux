#ifndef RME_MAP_SPAWN_MANAGER_H_
#define RME_MAP_SPAWN_MANAGER_H_

#include "map/map.h"

class MapSpawnManager {
public:
	static bool addSpawn(Map& map, Tile* tile);
	static void removeSpawn(Map& map, Tile* tile);
	static SpawnList getSpawnList(Map& map, Tile* where);

private:
	static void removeSpawnInternal(Map& map, Tile* tile);
};

#endif
