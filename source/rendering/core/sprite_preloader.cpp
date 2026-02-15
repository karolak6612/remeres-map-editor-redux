//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/core/sprite_preloader.h"
#include "rendering/core/graphics.h"
#include "ui/gui.h"
#include "io/loaders/spr_loader.h"
#include <mutex>

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

void SpritePreloader::preload(GameSprite* spr, int pattern_x, int pattern_y, int pattern_z, int frame) {
	if (!spr) {
		return;
	}

	// Capture global state once per preload call (one item)
	const std::string& sprfile = g_gui.gfx.getSpriteFile();
	const bool is_extended = g_gui.gfx.isExtended();
	const bool has_transparency = g_gui.gfx.hasTransparency();

	thread_local std::vector<uint32_t> ids_to_enqueue;
	ids_to_enqueue.clear();
	// Reserve for typical sprite sizes (1x1, 2x2, max layers etc) to minimize allocations
	if (ids_to_enqueue.capacity() < 64) {
		ids_to_enqueue.reserve(64);
	}

	for (int cx = 0; cx < spr->width; ++cx) {
		for (int cy = 0; cy < spr->height; ++cy) {
			for (int cf = 0; cf < spr->layers; ++cf) {
				int idx = spr->getIndex(cx, cy, cf, pattern_x, pattern_y, pattern_z, frame);

				if (idx >= (int)spr->numsprites) {
					if (spr->numsprites == 1) {
						idx = 0;
					} else {
						idx %= spr->numsprites;
					}
				}

				// Fix 1: Handle negative indices and strict bounds check
				if (idx < 0 || (size_t)idx >= spr->spriteList.size()) {
					continue;
				}

				GameSprite::NormalImage* img = spr->spriteList[idx];
				if (img && !img->isGLLoaded) {
					// Ensure parent is set so GC can invalidate cached_default_region
					// when evicting this sprite later (prevents stale cache → wrong sprite)
					img->parent = spr;
					ids_to_enqueue.push_back(img->id);
				}
			}
		}
	}

	if (!ids_to_enqueue.empty()) {
		std::lock_guard<std::mutex> lock(queue_mutex);
		if (task_queue.size() > MAX_QUEUE_SIZE) {
			return; // Drop requests if queue is slammed
		}

		for (uint32_t id : ids_to_enqueue) {
			if (pending_ids.insert(id).second) {
				// Capture the current generation ID of the sprite to detect stale tasks later
				uint64_t gen_id = 0;
				// We need to look up the sprite again to get generation_id, or cache it.
				// Since we just built ids_to_enqueue, we implicitly know the IDs.
				// However, ids_to_enqueue is just IDs. We should have captured generation_id earlier or valid pointer.
				// But we can't safely access 'img' here as it might have been invalidated if optimization was loop-hoisted?
				// Actually, 'img' was accessed inside the loop. 'ids_to_enqueue' is local thread storage.
				// We need to store pairs of (id, generation_id) in ids_to_enqueue, or look it up safely.
				// Safest is to look up in image_space since we are on main thread (preload called from draw).
				if (id < g_gui.gfx.image_space.size()) {
					auto& ptr = g_gui.gfx.image_space[id];
					if (ptr) {
						gen_id = ptr->generation_id;
					}
				}
				task_queue.push({ id, gen_id, sprfile, is_extended, has_transparency });
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

	while (!results.empty()) {
		Result& res = results.front();
		auto id = res.id;

		// Check if this ID was cancelled
		bool cancelled = false;
		{
			// We need to check cancelled_ids which is protected by mutex
			// But we don't want to hold the mutex for the whole loop if possible.
			// However, since we're in the main thread (update), and clear() also locks mutex,
			// we need to be careful.
			// Actually, let's just use the lock for the brief check if we were to be 100% safe,
			// but cancelled_ids is only modified in clear() (main thread) and update() (main thread).
			// Wait, clear() might be called from main thread too. A race with worker? No, worker doesn't touch cancelled_ids.
			// So it's safe to read/write cancelled_ids here without mutex IF clear() is main thread only.
			// SpritePreloader::clear() uses the mutex, implying it might be called from elsewhere?
			// The header says "Should be called when GraphicManager is cleared", which is main thread.
			// So we technically don't need the mutex for cancelled_ids if it's main-thread only.
			// BUT, let's be safe and assume clear() could be called during some emergency shutdown/reset.
			std::lock_guard<std::mutex> lock(queue_mutex);
			if (cancelled_ids.count(id)) {
				cancelled_ids.erase(id); // Consume the cancellation
				cancelled = true;
			}
		}

		if (!cancelled) {
			// Check if GraphicManager is loaded, for the correct sprite file, and ID is valid
			if (res.spritefile == g_gui.gfx.getSpriteFile() && !g_gui.gfx.isUnloaded() && id < g_gui.gfx.image_space.size()) {
				auto& img_ptr = g_gui.gfx.image_space[id];
				if (img_ptr) {
					// Use static_cast for performance, as we know the type from loaders
					// BUT we must verify it is indeed a NormalImage to avoid casting TemplateImage unsafely
					if (img_ptr->isNormalImage()) {
						auto* img = static_cast<GameSprite::NormalImage*>(img_ptr.get());

						// Fix 2 & 3: Validate Sprite Identity & Generation
						// Check ID match, Generation match, and GLLoaded state
						if (img && img->id == id && img->generation_id == res.generation_id && !img->isGLLoaded) {
							img->fulfillPreload(std::move(res.data));
						}
					}
				}
			}
		}

		ids_processed.push_back(id);
		results.pop();
	}

	if (!ids_processed.empty()) {
		std::lock_guard<std::mutex> lock(queue_mutex);
		for (const auto id : ids_processed) {
			pending_ids.erase(id);
		}
	}
}

namespace rme {
	void collectTileSprites(GameSprite* spr, int pattern_x, int pattern_y, int pattern_z, int frame) {
		SpritePreloader::get().preload(spr, pattern_x, pattern_y, pattern_z, frame);
		// Removed immediate update() call to allow true async processing
	}
}
