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
#include "rendering/core/sprite_archive.h"
#include "rendering/core/sprite_preloader.h"
#include <nanovg.h>
#include <spdlog/spdlog.h>
#include <nanovg_gl.h>
#include "io/filehandle.h"
#include "rendering/io/editor_sprite_loader.h"

#include <wx/mstream.h>
#include <wx/dir.h>
#include "rendering/utilities/wx_utils.h"

#include "rendering/core/outfit_colors.h"
#include "rendering/core/outfit_colorizer.h"
#include <atomic>
#include <functional>

GraphicManager::GraphicManager() {
	animation_timer_ = std::make_unique<RenderTimer>();
	animation_timer_->Start();
	gc_.preloader().setGraphicManager(this);
}

GraphicManager::~GraphicManager() {
	atlas_.clear();
}

NormalImage* GraphicManager::getNormalImage(uint32_t sprite_id) const {
	if (sprite_id >= db_.images().size()) {
		return nullptr;
	}
	auto* image = db_.images()[sprite_id].get();
	if (!image || !image->isNormalImage()) {
		return nullptr;
	}
	return static_cast<NormalImage*>(image);
}

NormalImage* GraphicManager::getOrCreateNormalImage(uint32_t sprite_id) {
	if (sprite_id >= db_.images().size()) {
		return nullptr;
	}

	auto& slot = db_.images()[sprite_id];
	if (!slot) {
		auto image = std::make_unique<NormalImage>();
		image->id = sprite_id;
		image->setGraphicManager(this);
		slot = std::move(image);
	}
	return static_cast<NormalImage*>(slot.get());
}

void GraphicManager::clear() {
	// CRITICAL: Ensure preloader is cleared before modifying image_space to avoid
	// use-after-free or OOB access in SpritePreloader::update() on main thread.
	gc_.preloader().clear();
	db_.clear();
	loader_.clear();
	gc_.clear();
	atlas_.clear();
}

void GraphicManager::resetLoadedGraphicsState() {
	gc_.preloader().clear();
	loader_.unloaded = true;
	loader_.sprite_archive_.reset();
	loader_.spritefile.clear();
	db_.clear();
	gc_.clear();
	atlas_.clear();
}

void GraphicManager::finalizeLoadedCatalog(
	DatFormat dat_format,
	uint16_t item_count,
	uint16_t creature_count,
	bool is_extended,
	bool has_transparency,
	bool has_frame_durations,
	bool has_frame_groups,
	std::shared_ptr<SpriteArchive> sprite_archive
) {
	loader_.dat_format = dat_format;
	loader_.item_count = item_count;
	loader_.creature_count = creature_count;
	loader_.is_extended = is_extended;
	loader_.has_transparency = has_transparency;
	loader_.has_frame_durations = has_frame_durations;
	loader_.has_frame_groups = has_frame_groups;
	loader_.sprite_archive_ = std::move(sprite_archive);
	loader_.spritefile = loader_.sprite_archive_ ? loader_.sprite_archive_->fileName() : std::string {};
	loader_.unloaded = false;
}

bool GraphicManager::loadEditorSprites() {
	return EditorSpriteLoader::Load(this);
}

void GraphicManager::insertSprite(int id, std::unique_ptr<Sprite> sprite) {
	if (auto* game_sprite = dynamic_cast<GameSprite*>(sprite.get())) {
		game_sprite->setGraphicManager(this);
	}
	db_.insertSprite(id, std::move(sprite));
}

void NVGDeleter::operator()(NVGcontext* nvg) const {
	if (nvg) {
		nvgDeleteGL3(nvg);
	}
}
