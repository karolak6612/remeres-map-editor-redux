#ifndef RME_RENDERING_CORE_TEXTURE_GC_H_
#define RME_RENDERING_CORE_TEXTURE_GC_H_

#include "rendering/core/render_timer.h"
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

class GameSprite;
class Sprite;
class SpriteDatabase;
class SpriteDatabase;
#include "rendering/core/image.h" // requires ImageHandle

class TextureGC {
public:
    TextureGC();
    ~TextureGC();

    TextureGC(const TextureGC&) = delete;
    TextureGC& operator=(const TextureGC&) = delete;

    void clear();
    void updateTime();

    long getElapsedTime() const
    {
        return animation_timer->getElapsedTime();
    }
    std::time_t getCachedTime() const
    {
        return cached_time_;
    }

    void pauseAnimation()
    {
        animation_timer->Pause();
    }
    void resumeAnimation()
    {
        animation_timer->Resume();
    }

    void garbageCollection(SpriteDatabase& db);
    void cleanSoftwareSprites(SpriteDatabase& db);
    void onSettingsChanged(SpriteDatabase& db);
    void addSpriteToCleanup(uint32_t spr_id);

    int getLoadedTexturesCount() const
    {
        return loaded_textures.load(std::memory_order_relaxed);
    }

    // Controlled API for resident sets
    void addResidentImage(ImageHandle handle);
    void removeResidentImage(ImageHandle handle);
    bool containsResidentImage(ImageHandle handle) const;

    void addResidentGameSprite(uint32_t clientID);
    void removeResidentGameSprite(uint32_t clientID);
    bool containsResidentGameSprite(uint32_t clientID) const;

private:
    std::unique_ptr<RenderTimer> animation_timer;
    std::time_t cached_time_ = 0;

    std::atomic<int> loaded_textures = 0;
    std::time_t lastclean = 0;
    int clean_software_counter = 0;
    std::deque<uint32_t> cleanup_list;

    mutable std::recursive_mutex resident_images_mutex_;
    std::vector<ImageHandle> resident_images;
    std::unordered_set<uint32_t> resident_game_sprites;
};

#endif
