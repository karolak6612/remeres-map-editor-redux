//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CORE_SPRITE_ATLAS_CACHE_H_
#define RME_RENDERING_CORE_SPRITE_ATLAS_CACHE_H_

#include "rendering/core/sprite_metadata.h"
#include "rendering/core/template_image.h"
#include <cstdint>
#include <ctime>
#include <memory>
#include <vector>

struct Outfit;
struct AtlasRegion;

class SpriteAtlasCache {
public:
    SpriteAtlasCache() = default;
    ~SpriteAtlasCache() = default;

    // Non-copyable to avoid double free of unique_ptrs
    SpriteAtlasCache(const SpriteAtlasCache&) = delete;
    SpriteAtlasCache& operator=(const SpriteAtlasCache&) = delete;
    SpriteAtlasCache(SpriteAtlasCache&&) = default;
    SpriteAtlasCache& operator=(SpriteAtlasCache&&) = default;

    std::vector<uint32_t> spriteList; // Stores indices into SpriteDatabase::normal_images_
    std::vector<uint32_t> instanced_templates; // Stores indices into SpriteDatabase::template_images_

    void invalidateCache(const AtlasRegion* region);
    void clean(time_t time, int longevity, SpriteDatabase& sprites, TextureGC& gc);

    const AtlasRegion* getAtlasRegion(
        uint32_t clientID, const SpriteMetadata& metadata, SpriteDatabase& sprites, AtlasManager& atlas, TextureGC& gc, SpriteLoader& loader, bool use_memcached, int _x, int _y, int _layer, int _count, int _pattern_x, int _pattern_y,
        int _pattern_z, int _frame, bool block = true
    );
    const AtlasRegion* getAtlasRegion(
        uint32_t clientID, const SpriteMetadata& metadata, SpriteDatabase& sprites, AtlasManager& atlas, TextureGC& gc, SpriteLoader& loader, bool use_memcached, int _x, int _y, int _dir, int _addon, int _pattern_z, const Outfit& _outfit,
        int _frame, bool block = true
    );

    [[nodiscard]] const AtlasRegion* getCachedDefaultRegion() const
    {
        return cached_default_region;
    }

    [[nodiscard]] uint32_t getDebugImageId(size_t index) const;
    [[nodiscard]] uint32_t getSpriteId(const SpriteMetadata& metadata, int frameIndex, int pattern_x, int pattern_y) const;

    [[nodiscard]] size_t getSpriteListSize() const
    {
        return spriteList.size();
    }

    [[nodiscard]] bool isSimpleAndLoaded(const SpriteMetadata& metadata) const;

    TemplateImage* getTemplateImage(SpriteDatabase& sprites, uint32_t clientID, const SpriteMetadata& metadata, int sprite_index, const Outfit& outfit);

protected:
    mutable const AtlasRegion* cached_default_region = nullptr;
    uint32_t cached_generation_id = 0;
    uint32_t cached_sprite_id = 0;
};

#endif // RME_RENDERING_CORE_SPRITE_ATLAS_CACHE_H_
