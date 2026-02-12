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
	cv.notify_all();
	// jthread will join on destruction/assignment, but explicit request_stop is handled by stop_token
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

				if (idx < 0 || (size_t)idx >= spr->spriteList.size()) {
					continue;
				}

				GameSprite::NormalImage* img = spr->spriteList[idx];
				if (!img || img->isGLLoaded) {
					continue; // Already loaded or null
				}

				{
					std::lock_guard<std::mutex> lock(queue_mutex);
					if (pending_ids.find(img->id) == pending_ids.end()) {
						pending_ids.insert(img->id);

						std::string sprfile = g_gui.gfx.getSpriteFile();
						bool is_extended = g_gui.gfx.isExtended();
						bool has_transparency = g_gui.gfx.hasTransparency();

						task_queue.push({ img->id, std::move(sprfile), is_extended, has_transparency });
						cv.notify_one();
					}
				}
			}
		}
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
				// Use dynamic_cast for safer type conversion
				auto* img = dynamic_cast<GameSprite::NormalImage*>(img_ptr.get());
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
	// Removed immediate update() call to allow true async processing
}
