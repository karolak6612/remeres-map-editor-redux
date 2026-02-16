#ifndef RME_TILE_SERIALIZATION_OTBM_H_
#define RME_TILE_SERIALIZATION_OTBM_H_

#include "app/main.h"
#include "io/iomap_otbm.h"

class Map;
class BinaryNode;
class NodeFileWriteHandle;
class Tile;

class TileSerializationOTBM {
public:
	static void readTileArea(IOMapOTBM& iomap, Map& map, BinaryNode* mapNode);
	static void writeTileData(const IOMapOTBM& iomap, const Map& map, NodeFileWriteHandle& f);
	static void serializeTile_OTBM(const IOMapOTBM& iomap, Tile* tile, NodeFileWriteHandle& f);
};

#endif
