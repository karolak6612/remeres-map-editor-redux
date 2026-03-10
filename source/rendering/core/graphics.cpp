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
	client_version(loader_.client_version) {
}

GraphicManager::~GraphicManager() {
	atlas_.clear();
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

bool GraphicManager::loadEditorSprites() {
	return EditorSpriteLoader::Load(this);
}

void NVGDeleter::operator()(NVGcontext* nvg) const {
	if (nvg) {
		nvgDeleteGL3(nvg);
	}
}
