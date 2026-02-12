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
	std::queue<Task> empty;
	std::swap(task_queue, empty);
	std::queue<Result> empty_res;
	std::swap(result_queue, empty_res);
	pending_ids.clear();
}

void SpritePreloader::preload(GameSprite* spr, int pattern_x, int pattern_y, int pattern_z, int frame) {
	if (!spr) {
		return;
	}

	// Capture global state once per preload call (one item)
	const std::string sprfile = g_gui.gfx.getSpriteFile();
	const bool is_extended = g_gui.gfx.isExtended();
	const bool has_transparency = g_gui.gfx.hasTransparency();

	std::vector<uint32_t> ids_to_enqueue;

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

				if (idx < 0 || (size_t)idx >= spr->spriteList.size()) {
					continue;
				}

				GameSprite::NormalImage* img = spr->spriteList[idx];
				if (img && !img->isGLLoaded) {
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
				task_queue.push({ id, sprfile, is_extended, has_transparency });
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
				result_queue.push({ task.id, std::move(rgba) });
			} else {
				pending_ids.erase(task.id);
			}
		}
	}
}

void SpritePreloader::update() {
	// CRITICAL: This method MUST only be called from the main GUI/OpenGL thread.
	std::queue<Result> results;
	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		if (result_queue.empty()) {
			return;
		}
		std::swap(results, result_queue);
	}

	while (!results.empty()) {
		Result& res = results.front();
		uint32_t id = res.id;

		// Check if GraphicManager is loaded and ID is valid
		if (!g_gui.gfx.isUnloaded() && id < g_gui.gfx.image_space.size()) {
			auto& img_ptr = g_gui.gfx.image_space[id];
			if (img_ptr) {
				// Use static_cast for performance, as we know the type from loaders
				auto* img = static_cast<GameSprite::NormalImage*>(img_ptr.get());
				assert(img);
				if (!img->isGLLoaded) {
					img->fulfillPreload(std::move(res.data));
				}
			}
		}

		{
			std::lock_guard<std::mutex> lock(queue_mutex);
			pending_ids.erase(id);
		}

		results.pop();
	}
}

void collectTileSprites(GameSprite* spr, int pattern_x, int pattern_y, int pattern_z, int frame) {
	SpritePreloader::get().preload(spr, pattern_x, pattern_y, pattern_z, frame);
	// Removed immediate update() call to allow true async processing
}
