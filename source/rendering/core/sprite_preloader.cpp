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

SpritePreloader::SpritePreloader() {
	worker = std::thread(&SpritePreloader::workerLoop, this);
}

SpritePreloader::~SpritePreloader() {
	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		stopping = true;
	}
	cv.notify_one();
	if (worker.joinable()) {
		worker.join();
	}
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
	if (!spr) return;

	// This loop logic mirrors the inefficient synchronous loop, but queues async tasks instead.
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

				if (idx < 0 || (size_t)idx >= spr->spriteList.size()) continue;

				GameSprite::NormalImage* img = spr->spriteList[idx];
				if (!img || img->isGLLoaded) continue; // Already loaded or null

				{
					std::lock_guard<std::mutex> lock(queue_mutex);
					if (pending_ids.find(img->id) == pending_ids.end()) {
						pending_ids.insert(img->id);
						task_queue.push({img->id});
						cv.notify_one();
					}
				}
			}
		}
	}
}

void SpritePreloader::workerLoop() {
	while (true) {
		Task task;
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			cv.wait(lock, [this] { return stopping || !task_queue.empty(); });
			if (stopping) break;
			task = task_queue.front();
			task_queue.pop();
		}

		std::unique_ptr<uint8_t[]> dump;
		uint16_t size = 0;
		// Safe to access g_gui.gfx members as long as they are thread-safe or read-only during runtime
		// spritefile string is constant after load
		bool success = SprLoader::LoadDump(&g_gui.gfx, dump, size, task.id);

		if (success && dump) {
			// Decompress
			// hasTransparency() reads a bool, thread-safe
			auto rgba = GameSprite::Decompress(dump.get(), size, g_gui.gfx.hasTransparency(), task.id);
			if (rgba) {
				std::lock_guard<std::mutex> lock(queue_mutex);
				result_queue.push({task.id, std::move(rgba)});
			} else {
				// Failed to decompress
				std::lock_guard<std::mutex> lock(queue_mutex);
				pending_ids.erase(task.id);
			}
		} else {
			// Failed to load dump
			std::lock_guard<std::mutex> lock(queue_mutex);
			pending_ids.erase(task.id);
		}
	}
}

void SpritePreloader::update() {
	std::queue<Result> results;
	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		if (result_queue.empty()) return;
		std::swap(results, result_queue);
	}

	while (!results.empty()) {
		Result& res = results.front();
		uint32_t id = res.id;

		// Check if GraphicManager is loaded and ID is valid
		if (!g_gui.gfx.isUnloaded() && id < g_gui.gfx.image_space.size()) {
			auto& img_ptr = g_gui.gfx.image_space[id];
			if (img_ptr) {
				// We assume it's NormalImage because it came from sprite file loading process
				// and image_space typically contains NormalImage (or derived from Image)
				auto* img = static_cast<GameSprite::NormalImage*>(img_ptr.get());
				if (img && !img->isGLLoaded) {
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
	SpritePreloader::get().update();
}
