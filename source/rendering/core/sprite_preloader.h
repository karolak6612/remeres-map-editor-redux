//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CORE_SPRITE_PRELOADER_H_
#define RME_RENDERING_CORE_SPRITE_PRELOADER_H_
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

class SpriteDatabase;
class SpriteLoader;
class TextureGC;
class AtlasLifecycle;

class SpritePreloader {
public:
  SpritePreloader();

  SpritePreloader(const SpritePreloader &) = delete;
  SpritePreloader &operator=(const SpritePreloader &) = delete;

  // Schedules sprites for preloading based on the given view parameters.
  void preload(SpriteDatabase &sprites, SpriteLoader &loader, uint32_t clientID, int pattern_x, int pattern_y, int pattern_z,
               int frame);

  // Processes finished preload tasks and uploads data to the GPU.
  // Should be called on the main thread.
  void update(SpriteDatabase& sprites, AtlasLifecycle& atlas, TextureGC& gc, SpriteLoader& loader);

  // Clears all pending tasks and results.
  void clear();

  // Explicit shutdown to be called before global destruction
  void shutdown();

  ~SpritePreloader();

private:
  static constexpr unsigned int MIN_WORKER_THREADS = 1;
  static constexpr unsigned int MAX_WORKER_THREADS = 4;
  static constexpr size_t MAX_QUEUE_SIZE = 1024;

  struct Task {
    uint32_t id;
    uint32_t generation_id;
    std::string spritefile;
    bool is_extended;
    bool has_transparency;
  };

  struct Result {
    uint32_t id;
    uint32_t generation_id;
    std::unique_ptr<uint8_t[]> data;
    std::string spritefile;
  };

  void workerLoop(std::stop_token stop_token);

  std::vector<std::jthread> workers;
  std::mutex queue_mutex;
  std::condition_variable_any cv;
  std::queue<Task> task_queue;
  std::queue<Result> result_queue;
  std::unordered_set<uint32_t> pending_ids;

  bool stopping = false;
};

namespace rme {
// Specialized helper for tile rendering that collects all needed layers
void collectTileSprites(SpritePreloader &preloader, SpriteDatabase &sprites, SpriteLoader &loader, uint32_t clientID,
                        int pattern_x, int pattern_y, int pattern_z, int frame);
} // namespace rme

#endif
