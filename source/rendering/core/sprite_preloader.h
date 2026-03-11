//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CORE_SPRITE_PRELOADER_H_
#define RME_RENDERING_CORE_SPRITE_PRELOADER_H_

#include "rendering/core/game_sprite.h"
#include "rendering/core/spsc_queue.h"
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <cstdint>
#include <queue>
#include <unordered_set>

class SpriteArchive;
class GraphicManager;

class SpritePreloader {
public:
	SpritePreloader();
	~SpritePreloader();

	SpritePreloader(const SpritePreloader&) = delete;
	SpritePreloader& operator=(const SpritePreloader&) = delete;

	// Inject the GraphicManager after construction (avoids circular dependency
	// since GraphicManager owns TextureGC which owns SpritePreloader).
	void setGraphicManager(GraphicManager* gfx) { gfx_ = gfx; }

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

	struct PendingSpriteKey {
		ArchiveSpriteKey key;
		uint32_t generation_id = 0;
		uint64_t epoch = 0;

		bool operator==(const PendingSpriteKey& other) const = default;
	};

	struct PendingSpriteKeyHash {
		size_t operator()(const PendingSpriteKey& key) const noexcept {
			size_t seed = ArchiveSpriteKeyHash {}(key.key);
			seed ^= std::hash<uint32_t> {}(key.generation_id) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= std::hash<uint64_t> {}(key.epoch) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};

	struct Task {
		PendingSpriteKey pending;
		std::shared_ptr<SpriteArchive> archive;
		bool has_transparency;
	};

	struct Result {
		PendingSpriteKey pending;
		std::unique_ptr<uint8_t[]> data;
		std::shared_ptr<SpriteArchive> archive;
	};

	struct WorkerState {
		SPSCQueue<Result, 1024> results;
		std::jthread thread;
	};

	void workerLoop(std::stop_token stop_token, WorkerState& worker);

	static constexpr unsigned int MIN_WORKER_THREADS = 2u;
	static constexpr unsigned int MAX_WORKER_THREADS = 8u;

	static constexpr size_t MAX_QUEUE_SIZE = 50000; // Limit pending tasks to prevent memory blowup

	std::mutex task_mutex_;    // Protects task_queue, pending_ids, stopping, active_epoch
	std::condition_variable cv;
	bool stopping = false;
	std::vector<std::unique_ptr<WorkerState>> workers;

	std::queue<Task> task_queue;
	std::unordered_set<PendingSpriteKey, PendingSpriteKeyHash> pending_ids; // To avoid duplicate tasks for the same archive/id/generation/epoch
	uint64_t active_epoch = 0;

	GraphicManager* gfx_ = nullptr;
};

#endif
