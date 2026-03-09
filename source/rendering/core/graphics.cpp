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

#include "app/main.h"

#include "game/sprites.h"
#include "rendering/core/graphics.h"
#include "rendering/core/sprite_preloader.h"
#include <nanovg.h>
#include <spdlog/spdlog.h>
#include <nanovg_gl.h>
#include "io/filehandle.h"
#include "app/settings.h"
#include "ui/gui.h"

#include "rendering/io/editor_sprite_loader.h"

#include <wx/mstream.h>
#include <wx/dir.h>
#include "rendering/utilities/wx_utils.h"

#include "rendering/core/outfit_colors.h"
#include "rendering/core/outfit_colorizer.h"
#include <atomic>
#include <functional>

GraphicManager::GraphicManager() :
	client_version(nullptr),
	unloaded(true),
	dat_format(DAT_FORMAT_UNKNOWN),
	is_extended(false),
	has_transparency(false),
	has_frame_durations(false),
	has_frame_groups(false) {
	animation_timer = std::make_unique<RenderTimer>();
	animation_timer->Start();
}

// Note: getSprite, insertSprite, getCreatureSprite, getItemSpriteMaxID,
// getCreatureSpriteMaxID are now inline in graphics.h, delegating to SpriteDatabase.

GraphicManager::~GraphicManager() {
	// Unique pointers handle deletion automatically
	// atlas_manager_ clean up still good to be explicit if it has custom clear logic
	if (atlas_manager_) {
		atlas_manager_->clear();
	}
}

bool GraphicManager::hasTransparency() const {
	return has_transparency;
}

bool GraphicManager::isUnloaded() const {
	return unloaded.load();
}

void GraphicManager::updateTime() {
	cached_time_ = time(nullptr);
	SpritePreloader::get().update();
}

void GraphicManager::clear() {
	// CRITICAL: Ensure preloader is cleared before modifying image_space to avoid
	// use-after-free or OOB access in SpritePreloader::update() on main thread.
	SpritePreloader::get().clear();
	sprites.clear();
	resident_images.clear();
	resident_game_sprites.clear();

	collector.Clear();
	spritefile = "";
	sprite_archive_.reset();

	// Cleanup atlas manager (will be reinitialized lazily when needed)
	if (atlas_manager_) {
		atlas_manager_->clear();
		atlas_manager_.reset();
	}

	client_version = nullptr;
	unloaded = true;
	dat_format = DAT_FORMAT_UNKNOWN;
	is_extended = false;
	has_transparency = false;
	has_frame_durations = false;
	has_frame_groups = false;
}

void GraphicManager::cleanSoftwareSprites() {
	collector.CleanSoftwareSprites(sprites.spriteSpace());
}

bool GraphicManager::ensureAtlasManager() {
	// Already initialized
	if (atlas_manager_ && atlas_manager_->isValid()) {
		return true;
	}

	// Create and initialize on first use
	if (!atlas_manager_) {
		atlas_manager_ = std::make_unique<AtlasManager>();
	}

	// Lazy initialization happens inside AtlasManager::ensureInitialized()
	if (!atlas_manager_->ensureInitialized()) {
		spdlog::error("GraphicManager: Failed to initialize atlas manager");
		atlas_manager_.reset();
		return false;
	}

	return true;
}

// getSprite, insertSprite, getCreatureSprite, getItemSpriteMaxID,
// getCreatureSpriteMaxID moved to SpriteDatabase (see sprite_database.cpp)

bool GraphicManager::loadEditorSprites() {
	return EditorSpriteLoader::Load(this);
}

void GraphicManager::addSpriteToCleanup(GameSprite* spr) {
	collector.AddSpriteToCleanup(spr);
}

void GraphicManager::garbageCollection() {
	collector.GarbageCollect(resident_game_sprites, resident_images, cached_time_);
}

void NVGDeleter::operator()(NVGcontext* nvg) const {
	if (nvg) {
		nvgDeleteGL3(nvg);
	}
}
