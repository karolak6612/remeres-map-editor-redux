//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/core/sprite_atlas_cache.h"
#include "game/outfit.h"

#include "rendering/core/normal_image.h"
#include "rendering/core/template_image.h"
#include <algorithm>

void SpriteAtlasCache::invalidateCache(const AtlasRegion* region)
{
    if (cached_default_region == region) {
        cached_default_region = nullptr;
        cached_generation_id = 0;
        cached_sprite_id = 0;
    }
}

void SpriteAtlasCache::clean(time_t time, int longevity)
{
    for (auto& iter : instanced_templates) {
        iter->clean(time, longevity);
    }
}

bool SpriteAtlasCache::isSimpleAndLoaded(const SpriteMetadata& metadata) const
{
    return metadata.is_simple && !spriteList.empty() && spriteList[0] != nullptr && spriteList[0]->isGLLoaded;
}

uint32_t SpriteAtlasCache::getDebugImageId(size_t index) const
{
    if (index < spriteList.size() && spriteList[index] != nullptr && spriteList[index]->isNormalImage()) {
        return static_cast<const NormalImage*>(spriteList[index])->id;
    }
    return 0;
}

uint32_t SpriteAtlasCache::getSpriteId(const SpriteMetadata& metadata, int frameIndex, int pattern_x, int pattern_y) const
{
    auto idx = metadata.getIndex(0, 0, 0, pattern_x, pattern_y, 0, frameIndex);
    if (idx < spriteList.size() && spriteList[idx] != nullptr && spriteList[idx]->isNormalImage()) {
        return static_cast<const NormalImage*>(spriteList[idx])->id;
    }
    return 0;
}

const AtlasRegion* SpriteAtlasCache::getAtlasRegion(
    uint32_t clientID, const SpriteMetadata& metadata, int _x, int _y, int _layer, int _count, int _pattern_x, int _pattern_y,
    int _pattern_z, int _frame
)
{
    if (metadata.numsprites == 0) {
        return nullptr;
    }

    if (_count == -1 && metadata.is_simple) {
        if (_x == 0 && _y == 0 && _layer == 0 && _frame == 0 && _pattern_x == 0 && _pattern_y == 0 && _pattern_z == 0
            && !spriteList.empty()) {
            if (cached_default_region && spriteList[0]->isGLLoaded && cached_generation_id == spriteList[0]->generation_id
                && cached_sprite_id == spriteList[0]->id) {
                return cached_default_region;
            }

            const AtlasRegion* valid_region = spriteList[0]->getAtlasRegion();
            if (valid_region && spriteList[0]->isGLLoaded) {
                cached_default_region = valid_region;
                cached_generation_id = spriteList[0]->generation_id;
                cached_sprite_id = spriteList[0]->id;
            } else {
                cached_default_region = nullptr;
                cached_generation_id = 0;
                cached_sprite_id = 0;
            }

            spriteList[0]->clientID = clientID;
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

    if (v < spriteList.size() && spriteList[v]) {
        spriteList[v]->clientID = clientID;
        return spriteList[v]->getAtlasRegion();
    }
    return nullptr;
}

TemplateImage* SpriteAtlasCache::getTemplateImage(uint32_t clientID, const SpriteMetadata& metadata, int sprite_index, const Outfit& outfit)
{
    auto it = std::ranges::find_if(instanced_templates, [sprite_index, &outfit](const auto& img) {
        if (img->sprite_index != sprite_index) {
            return false;
        }
        uint32_t lookHash = img->lookHead << 24 | img->lookBody << 16 | img->lookLegs << 8 | img->lookFeet;
        return outfit.getColorHash() == lookHash;
    });

    if (it != instanced_templates.end()) {
        if (it != instanced_templates.begin()) {
            std::iter_swap(it, instanced_templates.begin());
            return instanced_templates.front().get();
        }
        return it->get();
    }

    auto img = std::make_unique<TemplateImage>(clientID, sprite_index, outfit);
    TemplateImage* ptr = img.get();
    instanced_templates.push_back(std::move(img));
    return ptr;
}

const AtlasRegion* SpriteAtlasCache::getAtlasRegion(
    uint32_t clientID, const SpriteMetadata& metadata, int _x, int _y, int _dir, int _addon, int _pattern_z, const Outfit& _outfit,
    int _frame
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
        TemplateImage* img = getTemplateImage(clientID, metadata, v, _outfit);
        return img->getAtlasRegion();
    }
    if (v < spriteList.size() && spriteList[v]) {
        spriteList[v]->clientID = clientID;
        return spriteList[v]->getAtlasRegion();
    }
    return nullptr;
}
