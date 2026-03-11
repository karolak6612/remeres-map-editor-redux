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
#include "rendering/core/render_timer.h"
#include "rendering/core/shared_geometry.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/image.h"
#include "rendering/core/normal_image.h"
#include "rendering/core/template_image.h"

class GraphicManager {
	SpriteDatabase db_;
	AtlasLifecycle atlas_;
	SpriteLoaderState loader_;
	TextureGC gc_;
	std::unique_ptr<RenderTimer> animation_timer_;
	SharedGeometry shared_geometry_;

public:
	GraphicManager();
	~GraphicManager();

	void clear();

	void cleanSoftwareSprites() { gc_.cleanSoftwareSprites(db_); }

	Sprite* getSprite(int id) { return db_.getSprite(id); }
	void updateTime() { gc_.updateTime(); }

	void pauseAnimation() { animation_timer_->Pause(); }
	void resumeAnimation() { animation_timer_->Resume(); }

	GameSprite* getCreatureSprite(int id) { return db_.getCreatureSprite(id, loader_.item_count); }

	void insertSprite(int id, std::unique_ptr<Sprite> sprite) { db_.insertSprite(id, std::move(sprite)); }
	void insertSprite(int id, Sprite* sprite) { db_.insertSprite(id, std::unique_ptr<Sprite>(sprite)); }

	long getElapsedTime() const { return animation_timer_->getElapsedTime(); }
	time_t getCachedTime() const { return gc_.getCachedTime(); }

	uint16_t getItemSpriteMaxID() const { return loader_.item_count; }
	uint16_t getCreatureSpriteMaxID() const { return loader_.creature_count; }

	bool loadEditorSprites();

	void garbageCollection() { gc_.garbageCollect(db_); }
	void addSpriteToCleanup(uint32_t sprite_id) { gc_.addSpriteToCleanup(db_, sprite_id); }
	bool shouldCollectGarbage() const noexcept { return gc_.shouldCollect(); }
	void markGarbageCollected() noexcept { gc_.markCollected(); }

	wxFileName getMetadataFileName() const { return loader_.getMetadataFileName(); }
	wxFileName getSpritesFileName() const { return loader_.getSpritesFileName(); }

	bool hasTransparency() const { return loader_.has_transparency; }
	bool isUnloaded() const { return loader_.unloaded.load(); }

	const std::string& getSpriteFile() const { return loader_.spritefile; }
	bool isExtended() const { return loader_.is_extended; }
	std::shared_ptr<SpriteArchive> getSpriteArchive() const { return loader_.sprite_archive_; }
	ClientVersion* getClientVersion() const { return loader_.client_version; }
	void setClientVersion(ClientVersion* client_version) { loader_.client_version = client_version; }

	// Atlas facade
	AtlasManager* getAtlasManager() { return atlas_.get(); }
	bool hasAtlasManager() const { return atlas_.has(); }
	bool ensureAtlasManager() { return atlas_.ensure(); }
	void clearAtlas() { atlas_.clear(); }

	NormalImage* getNormalImage(uint32_t sprite_id) const;
	NormalImage* getOrCreateNormalImage(uint32_t sprite_id);
	void trackResidentImage(Image* image) { db_.residentImages().push_back(image); }
	void notifyTextureLoaded() { gc_.collector().NotifyTextureLoaded(); }
	void notifyTextureUnloaded() { gc_.collector().NotifyTextureUnloaded(); }
	void resizeStorage(size_t sprite_size, size_t image_size) { db_.resize(sprite_size, image_size); }
	void resetLoadedGraphicsState();
	void finalizeLoadedCatalog(
		DatFormat dat_format,
		uint16_t item_count,
		uint16_t creature_count,
		bool is_extended,
		bool has_transparency,
		bool has_frame_durations,
		bool has_frame_groups,
		std::shared_ptr<SpriteArchive> sprite_archive
	);
	SpritePreloader& spritePreloader() { return gc_.preloader(); }

	// Shared GPU geometry (quad VBO/EBO)
	SharedGeometry& sharedGeometry() { return shared_geometry_; }
};

#endif
