//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/utilities/sprite_icon_generator.h"
#include "app/main.h"
#include "app/settings.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/normal_image.h"
#include "rendering/core/template_image.h"
#include "ui/gui.h"
#include <algorithm>
#include <ranges>
#include <span>

wxBitmap SpriteIconGenerator::Generate(uint32_t clientID, SpriteSize size, bool rescale)
{
    if (clientID == 0 || clientID >= g_gui.sprites.getMetadataSpace().size()) {
        return wxBitmap();
    }
    const SpriteMetadata& metadata = g_gui.sprites.getMetadata(clientID);
    SpriteAtlasCache& atlas_cache = g_gui.sprites.getAtlasCache(clientID);

    ASSERT(metadata.width >= 1 && metadata.height >= 1);

    const int bgshade = g_settings.getInteger(Config::ICON_BACKGROUND);

    int image_size = std::max<uint8_t>(metadata.width, metadata.height) * SPRITE_PIXELS;
    wxImage image(image_size, image_size);
    image.Create(image_size, image_size);
    image.InitAlpha();

    unsigned char r = (bgshade >> 16) & 0xFF;
    unsigned char g = (bgshade >> 8) & 0xFF;
    unsigned char b = bgshade & 0xFF;
    unsigned char* rawData = image.GetData();
    unsigned char* rawAlpha = image.GetAlpha();
    int count = image_size * image_size;

    std::span<unsigned char> bgData(rawData, static_cast<size_t>(count) * 3);
    std::span<unsigned char> alphaData(rawAlpha, count);

    for (int i : std::views::iota(0, count)) {
        bgData[i * 3 + 0] = r;
        bgData[i * 3 + 1] = g;
        bgData[i * 3 + 2] = b;
    }
    std::ranges::fill(alphaData, 255);

    for (uint8_t l = 0; l < metadata.layers; l++) {
        for (uint8_t w = 0; w < metadata.width; w++) {
            for (uint8_t h = 0; h < metadata.height; h++) {
                const int i = metadata.getIndex(w, h, l, 0, 0, 0, 0);
                if (i < 0 || static_cast<size_t>(i) >= atlas_cache.spriteList.size() || !atlas_cache.spriteList[i]) {
                    continue;
                }
                std::unique_ptr<uint8_t[]> data = atlas_cache.spriteList[i]->getRGBData();
                if (data) {
                    wxImage img(SPRITE_PIXELS, SPRITE_PIXELS, data.get(), true);
                    img.SetMaskColour(0xFF, 0x00, 0xFF);
                    image.Paste(img, (metadata.width - w - 1) * SPRITE_PIXELS, (metadata.height - h - 1) * SPRITE_PIXELS);
                }
            }
        }
    }

    // Now comes the resizing / antialiasing
    if (rescale
        && (size == SPRITE_SIZE_16x16 || size == SPRITE_SIZE_64x64 || image.GetWidth() > SPRITE_PIXELS
            || image.GetHeight() > SPRITE_PIXELS)) {
        int new_size = 32;
        if (size == SPRITE_SIZE_16x16) {
            new_size = 16;
        } else if (size == SPRITE_SIZE_64x64) {
            new_size = 64;
        }
        image.Rescale(new_size, new_size, wxIMAGE_QUALITY_HIGH);
    }

    return wxBitmap(image);
}

wxBitmap SpriteIconGenerator::Generate(uint32_t clientID, SpriteSize size, const Outfit& outfit, bool rescale, Direction direction)
{
    if (clientID == 0 || clientID >= g_gui.sprites.getMetadataSpace().size()) {
        return wxBitmap();
    }
    const SpriteMetadata& metadata = g_gui.sprites.getMetadata(clientID);
    SpriteAtlasCache& atlas_cache = g_gui.sprites.getAtlasCache(clientID);

    ASSERT(metadata.width >= 1 && metadata.height >= 1);

    const int bgshade = g_settings.getInteger(Config::ICON_BACKGROUND);

    int image_size = std::max<uint8_t>(metadata.width, metadata.height) * SPRITE_PIXELS;
    wxImage image(image_size, image_size);
    image.Create(image_size, image_size);
    image.InitAlpha();

    unsigned char r = (bgshade >> 16) & 0xFF;
    unsigned char g = (bgshade >> 8) & 0xFF;
    unsigned char b = bgshade & 0xFF;
    unsigned char* rawData = image.GetData();
    unsigned char* rawAlpha = image.GetAlpha();
    int count = image_size * image_size;

    std::span<unsigned char> bgData(rawData, static_cast<size_t>(count) * 3);
    std::span<unsigned char> alphaData(rawAlpha, count);

    for (int i : std::views::iota(0, count)) {
        bgData[i * 3 + 0] = r;
        bgData[i * 3 + 1] = g;
        bgData[i * 3 + 2] = b;
    }
    std::ranges::fill(alphaData, 255);

    int frame_index = 0;
    if (metadata.pattern_x == 4) {
        frame_index = direction;
    }

    // Mounts
    int pattern_z = 0;
    if (outfit.lookMount != 0 && outfit.lookMount < g_gui.sprites.getMetadataSpace().size()) {
        const SpriteMetadata& mountMeta = g_gui.sprites.getMetadata(outfit.lookMount);
        SpriteAtlasCache& mountAtlas = g_gui.sprites.getAtlasCache(outfit.lookMount);

        // Mount outfit
        Outfit mountOutfit;
        mountOutfit.lookType = outfit.lookMount;
        mountOutfit.lookHead = outfit.lookMountHead;
        mountOutfit.lookBody = outfit.lookMountBody;
        mountOutfit.lookLegs = outfit.lookMountLegs;
        mountOutfit.lookFeet = outfit.lookMountFeet;

        // We need to render the mount
        // Simplified rendering: just render base frame 0 for mount (or south)
        int mount_frame_index = 0;
        if (mountMeta.pattern_x == 4) {
            mount_frame_index = direction;
        }

        for (uint8_t l = 0; l < mountMeta.layers; l++) {
            for (uint8_t w = 0; w < mountMeta.width; w++) {
                for (uint8_t h = 0; h < mountMeta.height; h++) {
                    std::unique_ptr<uint8_t[]> data = nullptr;

                    int mountIdx = mountMeta.getIndex(w, h, 0, mount_frame_index, 0, 0, 0);

                    // Handle mount sprite layers/templates similar to main sprite
                    // (Usually mounts are standard creatures)
                    if (mountMeta.layers == 2) {
                        if (l == 1) {
                            continue;
                        }

                        if (mountIdx >= 0 && static_cast<size_t>(mountIdx) < mountAtlas.spriteList.size()
                            && mountAtlas.spriteList[mountIdx]) {
                            auto tmplImg = mountAtlas.getTemplateImage(mountOutfit.lookMount, mountMeta, mountIdx, mountOutfit);
                            if (tmplImg) {
                                data = tmplImg->getRGBData();
                            }
                        }
                    } else {
                        // Standard mount
                        int idx = mountMeta.getIndex(w, h, l, mount_frame_index, 0, 0, 0);
                        if (idx >= 0 && static_cast<size_t>(idx) < mountAtlas.spriteList.size() && mountAtlas.spriteList[idx]) {
                            data = mountAtlas.spriteList[idx]->getRGBData();
                        }
                    }

                    if (data) {
                        wxImage img(SPRITE_PIXELS, SPRITE_PIXELS, data.get(), true);
                        img.SetMaskColour(0xFF, 0x00, 0xFF);
                        // Mount offset
                        int mount_x = (metadata.width - w - 1) * SPRITE_PIXELS - mountMeta.drawoffset_x;
                        int mount_y = (metadata.height - h - 1) * SPRITE_PIXELS - mountMeta.drawoffset_y;
                        image.Paste(img, mount_x, mount_y);
                    }
                }
            }
        }
        pattern_z = std::min<int>(1, metadata.pattern_z - 1);
    }

    for (int pattern_y = 0; pattern_y < metadata.pattern_y; pattern_y++) {
        if (pattern_y > 0) {
            if ((pattern_y - 1 >= 31) || !(outfit.lookAddon & (1 << (pattern_y - 1)))) {
                continue;
            }
        }

        for (uint8_t l = 0; l < metadata.layers; l++) {
            for (uint8_t w = 0; w < metadata.width; w++) {
                for (uint8_t h = 0; h < metadata.height; h++) {
                    std::unique_ptr<uint8_t[]> data = nullptr;

                    int idx0 = metadata.getIndex(w, h, 0, frame_index, pattern_y, pattern_z, 0);
                    int idx2 = metadata.getIndex(w, h, 2, frame_index, pattern_y, pattern_z, 0);
                    int idxl = metadata.getIndex(w, h, l, frame_index, pattern_y, pattern_z, 0);

                    if (metadata.layers == 2) {
                        if (l == 1) {
                            continue;
                        }
                        if (idx0 >= 0 && static_cast<size_t>(idx0) < atlas_cache.spriteList.size() && atlas_cache.spriteList[idx0]) {
                            auto tmplImg = atlas_cache.getTemplateImage(clientID, metadata, idx0, outfit);
                            if (tmplImg) {
                                data = tmplImg->getRGBData();
                            }
                        }
                    } else if (metadata.layers == 4) {
                        if (l == 1 || l == 3) {
                            continue;
                        }
                        if (l == 0) {
                            if (idx0 >= 0 && static_cast<size_t>(idx0) < atlas_cache.spriteList.size() && atlas_cache.spriteList[idx0]) {
                                auto tmplImg = atlas_cache.getTemplateImage(clientID, metadata, idx0, outfit);
                                if (tmplImg) {
                                    data = tmplImg->getRGBData();
                                }
                            }
                        }
                        if (l == 2) {
                            if (idx2 >= 0 && static_cast<size_t>(idx2) < atlas_cache.spriteList.size() && atlas_cache.spriteList[idx2]) {
                                auto tmplImg = atlas_cache.getTemplateImage(clientID, metadata, idx2, outfit);
                                if (tmplImg) {
                                    data = tmplImg->getRGBData();
                                }
                            }
                        }
                    } else {
                        if (idxl >= 0 && static_cast<size_t>(idxl) < atlas_cache.spriteList.size() && atlas_cache.spriteList[idxl]) {
                            data = atlas_cache.spriteList[idxl]->getRGBData();
                        }
                    }

                    if (data) {
                        wxImage img(SPRITE_PIXELS, SPRITE_PIXELS, data.get(), true);
                        img.SetMaskColour(0xFF, 0x00, 0xFF);
                        image.Paste(img, (metadata.width - w - 1) * SPRITE_PIXELS, (metadata.height - h - 1) * SPRITE_PIXELS);
                    }
                }
            }
        }
    }

    // Now comes the resizing / antialiasing
    if (rescale
        && (size == SPRITE_SIZE_16x16 || size == SPRITE_SIZE_64x64 || image.GetWidth() > SPRITE_PIXELS
            || image.GetHeight() > SPRITE_PIXELS)) {
        int new_size = 32;
        if (size == SPRITE_SIZE_16x16) {
            new_size = 16;
        } else if (size == SPRITE_SIZE_64x64) {
            new_size = 64;
        }
        image.Rescale(new_size, new_size, wxIMAGE_QUALITY_HIGH);
    }

    return wxBitmap(image);
}
