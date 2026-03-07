#include "rendering/core/template_image.h"
#include "app/settings.h"
#include "rendering/core/normal_image.h"
#include "rendering/core/outfit_colorizer.h"
#include "rendering/core/outfit_colors.h"
#include "rendering/core/sprite_database.h"
#include "ui/gui.h"
#include <atomic>
#include <ranges>
#include <span>
#include <spdlog/spdlog.h>

static std::atomic<uint32_t> template_id_generator(0x1000000);

TemplateImage::TemplateImage(uint32_t clientID, int v, const Outfit& outfit) :
    atlas_region(nullptr),
    texture_id(template_id_generator.fetch_add(1)), // Generate unique ID for Atlas
    clientID(clientID),
    sprite_index(v),
    lookHead(outfit.lookHead),
    lookBody(outfit.lookBody),
    lookLegs(outfit.lookLegs),
    lookFeet(outfit.lookFeet)
{
}

TemplateImage::~TemplateImage()
{
}

TemplateImage::TemplateImage(TemplateImage&& other) noexcept
    : Image(std::move(other)),
      atlas_region(other.atlas_region),
      texture_id(other.texture_id),
      clientID(other.clientID),
      sprite_index(other.sprite_index),
      lookHead(other.lookHead),
      lookBody(other.lookBody),
      lookLegs(other.lookLegs),
      lookFeet(other.lookFeet)
{
    other.atlas_region = nullptr;
    other.texture_id = 0;
    other.clientID = 0;
    other.sprite_index = 0;
    other.lookHead = 0;
    other.lookBody = 0;
    other.lookLegs = 0;
    other.lookFeet = 0;
}

TemplateImage& TemplateImage::operator=(TemplateImage&& other) noexcept
{
    if (this != &other) {
        Image::operator=(std::move(other));
        atlas_region = other.atlas_region;
        texture_id = other.texture_id;
        clientID = other.clientID;
        sprite_index = other.sprite_index;
        lookHead = other.lookHead;
        lookBody = other.lookBody;
        lookLegs = other.lookLegs;
        lookFeet = other.lookFeet;

        other.atlas_region = nullptr;
        other.texture_id = 0;
        other.clientID = 0;
        other.sprite_index = 0;
        other.lookHead = 0;
        other.lookBody = 0;
        other.lookLegs = 0;
        other.lookFeet = 0;
    }
    return *this;
}

void TemplateImage::clean(time_t time, int longevity, SpriteDatabase& sprites, TextureGC& gc)
{
    // Evict from atlas if expired
    if (longevity == -1) {
        longevity = g_settings.getInteger(Config::TEXTURE_LONGEVITY);
    }
    if (isGLLoaded && time - static_cast<time_t>(lastaccess.load(std::memory_order_relaxed)) > longevity) {
        isGLLoaded = false;
        atlas_region = nullptr;
        ImageHandle old_handle = handle;
        generation_id++;
        handle.generation = generation_id;
        gc.removeResidentImage(old_handle);
    }
}

namespace {
    void ColorizeTemplatePixels(
        uint8_t* dest, const uint8_t* mask, size_t pixelCount, int lookHead, int lookBody, int lookLegs, int lookFeet, bool destHasAlpha
    )
    {
        const int dest_step = destHasAlpha ? 4 : 3;
        const int mask_step = 3;

        std::span<uint8_t> destSpan(dest, pixelCount * dest_step);
        std::span<const uint8_t> maskSpan(mask, pixelCount * mask_step);

        for (size_t i : std::views::iota(0u, pixelCount)) {
            uint8_t& red = destSpan[i * dest_step + 0];
            uint8_t& green = destSpan[i * dest_step + 1];
            uint8_t& blue = destSpan[i * dest_step + 2];

            const uint8_t& tred = maskSpan[i * mask_step + 0];
            const uint8_t& tgreen = maskSpan[i * mask_step + 1];
            const uint8_t& tblue = maskSpan[i * mask_step + 2];

            if (tred && tgreen && !tblue) { // yellow => head
                OutfitColorizer::ColorizePixel(lookHead, red, green, blue);
            } else if (tred && !tgreen && !tblue) { // red => body
                OutfitColorizer::ColorizePixel(lookBody, red, green, blue);
            } else if (!tred && tgreen && !tblue) { // green => legs
                OutfitColorizer::ColorizePixel(lookLegs, red, green, blue);
            } else if (!tred && !tgreen && tblue) { // blue => feet
                OutfitColorizer::ColorizePixel(lookFeet, red, green, blue);
            }
        }
    }

    bool validateTemplateParentAndIndices(const TemplateImage* img, int sprite_index, size_t& mask_index, SpriteDatabase* sprites)
    {
        if (img->clientID >= sprites->getMetadataSpace().size() || img->clientID >= sprites->getAtlasCacheSpace().size()) {
            spdlog::warn("TemplateImage (texture_id={}): Invalid clientID {}.", img->texture_id, img->clientID);
            return false;
        }
        const SpriteMetadata& meta = sprites->getMetadataSpace()[img->clientID];
        const SpriteAtlasCache& atlas = sprites->getAtlasCacheSpace()[img->clientID];

        if (meta.width <= 0 || meta.height <= 0) {
            spdlog::warn("TemplateImage (texture_id={}): Invalid metadata dimensions ({}x{})", img->texture_id, meta.width, meta.height);
            return false;
        }

        if (sprite_index < 0) {
            spdlog::warn("TemplateImage (texture_id={}): Sprite index is negative (sprite_index={})", img->texture_id, sprite_index);
            return false;
        }

        mask_index = static_cast<size_t>(sprite_index) + static_cast<size_t>(meta.height * meta.width);
        if (static_cast<size_t>(sprite_index) >= atlas.spriteList.size() || mask_index >= atlas.spriteList.size()) {
            spdlog::warn(
                "TemplateImage (texture_id={}): Access index out of bounds (base_index={}, mask_index={}, list_size={})", img->texture_id,
                sprite_index, mask_index, atlas.spriteList.size()
            );
            return false;
        }

        return true;
    }

    void clampTemplateLookValues(TemplateImage* img)
    {
        if (img->lookHead >= TemplateOutfitLookupTableSize) {
            img->lookHead = 0;
        }
        if (img->lookBody >= TemplateOutfitLookupTableSize) {
            img->lookBody = 0;
        }
        if (img->lookLegs >= TemplateOutfitLookupTableSize) {
            img->lookLegs = 0;
        }
        if (img->lookFeet >= TemplateOutfitLookupTableSize) {
            img->lookFeet = 0;
        }
    }
} // namespace

std::unique_ptr<uint8_t[]> TemplateImage::getRGBData(SpriteDatabase* sprites, SpriteLoader& loader, bool use_memcached)
{
    size_t mask_index = 0;
    if (!validateTemplateParentAndIndices(this, sprite_index, mask_index, sprites)) {
        return nullptr;
    }

    SpriteAtlasCache& atlas = sprites->getAtlasCacheSpace()[clientID];
    auto& space = sprites->getNormalImageSpace();
    if (atlas.spriteList[sprite_index] >= space.size() || atlas.spriteList[mask_index] >= space.size()) {
        return nullptr;
    }
    
    auto rgbdata = space[atlas.spriteList[sprite_index]].getRGBData(sprites, loader, use_memcached);
    auto template_rgbdata = space[atlas.spriteList[mask_index]].getRGBData(sprites, loader, use_memcached);

    if (!rgbdata) {
        return nullptr;
    }
    if (!template_rgbdata) {
        return nullptr;
    }

    clampTemplateLookValues(this);

    ColorizeTemplatePixels(
        rgbdata.get(), template_rgbdata.get(), SPRITE_PIXELS * SPRITE_PIXELS, lookHead, lookBody, lookLegs, lookFeet, false
    );

    return rgbdata;
}

std::unique_ptr<uint8_t[]> TemplateImage::getRGBAData(SpriteDatabase* sprites, SpriteLoader& loader, bool use_memcached)
{
    size_t mask_index = 0;
    if (!validateTemplateParentAndIndices(this, sprite_index, mask_index, sprites)) {
        return nullptr;
    }

    SpriteAtlasCache& atlas = sprites->getAtlasCacheSpace()[clientID];
    const SpriteMetadata& meta = sprites->getMetadataSpace()[clientID];
    auto& space = sprites->getNormalImageSpace();
    
    if (atlas.spriteList[sprite_index] >= space.size() || atlas.spriteList[mask_index] >= space.size()) {
         return nullptr;
    }

    auto rgbadata = space[atlas.spriteList[sprite_index]].getRGBAData(sprites, loader, use_memcached);
    auto template_rgbdata = space[atlas.spriteList[mask_index]].getRGBData(sprites, loader, use_memcached);

    if (!rgbadata) {
        spdlog::warn(
            "TemplateImage: Failed to load BASE sprite data for sprite_index={} (template_id={}). Parent width={}, height={}", sprite_index,
            texture_id, meta.width, meta.height
        );
        return nullptr;
    }
    if (!template_rgbdata) {
        spdlog::warn(
            "TemplateImage: Failed to load MASK sprite data for sprite_index={} (template_id={}) (mask_index={})", sprite_index, texture_id,
            mask_index
        );
        return nullptr;
    }

    clampTemplateLookValues(this);

    // Note: the base data is RGBA (4 channels) while the mask data is RGB (3 channels).
    ColorizeTemplatePixels(
        rgbadata.get(), template_rgbdata.get(), SPRITE_PIXELS * SPRITE_PIXELS, lookHead, lookBody, lookLegs, lookFeet, true
    );

    return rgbadata;
}

const AtlasRegion* TemplateImage::getAtlasRegion(SpriteDatabase& sprites, AtlasManager& atlas, TextureGC& gc, SpriteLoader& loader, bool use_memcached, bool block)
{
    if (isGLLoaded && atlas_region) {
        // Self-Healing: Check for stale atlas region pointer
        if (atlas_region->debug_sprite_id == AtlasRegion::INVALID_SENTINEL
            || (atlas_region->debug_sprite_id != 0 && atlas_region->debug_sprite_id != texture_id)) {
            spdlog::warn(
                "STALE ATLAS REGION DETECTED: TemplateImage {} held region owned by {}. Force reloading.", texture_id,
                atlas_region->debug_sprite_id
            );
            isGLLoaded = false;
            atlas_region = nullptr;
        } else {
            visit(gc);
            return atlas_region;
        }
    }

    if (!isGLLoaded && block) {
        const AtlasRegion* region = EnsureAtlasSprite(&sprites, atlas, gc, loader, use_memcached, texture_id);
        if (region) {
            isGLLoaded = true;
            atlas_region = region;
            gc.addResidentImage(handle);
        } else {
            return nullptr;
        }
    }
    visit(gc);
    return atlas_region;
}
