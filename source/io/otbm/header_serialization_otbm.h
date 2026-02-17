#ifndef RME_HEADER_SERIALIZATION_OTBM_H_
#define RME_HEADER_SERIALIZATION_OTBM_H_

struct MapVersion;
class Map;
class NodeFileReadHandle;
class BinaryNode;

class HeaderSerializationOTBM {
public:
	static bool getVersionInfo(NodeFileReadHandle* f, MapVersion& out_ver);
	static bool loadMapRoot(Map& map, NodeFileReadHandle& f, MapVersion& version, BinaryNode*& root, BinaryNode*& mapHeaderNode);
	static bool readMapAttributes(Map& map, BinaryNode* mapHeaderNode);
};

#endif
