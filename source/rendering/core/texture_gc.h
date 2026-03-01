#ifndef RME_RENDERING_CORE_TEXTURE_GC_H_
#define RME_RENDERING_CORE_TEXTURE_GC_H_

#include "rendering/core/render_timer.h"
#include "rendering/core/texture_garbage_collector.h"
#include <memory>
#include <vector>

class GameSprite;

class TextureGC {
public:
  TextureGC() {
    animation_timer = std::make_unique<RenderTimer>();
    animation_timer->Start();
  }
  ~TextureGC() = default;
  void NotifyTextureLoaded() { collector.NotifyTextureLoaded(); }
  void NotifyTextureUnloaded() { collector.NotifyTextureUnloaded(); }

  void clear() {
    resident_images.clear();
    resident_game_sprites.clear();
    collector.Clear();
  }

  void garbageCollection() {
    collector.GarbageCollect(resident_game_sprites, resident_images,
                             cached_time_);
  }

  void addSpriteToCleanup(GameSprite *spr) {
    collector.AddSpriteToCleanup(spr);
  }

  void updateTime() { cached_time_ = time(nullptr); }

  void pauseAnimation() { animation_timer->Pause(); }
  void resumeAnimation() { animation_timer->Resume(); }

  long getElapsedTime() const { return animation_timer->getElapsedTime(); }
  time_t getCachedTime() const { return cached_time_; }

  TextureGarbageCollector collector;

  std::vector<void *> resident_images;
  std::vector<GameSprite *> resident_game_sprites;

private:
  std::unique_ptr<RenderTimer> animation_timer;
  time_t cached_time_ = 0;
};

#endif
