#ifndef RME_RENDERING_CORE_CHUNK_MANAGER_H_
#define RME_RENDERING_CORE_CHUNK_MANAGER_H_

#include "rendering/core/render_chunk.h"
#include <unordered_map>
#include <cstdint>

class MapNode;

class ChunkManager {
public:
	ChunkManager() = default;
	~ChunkManager() = default;

	RenderChunk* GetChunk(MapNode* node, int z);
	void Clear();

private:
	// Key: Unique ID derived from MapNode pointer and Z-layer
	std::unordered_map<uint64_t, RenderChunk> chunks;
};

#endif
