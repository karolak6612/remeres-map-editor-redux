#ifndef RME_TILE_SERIALIZATION_OTBM_H_
#define RME_TILE_SERIALIZATION_OTBM_H_

#include "io/iomap_otbm.h"

class Map;
class BinaryNode;
class NodeFileWriteHandle;
class Tile;

#include <functional>

class TileSerializationOTBM {
public:
	static void readTileArea(IOMapOTBM& iomap, Map& map, BinaryNode* mapNode);
	static void writeTileData(const IOMapOTBM& iomap, const Map& map, NodeFileWriteHandle& f, std::function<void(int)> progressCb = nullptr);
	static void serializeTile(const IOMapOTBM& iomap, Tile* tile, NodeFileWriteHandle& f);
};

#endif
