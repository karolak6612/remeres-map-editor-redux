#include "rendering/core/render_chunk_cache.h"
#include <vector>

RenderList* RenderChunkCache::Get(const Floor* floor, uint32_t current_frame) {
	auto it = cache.find(floor);
	if (it != cache.end()) {
		it->second.last_accessed_frame = current_frame;
		return it->second.render_list.get();
	}
	return nullptr;
}

RenderList* RenderChunkCache::GetOrCreate(const Floor* floor, uint32_t current_frame) {
	CachedRenderChunk& chunk = cache[floor];
	if (!chunk.render_list) {
		chunk.render_list = std::make_unique<RenderList>();
	}
	chunk.last_accessed_frame = current_frame;
	return chunk.render_list.get();
}

void RenderChunkCache::Invalidate(const Floor* floor) {
	cache.erase(floor);
}

void RenderChunkCache::Clear() {
	cache.clear();
}

void RenderChunkCache::Prune(uint32_t current_frame, uint32_t max_age_frames) {
	std::erase_if(cache, [&](const auto& item) {
		// Handle potential wrap-around, though unlikely with uint32_t frames
		if (current_frame < item.second.last_accessed_frame) {
			return true; // Something weird, clear it
		}
		return (current_frame - item.second.last_accessed_frame) > max_age_frames;
	});
}
