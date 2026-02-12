//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CORE_SPRITE_PRELOADER_H_
#define RME_RENDERING_CORE_SPRITE_PRELOADER_H_

#include "rendering/core/game_sprite.h"
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <unordered_set>

class SpritePreloader {
public:
	[[nodiscard]] static SpritePreloader& get();

	SpritePreloader(const SpritePreloader&) = delete;
	SpritePreloader& operator=(const SpritePreloader&) = delete;

	// Schedules sprites for preloading based on the given view parameters.
	// This corresponds to the loop logic previously in collectTileSprites.
	void preload(GameSprite* spr, int pattern_x, int pattern_y, int pattern_z, int frame);

	// Processes finished preload tasks and uploads data to the GPU.
	// Should be called on the main thread.
	void update();

	// Clears all pending tasks and results.
	// Should be called when GraphicManager is cleared.
	void clear();

	// Explicit shutdown to be called before global destruction
	void shutdown();

private:
	SpritePreloader();
	~SpritePreloader();

	struct Task {
		uint32_t id;
		std::string spritefile;
		bool is_extended;
		bool has_transparency;
		uint64_t priority;

		bool operator<(const Task& other) const {
			return priority < other.priority;
		}
	};

	struct Result {
		uint32_t id;
		std::unique_ptr<uint8_t[]> data;
	};

	void workerLoop(std::stop_token stop_token);

	static constexpr size_t MAX_QUEUE_SIZE = 50000; // Limit pending tasks to prevent memory blowup

	std::mutex queue_mutex;
	std::condition_variable cv;
	bool stopping = false;
	std::jthread worker;

	std::priority_queue<Task> task_queue;
	std::queue<Result> result_queue;
	std::unordered_set<uint32_t> pending_ids; // To avoid duplicate tasks
	uint64_t request_counter = 0;
};

namespace rme {
	void collectTileSprites(GameSprite* spr, int pattern_x, int pattern_y, int pattern_z, int frame);
}

#endif
