//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/core/sprite_preloader.h"
#include "app/settings.h"
#include "io/loaders/spr_loader.h"
#include "rendering/core/atlas_lifecycle.h"
#include "rendering/core/normal_image.h"
#include "rendering/core/sprite_database.h"
#include "rendering/core/sprite_decompression.h"
#include "rendering/core/texture_gc.h"
#include "rendering/io/sprite_loader.h"
#include "ui/gui.h"
#include <mutex>
#include <span>

namespace {
struct PendingTask {
  uint32_t id;
  uint32_t generation_id;
};
} // namespace

SpritePreloader::SpritePreloader() : stopping(false) {
  unsigned int num_threads = std::clamp(std::thread::hardware_concurrency(),
                                        MIN_WORKER_THREADS, MAX_WORKER_THREADS);
  workers.reserve(num_threads);
  for (unsigned int i = 0; i < num_threads; ++i) {
    workers.emplace_back(
        [this](std::stop_token stop_token) { this->workerLoop(stop_token); });
  }
}

SpritePreloader::~SpritePreloader() { shutdown(); }

void SpritePreloader::shutdown() {
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    if (stopping) {
      return;
    }
    stopping = true;
  }
  for (auto &worker : workers) {
    worker.request_stop(); 
  }
  cv.notify_all();
}

void SpritePreloader::clear() {
  std::lock_guard<std::mutex> lock(queue_mutex);
  task_queue = std::queue<Task>();
  result_queue = std::queue<Result>();
  pending_ids.clear();
}

void SpritePreloader::preload(SpriteDatabase &sprites, SpriteLoader &loader,
                                uint32_t clientID, int pattern_x, int pattern_y,
                                int pattern_z, int frame) {
  if (clientID == 0 || clientID >= sprites.getMetadataSpace().size() ||
      clientID >= sprites.getAtlasCacheSpace().size()) {
    return;
  }
  const SpriteMetadata &meta = sprites.getMetadataSpace()[clientID];
  SpriteAtlasCache &atlas = sprites.getAtlasCacheSpace()[clientID];

  const std::string &sprfile = loader.getSpriteFile();
  const bool is_extended = loader.isExtended();
  const bool has_transparency = loader.hasTransparency();

  static thread_local std::vector<PendingTask> ids_to_enqueue;
  ids_to_enqueue.clear();

  if (ids_to_enqueue.capacity() < 64) {
    ids_to_enqueue.reserve(64);
  }

  for (int cx = 0; cx < meta.width; ++cx) {
    for (int cy = 0; cy < meta.height; ++cy) {
      for (int cf = 0; cf < meta.layers; ++cf) {
        int idx =
            meta.getIndex(cx, cy, cf, pattern_x, pattern_y, pattern_z, frame);

        if (idx >= static_cast<int>(meta.numsprites)) {
          if (meta.numsprites <= 1) {
            idx = 0;
          } else {
            idx %= meta.numsprites;
          }
        }

        if (idx < 0 || static_cast<size_t>(idx) >= atlas.spriteList.size()) {
          continue;
        }

        uint32_t spr_index = atlas.spriteList[idx];
        auto& space = sprites.getNormalImageSpace();
        if (spr_index >= space.size()) continue;

        NormalImage *img = &space[spr_index];
        if (!img->isGLLoaded) {
          img->clientID = clientID;
          ids_to_enqueue.push_back({img->id, img->generation_id});
        }
      }
    }
  }

  if (!ids_to_enqueue.empty()) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    for (const auto &pending : ids_to_enqueue) {
      if (task_queue.size() >= MAX_QUEUE_SIZE) {
        break; 
      }
      if (pending_ids.insert(pending.id).second) {
        task_queue.push({pending.id, pending.generation_id, sprfile,
                         is_extended, has_transparency});
      }
    }
    cv.notify_all();
  }
}

void SpritePreloader::workerLoop(std::stop_token stop_token) {
  while (!stop_token.stop_requested()) {
    Task task;
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      cv.wait(lock, [this, &stop_token] {
        return stop_token.stop_requested() || !task_queue.empty();
      });
      if (stop_token.stop_requested()) {
        break;
      }
      task = std::move(task_queue.front());
      task_queue.pop();
    }

    std::unique_ptr<uint8_t[]> dump;
    uint16_t size = 0;
    bool success = false;

    if (!task.spritefile.empty()) {
      success = SprLoader::LoadDump(task.spritefile, task.is_extended, dump,
                                    size, static_cast<int>(task.id));
    }

    std::unique_ptr<uint8_t[]> rgba;
    if (success && dump) {
      rgba = decompress_sprite(std::span<const uint8_t>{dump.get(), size},
                                task.has_transparency, static_cast<int>(task.id));
    }

    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      if (rgba) {
        result_queue.push({task.id, task.generation_id, std::move(rgba),
                           std::move(task.spritefile)});
      } else {
        pending_ids.erase(task.id);
      }
    }
  }
}

void SpritePreloader::update(SpriteDatabase& sprites, AtlasLifecycle& atlas_lifecycle, TextureGC& gc, SpriteLoader& loader) {
  assert(wxIsMainThread());

  std::queue<Result> results;
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    if (result_queue.empty()) {
      return;
    }
    std::swap(results, result_queue);
  }

  thread_local std::vector<uint32_t> ids_processed;
  ids_processed.clear();
  ids_processed.reserve(results.size());

  const std::string &current_sprfile = loader.getSpriteFile();
  const bool graphics_unloaded = loader.isUnloaded();

  while (!results.empty()) {
    Result res = std::move(results.front());
    results.pop();

    uint32_t id = res.id;
    ids_processed.push_back(id);

    auto &normalSpace = sprites.getNormalImageSpace();
    if (res.spritefile == current_sprfile && !graphics_unloaded &&
        static_cast<size_t>(id) < normalSpace.size()) {
      NormalImage *img = &normalSpace[id];

      if (img->id == id && img->generation_id == res.generation_id &&
          !img->isGLLoaded) {
        if (atlas_lifecycle.hasAtlasManager()) {
            bool use_memcached = false; 
            img->fulfillPreload(*(atlas_lifecycle.getAtlasManager()), gc, loader, use_memcached, std::move(res.data));
        }
      }
    }
  }

  if (!ids_processed.empty()) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    for (uint32_t id : ids_processed) {
      pending_ids.erase(id);
    }
  }
}

namespace rme {
void collectTileSprites(SpritePreloader &preloader, SpriteDatabase &sprites,
                        SpriteLoader &loader, uint32_t clientID, int pattern_x,
                        int pattern_y, int pattern_z, int frame) {
  preloader.preload(sprites, loader, clientID, pattern_x, pattern_y, pattern_z,
                    frame);
}
} // namespace rme
