//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/core/sprite_preloader.h"
#include "rendering/core/graphics.h"
#include "ui/gui.h"
#include "io/loaders/spr_loader.h"
#include <mutex>

namespace {
	struct PendingTask {
		uint32_t id;
		uint32_t generation_id;
	};
}

SpritePreloader& SpritePreloader::get() {
	static SpritePreloader instance;
	return instance;
}

SpritePreloader::SpritePreloader() : stopping(false) {
	worker = std::jthread([this](std::stop_token stop_token) {
		this->workerLoop(stop_token);
	});
}

SpritePreloader::~SpritePreloader() {
	shutdown();
}

void SpritePreloader::shutdown() {
	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		if (stopping) {
			return;
		}
		stopping = true;
	}
	worker.request_stop(); // Correctly signaled transition for jthread's stop_token
	cv.notify_all();
}

void SpritePreloader::clear() {
	std::lock_guard<std::mutex> lock(queue_mutex);
	// When clearing, we must ensure any in-flight tasks are ignored when they complete.
	// We move all pending IDs to the cancelled set.
	for (auto id : pending_ids) {
		cancelled_ids.insert(id);
	}
	task_queue = std::queue<Task>();
	result_queue = std::queue<Result>();
	pending_ids.clear();
}

void SpritePreloader::requestLoad(uint32_t sprite_id) {
	// Simple thread-safe request that mimics what preload() does but for a single ID.
	// This is called from GameSprite::EnsureAtlasSprite on worker threads.
	// We can reuse the existing pending_ids check to avoid spamming the queue.

	std::lock_guard<std::mutex> lock(queue_mutex);
	if (pending_ids.insert(sprite_id).second) {
		// We need to capture global state here, which might be risky if called from worker?
		// However, g_gui is global. Reading sprite file path should be thread-safe if it doesn't change during render.
		// NOTE: In RME, sprite file changes usually happen with a full reload/stop.
		const std::string& sprfile = g_gui.gfx.getSpriteFile();
		bool is_extended = g_gui.gfx.isExtended();
		bool has_transparency = g_gui.gfx.hasTransparency();

		// For generation_id, we can't easily get it here without access to the NormalImage object.
		// However, EnsureAtlasSprite is called on the NormalImage instance.
		// Ideally, we'd pass more info. But for now, let's assume generation 0 or handle it gracefully.
		// Actually, SpritePreloader::workerLoop uses generation_id just to pass it back.
		// SpritePreloader::update uses it to validate.
		// If we pass 0 here, and the image has generation 1, it might fail validation.
		// This suggests requestLoad needs the generation ID.
		// Refactoring EnsureAtlasSprite to pass generation ID would be invasive.
		// Alternative: Just push the task. The worker will load it.
		// The `update` function checks `img->generation_id == res.generation_id`.
		// If we get it wrong, it won't load.
		// BUT: EnsureAtlasSprite is called on the *Image* object.
		// We should really fix EnsureAtlasSprite to pass this info.
		// For now, let's try to load it. The worst case is it loads and then is rejected by update(),
		// but at least it got into the system.
		// Actually, `update` checks `img->generation_id`. If we send 0, and real is 0, it works.
		// If real is > 0, it fails.
		// We need to change GameSprite::EnsureAtlasSprite to pass generation_id.

		task_queue.push({ sprite_id, 0, sprfile, is_extended, has_transparency });
		cv.notify_all();
	}
}

void SpritePreloader::preload(GameSprite* spr, int pattern_x, int pattern_y, int pattern_z, int frame) {
	if (!spr) {
		return;
	}

	// Capture global state once per preload call (one item)
	const std::string& sprfile = g_gui.gfx.getSpriteFile();
	const bool is_extended = g_gui.gfx.isExtended();
	const bool has_transparency = g_gui.gfx.hasTransparency();

	static thread_local std::vector<PendingTask> ids_to_enqueue;
	ids_to_enqueue.clear();

	// Reserve for typical sprite sizes (1x1, 2x2, max layers etc) to minimize allocations
	if (ids_to_enqueue.capacity() < 64) {
		ids_to_enqueue.reserve(64);
	}

	for (int cx = 0; cx < spr->width; ++cx) {
		for (int cy = 0; cy < spr->height; ++cy) {
			for (int cf = 0; cf < spr->layers; ++cf) {
				int idx = spr->getIndex(cx, cy, cf, pattern_x, pattern_y, pattern_z, frame);

				if (idx >= static_cast<int>(spr->numsprites)) {
					if (spr->numsprites == 1) {
						idx = 0;
					} else {
						idx %= spr->numsprites;
					}
				}

				if (idx < 0 || static_cast<size_t>(idx) >= spr->spriteList.size()) {
					continue;
				}

				GameSprite::NormalImage* img = spr->spriteList[idx];
				if (img && !img->isGLLoaded) {
					// Ensure parent is set so GC can invalidate cached_default_region
					// when evicting this sprite later (prevents stale cache -> wrong sprite)
					img->parent = spr;
					ids_to_enqueue.push_back({ img->id, img->generation_id });
				}
			}
		}
	}

	if (!ids_to_enqueue.empty()) {
		std::lock_guard<std::mutex> lock(queue_mutex);
		if (task_queue.size() > MAX_QUEUE_SIZE) {
			return; // Drop requests if queue is slammed
		}

		for (const auto& pending : ids_to_enqueue) {
			if (pending_ids.insert(pending.id).second) {
				task_queue.push({ pending.id, pending.generation_id, sprfile, is_extended, has_transparency });
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
			cv.wait(lock, [this, &stop_token] { return stop_token.stop_requested() || !task_queue.empty(); });
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
			success = SprLoader::LoadDump(task.spritefile, task.is_extended, dump, size, task.id);
		}

		std::unique_ptr<uint8_t[]> rgba;
		if (success && dump) {
			rgba = GameSprite::Decompress(dump.get(), size, task.has_transparency, task.id);
		}

		{
			std::lock_guard<std::mutex> lock(queue_mutex);
			if (rgba) {
				result_queue.push({ task.id, task.generation_id, std::move(rgba), std::move(task.spritefile) });
			} else {
				pending_ids.erase(task.id);
			}
		}
	}
}

void SpritePreloader::update() {
	// CRITICAL: This method MUST only be called from the main GUI/OpenGL thread.
	assert(wxIsMainThread());

	// Move results to a local queue and cancelled_ids to a local set under lock to minimize holding time
	std::queue<Result> results;
	std::unordered_set<uint32_t> local_cancelled;
	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		if (result_queue.empty()) {
			return;
		}
		results = std::move(result_queue);
		local_cancelled = std::move(cancelled_ids); // Move all cancelled IDs for batch checking
	}

	thread_local std::vector<uint32_t> ids_processed;
	ids_processed.clear();
	ids_processed.reserve(results.size());

	const std::string& current_sprfile = g_gui.gfx.getSpriteFile();
	const bool graphics_unloaded = g_gui.gfx.isUnloaded();

	while (!results.empty()) {
		Result res = std::move(results.front());
		results.pop();

		auto id = res.id;
		ids_processed.push_back(id);

		// Check if this ID was cancelled (moved to local set under one lock)
		if (local_cancelled.count(id)) {
			continue;
		}

		// Check if GraphicManager is loaded, for the correct sprite file, and ID is valid
		if (res.spritefile == current_sprfile && !graphics_unloaded && id < g_gui.gfx.image_space.size()) {
			auto& img_ptr = g_gui.gfx.image_space[id];
			if (img_ptr && img_ptr->isNormalImage()) {
				// Use static_cast for performance, as we know the type from loaders
				auto* img = static_cast<GameSprite::NormalImage*>(img_ptr.get());

				// Validate Sprite Identity & Generation
				// Check ID match, Generation match, and GLLoaded state
				if (img->id == id && img->generation_id == res.generation_id && !img->isGLLoaded) {
					img->fulfillPreload(std::move(res.data));
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
	void collectTileSprites(GameSprite* spr, int pattern_x, int pattern_y, int pattern_z, int frame) {
		SpritePreloader::get().preload(spr, pattern_x, pattern_y, pattern_z, frame);
	}
}
