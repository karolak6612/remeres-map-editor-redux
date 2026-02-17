#ifndef RME_TOWN_SERIALIZATION_OTBM_H_
#define RME_TOWN_SERIALIZATION_OTBM_H_

#include "io/otbm/otbm_types.h"

class Map;
class BinaryNode;
class NodeFileWriteHandle;

class TownSerializationOTBM {
public:
	static void readTowns(Map& map, BinaryNode* mapNode);
	static void writeTowns(const Map& map, NodeFileWriteHandle& f);
};

#endif
