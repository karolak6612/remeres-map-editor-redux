#ifndef RME_RENDERING_CORE_RENDER_CHUNK_CACHE_H_
#define RME_RENDERING_CORE_RENDER_CHUNK_CACHE_H_

#include <unordered_map>
#include <memory>
#include "rendering/core/render_list.h"

class Floor;

class RenderChunkCache {
public:
	// Returns the cached render list for the given floor, or nullptr if not found.
	RenderList* Get(const Floor* floor);

	// Returns the cached render list for the given floor, creating it if necessary.
	RenderList* GetOrCreate(const Floor* floor);

	// Removes the render list for the given floor from the cache.
	void Invalidate(const Floor* floor);

	// Clears all cached render lists.
	void Clear();

private:
	std::unordered_map<const Floor*, std::unique_ptr<RenderList>> cache;
};

#endif
