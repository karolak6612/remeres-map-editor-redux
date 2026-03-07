#include "rendering/core/normal_image.h"
#include "app/settings.h"
#include "rendering/core/sprite_database.h"
#include "rendering/core/sprite_decompression.h"
#include "rendering/core/texture_gc.h"
#include "rendering/io/sprite_loader.h"
#include <spdlog/spdlog.h>

constexpr int RGB_COMPONENTS = 3;

NormalImage::NormalImage() : id(0), atlas_region(nullptr), size(0), dump(nullptr) { }

NormalImage::~NormalImage()
{
}

NormalImage::NormalImage(NormalImage&& other) noexcept
    : Image(std::move(other)),
      id(other.id),
      atlas_region(other.atlas_region),
      size(other.size),
      dump(std::move(other.dump)),
      clientID(other.clientID)
{
    other.id = 0;
    other.atlas_region = nullptr;
    other.size = 0;
    other.clientID = 0;
}

NormalImage& NormalImage::operator=(NormalImage&& other) noexcept
{
    if (this != &other) {
        Image::operator=(std::move(other));
        id = other.id;
        atlas_region = other.atlas_region;
        size = other.size;
        dump = std::move(other.dump);
        clientID = other.clientID;
        
        other.id = 0;
        other.atlas_region = nullptr;
        other.size = 0;
        other.clientID = 0;
    }
    return *this;
}

void NormalImage::unloadGL(AtlasManager* atlas, TextureGC& gc)
{
    if (isGLLoaded) {
        if (atlas) {
            atlas->removeSprite(id);
        }

        isGLLoaded = false;
        atlas_region = nullptr;
        ImageHandle old_handle = handle;
        generation_id++;
        handle.generation = generation_id;

        gc.removeResidentImage(old_handle);
    }
}

void NormalImage::fulfillPreload(AtlasManager& atlas, TextureGC& gc, SpriteLoader& loader, bool use_memcached, std::unique_ptr<uint8_t[]> data)
{
    atlas_region = EnsureAtlasSprite(nullptr, atlas, gc, loader, use_memcached, id, std::move(data));
}

void NormalImage::clean(time_t time, int longevity, SpriteDatabase& sprites, TextureGC& gc)
{
    // Evict from atlas if expired
    if (longevity == -1) {
        longevity = g_settings.getInteger(Config::TEXTURE_LONGEVITY);
    }
    if (isGLLoaded && time - static_cast<time_t>(lastaccess.load(std::memory_order_relaxed)) > longevity) {
        if (clientID != 0 && clientID < sprites.getAtlasCacheSpace().size()) {
            sprites.getAtlasCacheSpace()[clientID].invalidateCache(atlas_region);
        }
    }
}

bool NormalImage::ensureDumpLoaded(SpriteLoader& loader, bool use_memcached)
{
    if (!dump) {
        if (use_memcached) {
            return false;
        }

        if (!loader.loadSpriteDump(dump, size, id)) {
            return false;
        }
    }
    return true;
}

std::unique_ptr<uint8_t[]> NormalImage::getRGBData(SpriteDatabase* sprites, SpriteLoader& loader, bool use_memcached)
{
    const size_t pixels_data_size = static_cast<size_t>(SPRITE_PIXELS) * SPRITE_PIXELS * RGB_COMPONENTS;
    if (id == 0) {
        return std::make_unique<uint8_t[]>(pixels_data_size); // Value-initialized (zeroed)
    }

    if (!ensureDumpLoaded(loader, use_memcached)) {
        return nullptr;
    }

    auto data = std::make_unique<uint8_t[]>(pixels_data_size);
    uint8_t bpp = loader.hasTransparency() ? 4 : RGB_COMPONENTS;
    size_t write = 0;
    size_t read = 0;

    // decompress pixels
    while (read < size && write < pixels_data_size) {
        if (read + 1 >= size) {
            spdlog::warn("NormalImage::getRGBData: Transparency header truncated (read={}, size={})", read, size);
            break;
        }
        int transparent = dump[read] | dump[read + 1] << 8;
        read += 2;
        for (int cnt = 0; cnt < transparent && write < pixels_data_size; ++cnt) {
            data[write + 0] = 0xFF; // red
            data[write + 1] = 0x00; // green
            data[write + 2] = 0xFF; // blue
            write += RGB_COMPONENTS;
        }

        if (read + 1 >= size) {
            spdlog::warn("NormalImage::getRGBData: Colored header truncated (read={}, size={})", read, size);
            break;
        }

        int colored = dump[read] | dump[read + 1] << 8;
        read += 2;

        if (read + static_cast<size_t>(colored) * bpp > size) {
            spdlog::warn("NormalImage::getRGBData: Read buffer overrun (colored={}, bpp={}, read={}, size={})", colored, bpp, read, size);
            break;
        }

        for (int cnt = 0; cnt < colored && write < pixels_data_size; ++cnt) {
            data[write + 0] = dump[read + 0]; // red
            data[write + 1] = dump[read + 1]; // green
            data[write + 2] = dump[read + 2]; // blue
            write += RGB_COMPONENTS;
            read += bpp;
        }
    }

    // fill remaining pixels
    while (write < pixels_data_size) {
        data[write + 0] = 0xFF; // red
        data[write + 1] = 0x00; // green
        data[write + 2] = 0xFF; // blue
        write += RGB_COMPONENTS;
    }
    return data;
}

std::unique_ptr<uint8_t[]> NormalImage::getRGBAData(SpriteDatabase* sprites, SpriteLoader& loader, bool use_memcached)
{
    // Robust ID 0 handling
    if (id == 0) {
        const size_t pixels_data_size = static_cast<size_t>(SPRITE_PIXELS_SIZE) * 4;
        return std::make_unique<uint8_t[]>(pixels_data_size); // Value-initialized (zeroed)
    }

    if (!ensureDumpLoaded(loader, use_memcached)) {
        // This is the only case where we return nullptr for non-zero ID
        // effectively warning the caller that the sprite is missing from file
        return nullptr;
    }

    return decompress_sprite(std::span {dump.get(), size}, loader.hasTransparency(), id);
}

const AtlasRegion* NormalImage::getAtlasRegion(SpriteDatabase& sprites, AtlasManager& atlas, TextureGC& gc, SpriteLoader& loader, bool use_memcached, bool block)
{
    if (isGLLoaded && atlas_region) {
        // Self-Healing: Check for stale atlas region pointer (e.g. from memory reuse)
        // Force reload if Owner is INVALID or DOES NOT MATCH
        if (atlas_region->debug_sprite_id == AtlasRegion::INVALID_SENTINEL
            || (atlas_region->debug_sprite_id != 0 && atlas_region->debug_sprite_id != id)) {
            spdlog::warn(
                "STALE ATLAS REGION DETECTED: NormalImage {} held region owned by {}. Force reloading.", id, atlas_region->debug_sprite_id
            );
            isGLLoaded = false;
            atlas_region = nullptr;
        } else {
            visit(gc);
            return atlas_region;
        }
    }

    if (!isGLLoaded && block) {
        atlas_region = EnsureAtlasSprite(&sprites, atlas, gc, loader, use_memcached, id);
    }
    visit(gc);
    return atlas_region;
}
