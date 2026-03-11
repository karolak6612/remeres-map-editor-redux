//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/core/sprite_preloader.h"
#include "rendering/core/graphics.h"
#include "rendering/core/normal_image.h"
#include "rendering/core/sprite_archive.h"

#include <algorithm>
#include <cassert>
#include <span>

SpritePreloader::SpritePreloader() : stopping(false) {
	unsigned int num_threads = std::clamp(std::thread::hardware_concurrency(), MIN_WORKER_THREADS, MAX_WORKER_THREADS);
	workers.reserve(num_threads);
	for (unsigned int i = 0; i < num_threads; ++i) {
		workers.emplace_back([this](std::stop_token stop_token) {
			this->workerLoop(stop_token);
		});
	}
}

SpritePreloader::~SpritePreloader() {
	shutdown();
}

void SpritePreloader::shutdown() {
	{
		std::lock_guard<std::mutex> lock(task_mutex_);
		if (stopping) {
			return;
		}
		stopping = true;
	}
	for (auto& worker : workers) {
		worker.request_stop(); // Correctly signaled transition for jthread's stop_token
	}
	cv.notify_all();
}

void SpritePreloader::clear() {
	{
		std::lock_guard<std::mutex> lock(task_mutex_);
		// Bump the epoch so any in-flight worker result becomes stale.
		++active_epoch;
		task_queue = std::queue<Task>();
		pending_ids.clear();
	}
	{
		std::lock_guard<std::mutex> lock(result_mutex_);
		result_buffer_.clear();
	}
}

void SpritePreloader::preload(GameSprite* spr, int pattern_x, int pattern_y, int pattern_z, int frame) {
	if (!spr || !gfx_) {
		return;
	}

	const auto archive = gfx_->getSpriteArchive();
	const bool has_transparency = gfx_->hasTransparency();
	if (!archive) {
		return;
	}

	struct PendingTask {
		ArchiveSpriteKey key;
		uint32_t generation_id = 0;
	};

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

				NormalImage* img = spr->spriteList[idx];
				if (img && !img->isGLLoaded) {
					ids_to_enqueue.push_back({ { archive.get(), img->id }, img->generation_id });
				}
			}
		}
	}

	if (!ids_to_enqueue.empty()) {
		std::lock_guard<std::mutex> lock(task_mutex_);
		if (task_queue.size() > MAX_QUEUE_SIZE) {
			return; // Drop requests if queue is slammed
		}

		for (const auto& pending : ids_to_enqueue) {
			const PendingSpriteKey pending_key {
				.key = pending.key,
				.generation_id = pending.generation_id,
				.epoch = active_epoch,
			};
			if (pending_ids.insert(pending_key).second) {
				task_queue.push({ pending_key, archive, has_transparency });
			}
		}
		cv.notify_all();
	}
}

void SpritePreloader::workerLoop(std::stop_token stop_token) {
	while (!stop_token.stop_requested()) {
		Task task;
		{
			std::unique_lock<std::mutex> lock(task_mutex_);
			cv.wait(lock, [this, &stop_token] { return stop_token.stop_requested() || !task_queue.empty(); });
			if (stop_token.stop_requested()) {
				break;
			}
			task = std::move(task_queue.front());
			task_queue.pop();
		}

		std::unique_ptr<uint8_t[]> dump;
		uint16_t size = 0;
		const bool success = task.archive && task.archive->readCompressed(task.pending.key.id, dump, size);

		std::unique_ptr<uint8_t[]> rgba;
		if (success && dump) {
			rgba = GameSprite::Decompress(std::span { dump.get(), size }, task.has_transparency, task.pending.key.id);
		}

		if (rgba) {
			// Result push uses separate mutex — no contention with task popping
			std::lock_guard<std::mutex> lock(result_mutex_);
			result_buffer_.push_back({ task.pending, std::move(rgba), std::move(task.archive) });
		} else {
			std::lock_guard<std::mutex> lock(task_mutex_);
			pending_ids.erase(task.pending);
		}
	}
}

void SpritePreloader::update() {
	// CRITICAL: This method MUST only be called from the main GUI/OpenGL thread.
	assert(wxIsMainThread());

	// Swap result buffer under result_mutex_ — brief lock, no contention with task popping.
	std::vector<Result> results;
	{
		std::lock_guard<std::mutex> lock(result_mutex_);
		if (result_buffer_.empty()) {
			return;
		}
		std::swap(results, result_buffer_);
	}

	// Read epoch under task_mutex_ (separate from result drain).
	uint64_t current_epoch;
	{
		std::lock_guard<std::mutex> lock(task_mutex_);
		current_epoch = active_epoch;
	}

	thread_local std::vector<PendingSpriteKey> keys_processed;
	keys_processed.clear();
	keys_processed.reserve(results.size());

	const auto current_archive = gfx_->getSpriteArchive();
	const bool graphics_unloaded = gfx_->isUnloaded();

	for (auto& res : results) {
		const auto pending = res.pending;
		const auto id = pending.key.id;
		keys_processed.push_back(pending);

		if (pending.epoch != current_epoch) {
			continue;
		}

		// Check if GraphicManager is loaded, for the correct sprite file, and ID is valid
		if (res.archive == current_archive && !graphics_unloaded && id < gfx_->db().images().size()) {
			auto& img_ptr = gfx_->db().images()[id];
			if (img_ptr && img_ptr->isNormalImage()) {
				// Use static_cast for performance, as we know the type from loaders
				auto* img = static_cast<NormalImage*>(img_ptr.get());

				// Validate Sprite Identity & Generation
				// Check ID match, Generation match, and GLLoaded state
				if (img->id == id && img->generation_id == pending.generation_id && !img->isGLLoaded) {
					img->fulfillPreload(std::move(res.data));
				}
			}
		}
	}

	if (!keys_processed.empty()) {
		std::lock_guard<std::mutex> lock(task_mutex_);
		for (const auto& pending : keys_processed) {
			pending_ids.erase(pending);
		}
	}
}
