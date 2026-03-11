//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CORE_TEXTURE_GC_H_
#define RME_RENDERING_CORE_TEXTURE_GC_H_

#include <memory>
#include <ctime>
#include "rendering/core/texture_garbage_collector.h"

class GameSprite;
class SpriteDatabase;
class SpritePreloader;

class TextureGC {
public:
	TextureGC();
	~TextureGC();

	void updateTime();
	time_t getCachedTime() const { return cached_time_; }

	void addSpriteToCleanup(SpriteDatabase& db, uint32_t sprite_id, const GraphicsRuntimeConfig& config);
	void garbageCollect(SpriteDatabase& db, const GraphicsRuntimeConfig& config);
	void cleanSoftwareSprites(SpriteDatabase& db);
	void clear();

	TextureGarbageCollector& collector() { return collector_; }

	// Sprite preloader (background decompression threads)
	SpritePreloader& preloader() { return *preloader_; }

	// GC scheduling: returns true if enough time has elapsed since last collection.
	bool shouldCollect() const noexcept { return (cached_time_ - last_gc_time_) >= 1; }
	void markCollected() noexcept { last_gc_time_ = cached_time_; }

private:
	TextureGarbageCollector collector_;
	std::unique_ptr<SpritePreloader> preloader_;
	time_t cached_time_ = 0;
	time_t last_gc_time_ = 0;
};

#endif
