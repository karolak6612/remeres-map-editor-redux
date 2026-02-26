//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CORE_RENDER_CHUNK_CACHE_H_
#define RME_RENDERING_CORE_RENDER_CHUNK_CACHE_H_

#include <unordered_map>
#include <memory>
#include "rendering/core/render_list.h"

class Floor;

struct CachedRenderChunk {
	std::unique_ptr<RenderList> render_list;
	uint32_t last_accessed_frame = 0;
};

class RenderChunkCache {
public:
	// Returns the cached render list for the given floor, or nullptr if not found.
	RenderList* Get(const Floor* floor, uint32_t current_frame);

	// Returns the cached render list for the given floor, creating it if necessary.
	RenderList* GetOrCreate(const Floor* floor, uint32_t current_frame);

	// Removes the render list for the given floor from the cache.
	void Invalidate(const Floor* floor);

	// Clears all cached render lists.
	void Clear();

	// Prunes entries that haven't been accessed for 'max_age_frames'.
	// This is critical for memory management, as Floors do not notify cache upon destruction.
	void Prune(uint32_t current_frame, uint32_t max_age_frames);

private:
	std::unordered_map<const Floor*, CachedRenderChunk> cache;
};

#endif
