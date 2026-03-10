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
#include "rendering/core/sprite_database.h"
#include "rendering/core/atlas_lifecycle.h"
#include "rendering/core/sprite_loader_state.h"
#include "rendering/core/texture_gc.h"
#include "rendering/core/atlas_manager.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/image.h"
#include "rendering/core/normal_image.h"
#include "rendering/core/template_image.h"

class GraphicManager {
public:
	GraphicManager();
	~GraphicManager();

	void clear();

	void cleanSoftwareSprites() { gc_.cleanSoftwareSprites(db_); }

	Sprite* getSprite(int id) { return db_.getSprite(id); }
	void updateTime() { gc_.updateTime(); }

	void pauseAnimation() { gc_.pauseAnimation(); }
	void resumeAnimation() { gc_.resumeAnimation(); }

	GameSprite* getCreatureSprite(int id) { return db_.getCreatureSprite(id, loader_.item_count); }

	void insertSprite(int id, std::unique_ptr<Sprite> sprite) { db_.insertSprite(id, std::move(sprite)); }
	void insertSprite(int id, Sprite* sprite) { db_.insertSprite(id, std::unique_ptr<Sprite>(sprite)); }

	long getElapsedTime() const { return gc_.getElapsedTime(); }
	time_t getCachedTime() const { return gc_.getCachedTime(); }

	uint16_t getItemSpriteMaxID() const { return loader_.item_count; }
	uint16_t getCreatureSpriteMaxID() const { return loader_.creature_count; }

	bool loadEditorSprites();

	void garbageCollection() { gc_.garbageCollect(db_); }
	void addSpriteToCleanup(GameSprite* spr) { gc_.addSpriteToCleanup(spr); }

	wxFileName getMetadataFileName() const { return loader_.getMetadataFileName(); }
	wxFileName getSpritesFileName() const { return loader_.getSpritesFileName(); }

	bool hasTransparency() const { return loader_.has_transparency; }
	bool isUnloaded() const { return loader_.unloaded.load(); }

	const std::string& getSpriteFile() const { return loader_.spritefile; }
	bool isExtended() const { return loader_.is_extended; }
	std::shared_ptr<SpriteArchive> getSpriteArchive() const { return loader_.sprite_archive_; }

	// client_version exposed via loader for backward compatibility
	ClientVersion*& client_version;

	// Atlas facade
	AtlasManager* getAtlasManager() { return atlas_.get(); }
	bool hasAtlasManager() const { return atlas_.has(); }
	bool ensureAtlasManager() { return atlas_.ensure(); }

	// Sub-object accessors for internal/friend use
	SpriteDatabase& db() { return db_; }
	AtlasLifecycle& atlas() { return atlas_; }
	SpriteLoaderState& loader() { return loader_; }
	TextureGC& gc() { return gc_; }

private:
	SpriteDatabase db_;
	AtlasLifecycle atlas_;
	SpriteLoaderState loader_;
	TextureGC gc_;
};

#include "minimap_colors.h"

#endif
