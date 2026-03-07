#include "rendering/core/texture_gc.h"
#include "app/main.h"
#include "app/settings.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/image.h"
#include "rendering/core/sprite_database.h"
#include "ui/gui.h"
#include <algorithm>

TextureGC::TextureGC() : animation_timer(std::make_unique<RenderTimer>())
{
    animation_timer->Start();
}

TextureGC::~TextureGC() = default;

void TextureGC::clear()
{
    std::lock_guard<std::recursive_mutex> lock(resident_images_mutex_);
    resident_images.clear();
    resident_game_sprites.clear();
    cleanup_list.clear();
    loaded_textures = 0;
    lastclean = std::time(nullptr);
    cached_time_ = 0;
}

void TextureGC::updateTime()
{
    cached_time_ = std::time(nullptr);
}

void TextureGC::addResidentImage(ImageHandle handle)
{
    if (!handle.isValid()) return;
    std::lock_guard<std::recursive_mutex> lock(resident_images_mutex_);
    resident_images.push_back(handle);
    loaded_textures++;
}

void TextureGC::removeResidentImage(ImageHandle handle)
{
    if (!handle.isValid()) return;
    std::lock_guard<std::recursive_mutex> lock(resident_images_mutex_);
    auto it = std::find(resident_images.begin(), resident_images.end(), handle);
    if (it != resident_images.end()) {
        // O(1) removal via swap-and-pop
        if (it != resident_images.end() - 1) {
            *it = resident_images.back();
        }
        resident_images.pop_back();
        loaded_textures.fetch_sub(1, std::memory_order_relaxed);
    }
}

bool TextureGC::containsResidentImage(ImageHandle handle) const
{
    std::lock_guard<std::recursive_mutex> lock(resident_images_mutex_);
    return std::find(resident_images.begin(), resident_images.end(), handle) != resident_images.end();
}

void TextureGC::addResidentGameSprite(uint32_t clientID)
{
    std::lock_guard<std::recursive_mutex> lock(resident_images_mutex_);
    resident_game_sprites.insert(clientID);
}

void TextureGC::removeResidentGameSprite(uint32_t clientID)
{
    std::lock_guard<std::recursive_mutex> lock(resident_images_mutex_);
    resident_game_sprites.erase(clientID);
}

bool TextureGC::containsResidentGameSprite(uint32_t clientID) const
{
    std::lock_guard<std::recursive_mutex> lock(resident_images_mutex_);
    return resident_game_sprites.contains(clientID);
}

constexpr int MIN_CLEAN_THRESHOLD = 100;

void TextureGC::addSpriteToCleanup(uint32_t spr_id, SpriteDatabase& sprites)
{
    std::lock_guard<std::recursive_mutex> lock(resident_images_mutex_);
    cleanup_list.push_back(spr_id);
    const size_t clean_threshold
        = static_cast<size_t>(std::max(MIN_CLEAN_THRESHOLD, g_settings.getInteger(Config::SOFTWARE_CLEAN_THRESHOLD)));
    if (cleanup_list.size() > clean_threshold) {
        int software_clean_size = g_settings.getInteger(Config::SOFTWARE_CLEAN_SIZE);
        const size_t cleanup_count
            = (software_clean_size <= 0) ? cleanup_list.size() : std::min(cleanup_list.size(), static_cast<size_t>(software_clean_size));

        for (size_t i = 0; i < cleanup_count; ++i) {
            uint32_t id = cleanup_list.front();
            cleanup_list.pop_front();

            if (id > 0 && id < sprites.getIconRendererSpace().size()) {
                sprites.getIconRendererSpace()[id].unloadDC();
            }
        }
    }
}

void TextureGC::garbageCollection(SpriteDatabase& db)
{
    if (g_settings.getInteger(Config::TEXTURE_MANAGEMENT)) {
        if (loaded_textures > g_settings.getInteger(Config::TEXTURE_CLEAN_THRESHOLD)
            && cached_time_ - lastclean > g_settings.getInteger(Config::TEXTURE_CLEAN_PULSE)) {

            int longevity = g_settings.getInteger(Config::TEXTURE_LONGEVITY);

            // We iterate backwards and handle the fact that img->clean() might call removeResidentImage()
            // which would remove the element from the vector.
            {
                std::lock_guard<std::recursive_mutex> lock(resident_images_mutex_);
                for (size_t i = resident_images.size(); i > 0; --i) {
                    ImageHandle handle = resident_images[i - 1];
                    Image* img = db.resolveImage(handle);
                    if (img) {
                        img->clean(cached_time_, longevity, db, *this);
                    } else {
                        // Invalid handle, remove it
                        resident_images[i - 1] = resident_images.back();
                        resident_images.pop_back();
                        loaded_textures.fetch_sub(1, std::memory_order_relaxed);
                    }

                    // If the image chose to unload, it already removed itself from resident_images
                    // via the recursive removeResidentImage call. We don't need to do anything else
                    // but the loop continues safely because its size-based and we are going backwards.
                }

                // Call clean on resident ui sprites
                for (uint32_t gs : resident_game_sprites) {
                    if (gs > 0 && gs < db.getIconRendererSpace().size()) {
                        db.getIconRendererSpace()[gs].clean(cached_time_, longevity);
                    }
                }
            }

            lastclean = cached_time_;
        }
    }
}

void TextureGC::onSettingsChanged(SpriteDatabase& db)
{
    // Clean software sprites on-demand when settings change (e.g. toggling texture management)
    cleanSoftwareSprites(db);
}

void TextureGC::cleanSoftwareSprites(SpriteDatabase& db)
{
    for (auto& icon : db.getIconRendererSpace()) {
        icon.unloadDC();
    }
}
