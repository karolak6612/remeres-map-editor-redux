//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/core/sprite_atlas_cache.h"
#include "game/outfit.h"

#include "rendering/core/normal_image.h"
#include "rendering/core/template_image.h"
#include "rendering/core/sprite_database.h"
#include "ui/gui.h"
#include <algorithm>
#include <ranges>

void SpriteAtlasCache::invalidateCache(const AtlasRegion* region)
{
    if (cached_default_region == region) {
        cached_default_region = nullptr;
        cached_generation_id = 0;
        cached_sprite_id = 0;
    }
}

void SpriteAtlasCache::clean(time_t time, int longevity, SpriteDatabase& sprites, TextureGC& gc)
{
    auto& template_space = sprites.getTemplateImageSpace();
    for (uint32_t idx : instanced_templates) {
        if (idx < template_space.size()) {
            template_space[idx].clean(time, longevity, sprites, gc);
        }
    }
}

bool SpriteAtlasCache::isSimpleAndLoaded(const SpriteMetadata& metadata) const
{
    if (!metadata.is_simple || spriteList.empty()) return false;
    auto& space = g_gui.sprites.getNormalImageSpace();
    if (spriteList[0] >= space.size()) return false;
    return space[spriteList[0]].isGLLoaded;
}

uint32_t SpriteAtlasCache::getDebugImageId(size_t index) const
{
    if (index < spriteList.size()) {
        return spriteList[index];
    }
    return 0;
}

uint32_t SpriteAtlasCache::getSpriteId(const SpriteMetadata& metadata, int frameIndex, int pattern_x, int pattern_y) const
{
    auto idx = metadata.getIndex(0, 0, 0, pattern_x, pattern_y, 0, frameIndex);
    if (idx < spriteList.size()) {
        return spriteList[idx];
    }
    return 0;
}

const AtlasRegion* SpriteAtlasCache::getAtlasRegion(
    uint32_t clientID, const SpriteMetadata& metadata, SpriteDatabase& sprites, AtlasManager& atlas, TextureGC& gc, SpriteLoader& loader, bool use_memcached, int _x, int _y, int _layer, int _count, int _pattern_x, int _pattern_y,
    int _pattern_z, int _frame, bool block
)
{
    if (metadata.numsprites == 0) {
        return nullptr;
    }

    auto& space = sprites.getNormalImageSpace();

    if (_count == -1 && metadata.is_simple) {
        if (_x == 0 && _y == 0 && _layer == 0 && _frame == 0 && _pattern_x == 0 && _pattern_y == 0 && _pattern_z == 0 && !spriteList.empty()
            && spriteList[0] < space.size()) {
                
            auto& img = space[spriteList[0]];
            if (cached_default_region && img.isGLLoaded && cached_generation_id == img.generation_id
                && cached_sprite_id == img.id) {
                return cached_default_region;
            }

            const AtlasRegion* valid_region = img.getAtlasRegion(sprites, atlas, gc, loader, use_memcached, block);
            if (valid_region && img.isGLLoaded) {
                cached_default_region = valid_region;
                cached_generation_id = img.generation_id;
                cached_sprite_id = img.id;
            } else {
                cached_default_region = nullptr;
                cached_generation_id = 0;
                cached_sprite_id = 0;
            }

            img.clientID = clientID;
            return valid_region;
        }
    }

    uint32_t v;
    if (_count >= 0 && metadata.height <= 1 && metadata.width <= 1) {
        v = _count;
    } else {
        v = static_cast<uint32_t>(metadata.getIndex(_x, _y, _layer, _pattern_x, _pattern_y, _pattern_z, _frame));
    }
    if (v >= metadata.numsprites) {
        if (metadata.numsprites == 1) {
            v = 0;
        } else {
            v %= metadata.numsprites;
        }
    }

    if (v < spriteList.size() && spriteList[v] < space.size()) {
        auto& img = space[spriteList[v]];
        img.clientID = clientID;
        return img.getAtlasRegion(sprites, atlas, gc, loader, use_memcached, block);
    }
    return nullptr;
}

TemplateImage* SpriteAtlasCache::getTemplateImage(SpriteDatabase& sprites, uint32_t clientID, const SpriteMetadata& metadata, int sprite_index, const Outfit& outfit)
{
    auto& template_space = sprites.getTemplateImageSpace();
    
    auto it = std::ranges::find_if(instanced_templates, [sprite_index, &outfit, &template_space](uint32_t idx) {
        if (idx >= template_space.size()) return false;
        auto& img = template_space[idx];
        if (img.sprite_index != sprite_index) {
            return false;
        }
        uint32_t lookHash = static_cast<uint32_t>(img.lookHead) << 24 | static_cast<uint32_t>(img.lookBody) << 16
            | static_cast<uint32_t>(img.lookLegs) << 8 | static_cast<uint32_t>(img.lookFeet);
        return outfit.getColorHash() == lookHash;
    });

    if (it != instanced_templates.end()) {
        if (it != instanced_templates.begin()) {
            std::iter_swap(it, instanced_templates.begin());
            return &template_space[instanced_templates.front()];
        }
        return &template_space[*it];
    }

    uint32_t new_idx = template_space.size();
    template_space.emplace_back(clientID, sprite_index, outfit);
    auto& new_img = template_space.back();
    new_img.handle = {ImageType::Template, new_idx, new_img.generation_id};
    
    instanced_templates.push_back(new_idx);
    return &template_space.back();
}

const AtlasRegion* SpriteAtlasCache::getAtlasRegion(
    uint32_t clientID, const SpriteMetadata& metadata, SpriteDatabase& sprites, AtlasManager& atlas, TextureGC& gc, SpriteLoader& loader, bool use_memcached, int _x, int _y, int _dir, int _addon, int _pattern_z, const Outfit& _outfit,
    int _frame, bool block
)
{
    if (metadata.numsprites == 0) {
        return nullptr;
    }

    uint32_t v = metadata.getIndex(_x, _y, 0, _dir, _addon, _pattern_z, _frame);
    if (v >= metadata.numsprites) {
        if (metadata.numsprites == 1) {
            v = 0;
        } else {
            v %= metadata.numsprites;
        }
    }
    if (metadata.layers > 1) {
        TemplateImage* img = getTemplateImage(sprites, clientID, metadata, v, _outfit);
        return img->getAtlasRegion(sprites, atlas, gc, loader, use_memcached, block);
    }
    if (v < spriteList.size()) {
        auto& space = sprites.getNormalImageSpace();
        if (spriteList[v] < space.size()) {
            auto& img = space[spriteList[v]];
            img.clientID = clientID;
            return img.getAtlasRegion(sprites, atlas, gc, loader, use_memcached, block);
        }
    }
    return nullptr;
}
