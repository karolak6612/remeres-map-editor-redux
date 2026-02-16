#ifndef RME_WAYPOINT_SERIALIZATION_OTBM_H_
#define RME_WAYPOINT_SERIALIZATION_OTBM_H_

#include "io/iomap_otbm.h"

class Map;
class BinaryNode;
class NodeFileWriteHandle;

class WaypointSerializationOTBM {
public:
	static void readWaypoints(Map& map, BinaryNode* mapNode);
	static IOMapOTBM::WriteResult writeWaypoints(const Map& map, NodeFileWriteHandle& f, MapVersion mapVersion);
};

#endif
