#include "app/main.h"
#include "rendering/core/sprite_preload_queue.h"
#include "rendering/core/sprite_preloader.h"

void SpritePreloadQueue::processAll()
{
    if (!preloader_) {
        return;
    }
    for (const auto& req : requests) {
        preloader_->preload(req.sprite, req.pattern_x, req.pattern_y, req.pattern_z, req.frame);
    }
}
