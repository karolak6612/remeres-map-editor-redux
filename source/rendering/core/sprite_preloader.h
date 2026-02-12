//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CORE_SPRITE_PRELOADER_H_
#define RME_RENDERING_CORE_SPRITE_PRELOADER_H_

#include "rendering/core/game_sprite.h"
#include <memory>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <unordered_set>

class SpritePreloader {
public:
	static SpritePreloader& get();

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

private:
	SpritePreloader();
	~SpritePreloader();

	struct Task {
		uint32_t id;
	};

	struct Result {
		uint32_t id;
		std::unique_ptr<uint8_t[]> data;
	};

	void workerLoop();

	std::thread worker;
	std::mutex queue_mutex;
	std::condition_variable cv;
	bool stopping = false;

	std::queue<Task> task_queue;
	std::queue<Result> result_queue;
	std::unordered_set<uint32_t> pending_ids; // To avoid duplicate tasks
};

// Helper function to replace the inefficient loop
void collectTileSprites(GameSprite* spr, int pattern_x, int pattern_y, int pattern_z, int frame);

#endif
