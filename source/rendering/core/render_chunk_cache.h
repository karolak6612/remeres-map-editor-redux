#ifndef RME_RENDERING_CORE_RENDER_CHUNK_CACHE_H_
#define RME_RENDERING_CORE_RENDER_CHUNK_CACHE_H_

#include "rendering/core/sprite_instance.h"
#include <vector>
#include <unordered_map>
#include <memory>

class Floor;

struct RenderList {
	std::vector<SpriteInstance> sprites;
	bool is_valid = false;
};

class RenderChunkCache {
public:
	RenderChunkCache() = default;
	~RenderChunkCache() = default;

	// Non-copyable
	RenderChunkCache(const RenderChunkCache&) = delete;
	RenderChunkCache& operator=(const RenderChunkCache&) = delete;

	/**
	 * Get the RenderList for a specific floor.
	 * Returns nullptr if not cached.
	 */
	RenderList* getRenderList(const Floor* floor);

	/**
	 * Create or clear a RenderList for a specific floor.
	 * The returned list should be populated with sprites and marked valid.
	 */
	RenderList& getOrCreateRenderList(const Floor* floor);

	/**
	 * Mark a floor as dirty (invalidates cache).
	 */
	void invalidate(const Floor* floor);

	/**
	 * Prune old entries (e.g. unloaded floors).
	 * Should be called periodically.
	 * @param current_frame Frame counter or timestamp
	 * @param max_age Maximum age before eviction
	 */
	void prune(uint64_t current_frame, uint64_t max_age);

	void clear();

private:
	struct CacheEntry {
		RenderList list;
		uint64_t last_access_frame = 0;
	};

	std::unordered_map<const Floor*, std::unique_ptr<CacheEntry>> cache_;
};

#endif
