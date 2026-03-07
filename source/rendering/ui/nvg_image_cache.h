//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_NVG_IMAGE_CACHE_H_
#define RME_NVG_IMAGE_CACHE_H_

#include <cstdint>
#include <unordered_map>

struct NVGcontext;

// Shared cache for NanoVG sprite images
class NVGImageCache {
public:
    NVGImageCache() = default;
    ~NVGImageCache();

    // Disable copy/move to safely manage NVG handles
    NVGImageCache(const NVGImageCache&) = delete;
    NVGImageCache& operator=(const NVGImageCache&) = delete;

    // Get an image handle for the given item ID.
    // Loads and caches the image if necessary.
    int getSpriteImage(NVGcontext* vg, uint16_t itemId);

    // Clear the cache. Deletes all valid NanoVG images created so far.
    void clear();

private:
    std::unordered_map<uint32_t, int> spriteCache; // itemId -> nvg image handle
    NVGcontext* lastContext = nullptr;
};

#endif
