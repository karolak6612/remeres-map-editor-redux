#include "rendering/core/render_chunk_cache.h"

RenderList* RenderChunkCache::Get(const Floor* floor) {
	auto it = cache.find(floor);
	if (it != cache.end()) {
		return it->second.get();
	}
	return nullptr;
}

RenderList* RenderChunkCache::GetOrCreate(const Floor* floor) {
	auto& ptr = cache[floor];
	if (!ptr) {
		ptr = std::make_unique<RenderList>();
	}
	return ptr.get();
}

void RenderChunkCache::Invalidate(const Floor* floor) {
	cache.erase(floor);
}

void RenderChunkCache::Clear() {
	cache.clear();
}
