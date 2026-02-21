#include "rendering/core/chunk_manager.h"
#include "map/map_region.h"

RenderChunk* ChunkManager::GetChunk(MapNode* node, int z) {
	if (!node) {
		return nullptr;
	}

	// Simple key packing using pointer address and Z-layer
	// Since MapNode size > 16, adding z (0-15) is collision-free
	uint64_t key = reinterpret_cast<uint64_t>(node) + z;
	return &chunks[key];
}

void ChunkManager::Clear() {
	chunks.clear();
}
