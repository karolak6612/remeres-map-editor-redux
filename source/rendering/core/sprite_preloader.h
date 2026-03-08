//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CORE_SPRITE_PRELOADER_H_
#define RME_RENDERING_CORE_SPRITE_PRELOADER_H_

#include "rendering/core/game_sprite.h"
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <unordered_set>

class SpriteArchive;

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

	struct ArchiveSpriteKey {
		const SpriteArchive* archive = nullptr;
		uint32_t id = 0;

		bool operator==(const ArchiveSpriteKey& other) const = default;
	};

	struct ArchiveSpriteKeyHash {
		size_t operator()(const ArchiveSpriteKey& key) const noexcept {
			size_t seed = std::hash<const SpriteArchive*> {}(key.archive);
			seed ^= std::hash<uint32_t> {}(key.id) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};

	struct Task {
		ArchiveSpriteKey key;
		uint32_t generation_id;
		std::shared_ptr<SpriteArchive> archive;
		bool has_transparency;
	};

	struct Result {
		ArchiveSpriteKey key;
		uint32_t generation_id;
		std::unique_ptr<uint8_t[]> data;
		std::shared_ptr<SpriteArchive> archive;
	};

	void workerLoop(std::stop_token stop_token);

	static constexpr unsigned int MIN_WORKER_THREADS = 2u;
	static constexpr unsigned int MAX_WORKER_THREADS = 8u;

	static constexpr size_t MAX_QUEUE_SIZE = 50000; // Limit pending tasks to prevent memory blowup

	std::mutex queue_mutex;
	std::condition_variable cv;
	bool stopping = false;
	std::vector<std::jthread> workers;

	std::queue<Task> task_queue;
	std::queue<Result> result_queue;
	std::unordered_set<ArchiveSpriteKey, ArchiveSpriteKeyHash> pending_ids; // To avoid duplicate tasks per archive
	std::unordered_set<ArchiveSpriteKey, ArchiveSpriteKeyHash> cancelled_ids; // Keys that were cleared and should be ignored
};

namespace rme {
	void collectTileSprites(GameSprite* spr, int pattern_x, int pattern_y, int pattern_z, int frame);
}

#endif
