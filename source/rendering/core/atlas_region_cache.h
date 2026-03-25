#ifndef RME_RENDERING_CORE_ATLAS_REGION_CACHE_H_
#define RME_RENDERING_CORE_ATLAS_REGION_CACHE_H_

#include <cstdint>

struct AtlasRegion;

// Per-sprite atlas cache. Stores the result of the last atlas region lookup
// to avoid repeated hash map queries for the most common case (simple 1x1 sprites).
struct AtlasRegionCache {
	mutable const AtlasRegion* cached_default_region = nullptr;
	uint32_t cached_generation_id = 0;
	uint32_t cached_sprite_id = 0;

	void invalidate() {
		cached_default_region = nullptr;
		cached_generation_id = 0;
		cached_sprite_id = 0;
	}
};

#endif
