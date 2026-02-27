#include "rendering/core/render_chunk_cache.h"
#include <algorithm>

RenderList* RenderChunkCache::getRenderList(const Floor* floor) {
	auto it = cache_.find(floor);
	if (it != cache_.end()) {
		// Update access time? For now, we assume prune handles external timing or we pass frame here.
		// Let's just return it. The caller responsible for updating timestamp if we add that.
		// For simplicity, we won't track LRU strictly yet, or we assume caller updates some global frame.
		// Actually, let's update last_access_frame if we had access to a clock.
		return &it->second->list;
	}
	return nullptr;
}

RenderList& RenderChunkCache::getOrCreateRenderList(const Floor* floor) {
	auto& entry = cache_[floor];
	if (!entry) {
		entry = std::make_unique<CacheEntry>();
	}
	return entry->list;
}

void RenderChunkCache::invalidate(const Floor* floor) {
	auto it = cache_.find(floor);
	if (it != cache_.end()) {
		it->second->list.is_valid = false;
		it->second->list.sprites.clear();
	}
}

void RenderChunkCache::prune(uint64_t current_frame, uint64_t max_age) {
	// std::erase_if (C++20)
	std::erase_if(cache_, [&](const auto& item) {
		return (current_frame - item.second->last_access_frame) > max_age;
	});
}

void RenderChunkCache::clear() {
	cache_.clear();
}
