#ifndef RME_ARCHIVE_SERIALIZATION_OTBM_H_
#define RME_ARCHIVE_SERIALIZATION_OTBM_H_

#include "io/iomap_otbm.h"

class Map;

class ArchiveSerializationOTBM {
public:
	static bool loadMapFromOTGZ(IOMapOTBM& iomap, Map& map, const FileName& filename);
	static bool saveMapToOTGZ(IOMapOTBM& iomap, Map& map, const FileName& identifier);
};

#endif
