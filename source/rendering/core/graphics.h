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

#ifndef RME_GRAPHICS_H_
#define RME_GRAPHICS_H_

#include "game/outfit.h"
#include "util/common.h"
#include <deque>
#include <memory>
#include <map>

struct NVGcontext;
struct NVGDeleter {
	void operator()(NVGcontext* nvg) const;
};

#include <unordered_map>
#include <list>
#include <vector>

#include "app/client_version.h"

#include "animator.h"

class MapCanvas;
class GraphicManager;
class FileReadHandle;
class Animator;
class SpriteArchive;

#include "rendering/core/sprite_light.h"
#include "rendering/core/texture_garbage_collector.h"
#include "rendering/core/render_timer.h"
#include "rendering/core/atlas_manager.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/image.h"
#include "rendering/core/normal_image.h"
#include "rendering/core/template_image.h"
#include "rendering/core/sprite_database.h"

class GraphicManager {
public:
	GraphicManager();
	~GraphicManager();

	void clear();
	void cleanSoftwareSprites();

	// --- Sprite Database delegation (backward compatible) ---
	Sprite* getSprite(int id) { return sprites.getSprite(id); }
	GameSprite* getCreatureSprite(int id) { return sprites.getCreatureSprite(id); }
	void insertSprite(int id, std::unique_ptr<Sprite> sprite) { sprites.insertSprite(id, std::move(sprite)); }
	void insertSprite(int id, Sprite* sprite) { sprites.insertSprite(id, std::unique_ptr<Sprite>(sprite)); }
	uint16_t getItemSpriteMaxID() const { return sprites.getItemSpriteMaxID(); }
	uint16_t getCreatureSpriteMaxID() const { return sprites.getCreatureSpriteMaxID(); }

	// Direct access to the sprite database
	SpriteDatabase& getSpriteDatabase() { return sprites; }
	const SpriteDatabase& getSpriteDatabase() const { return sprites; }

	// --- Animation ---
	void updateTime();

	void pauseAnimation() {
		animation_timer->Pause();
	}
	void resumeAnimation() {
		animation_timer->Resume();
	}

	long getElapsedTime() const {
		return animation_timer->getElapsedTime();
	}

	time_t getCachedTime() const {
		return cached_time_;
	}

	// --- Lifecycle ---
	bool loadEditorSprites();
	void garbageCollection();
	void addSpriteToCleanup(GameSprite* spr);

	// --- Texture tracking (replaces friend access from Image/NormalImage/TemplateImage) ---
	void notifyTextureLoaded() { collector.NotifyTextureLoaded(); }
	void notifyTextureUnloaded() { collector.NotifyTextureUnloaded(); }
	void addResidentImage(void* img) { resident_images.push_back(img); }

	// --- Accessors ---
	wxFileName getMetadataFileName() const {
		return client_version ? client_version->getMetadataPath() : wxFileName();
	}
	wxFileName getSpritesFileName() const {
		return client_version ? client_version->getSpritesPath() : wxFileName();
	}

	bool hasTransparency() const;
	bool isUnloaded() const;

	const std::string& getSpriteFile() const {
		return spritefile;
	}
	bool isExtended() const {
		return is_extended;
	}
	std::shared_ptr<SpriteArchive> getSpriteArchive() const {
		return sprite_archive_;
	}

	ClientVersion* client_version;

	// Sprite Atlas (Phase 2) - manages all game sprites in a texture array
	AtlasManager* getAtlasManager() {
		return atlas_manager_.get();
	}
	bool hasAtlasManager() const {
		return atlas_manager_ != nullptr && atlas_manager_->isValid();
	}
	bool ensureAtlasManager();

private:
	SpriteDatabase sprites;

	std::atomic<bool> unloaded;
	std::string spritefile;
	std::shared_ptr<SpriteArchive> sprite_archive_;

	std::unique_ptr<AtlasManager> atlas_manager_ = nullptr;

	// Active Resident Sets: Track only what's currently occupying memory/VRAM
	std::vector<void*> resident_images;
	std::vector<GameSprite*> resident_game_sprites;

	DatFormat dat_format;
	bool is_extended;
	bool has_transparency;
	bool has_frame_durations;
	bool has_frame_groups;
	TextureGarbageCollector collector;

	std::unique_ptr<RenderTimer> animation_timer;
	time_t cached_time_ = 0;

	// GraphicsAssembler is the loader — needs write access to all metadata fields
	friend class GraphicsAssembler;
};

#include "minimap_colors.h"

#endif
