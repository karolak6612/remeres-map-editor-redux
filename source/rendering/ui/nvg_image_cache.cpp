//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/ui/nvg_image_cache.h"
#include "game/items.h"
#include "game/sprites.h"
#include "rendering/core/normal_image.h"
#include "rendering/core/sprite_atlas_cache.h"
#include "rendering/core/sprite_database.h"
#include "rendering/io/sprite_loader.h"
#include "ui/gui.h"
#include <nanovg.h>

NVGImageCache::~NVGImageCache()
{
    clear();
}

void NVGImageCache::clear()
{
    if (lastContext && !spriteCache.empty()) {
        for (auto& [id, handle] : spriteCache) {
            if (handle > 0) {
                nvgDeleteImage(lastContext, handle);
            }
        }
    }
    spriteCache.clear();
    lastContext = nullptr;
}

int NVGImageCache::getSpriteImage(NVGcontext* vg, uint16_t itemId)
{
    if (itemId == 0) {
        return 0;
    }

    // Detect context change and clear cache
    if (vg != lastContext) {
        clear();
        lastContext = vg;
    }

    // Resolve Item ID
    ItemType& it = g_items[itemId];
    if (it.clientID == 0 || it.clientID >= g_gui.sprites.getMetadataSpace().size()
        || it.clientID >= g_gui.sprites.getAtlasCacheSpace().size()) {
        return 0;
    }

    // We use the item ID as the cache key since it's unique and stable
    auto itCache = spriteCache.find(itemId);
    if (itCache != spriteCache.end()) {
        return itCache->second;
    }

    uint32_t clientID = it.clientID;
    SpriteAtlasCache& atlas_cache = g_gui.sprites.getAtlasCacheSpace()[clientID];

    if (!atlas_cache.spriteList.empty()) {
        // Use the first frame/part of the sprite
        uint32_t sprite_index = atlas_cache.spriteList[0];
        auto& space = g_gui.sprites.getNormalImageSpace();
        if (sprite_index < space.size()) {
            NormalImage* img = &space[sprite_index];
            std::unique_ptr<uint8_t[]> rgba;

            // For legacy sprites (no transparency), use getRGBData + Magenta Masking
            // This matches how WxWidgets/SpriteIconGenerator renders icons
            if (!g_gui.loader.hasTransparency()) {
                std::unique_ptr<uint8_t[]> rgb = img->getRGBData();
                if (rgb) {
                    rgba = std::make_unique<uint8_t[]>(32 * 32 * 4);
                    for (int i = 0; i < 32 * 32; ++i) {
                        uint8_t r = rgb[i * 3 + 0];
                        uint8_t g = rgb[i * 3 + 1];
                        uint8_t b = rgb[i * 3 + 2];

                        // Magic Pink (Magenta) is transparent for legacy sprites
                        if (r == 0xFF && g == 0x00 && b == 0xFF) {
                            rgba[i * 4 + 0] = 0;
                            rgba[i * 4 + 1] = 0;
                            rgba[i * 4 + 2] = 0;
                            rgba[i * 4 + 3] = 0;
                        } else {
                            rgba[i * 4 + 0] = r;
                            rgba[i * 4 + 1] = g;
                            rgba[i * 4 + 2] = b;
                            rgba[i * 4 + 3] = 255;
                        }
                    }
                }
            }

            // Fallback/Standard path for alpha sprites or if RGB failed
            if (!rgba) {
                rgba = img->getRGBAData();
            }

            if (rgba) {
                int image = nvgCreateImageRGBA(vg, 32, 32, 0, rgba.get());
                if (image != 0) {
                    spriteCache[itemId] = image;
                    return image;
                }
            }
        }
    }

    return 0;
}
