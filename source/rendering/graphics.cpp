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

#include "../main.h"

#include "sprites.h"
#include "graphics.h"
#include "filehandle.h"
#include "settings.h"
#include "texture/sprite_loader.h"
#include "gui.h"
#include "otml.h"

#include <wx/mstream.h>
#include <wx/stopwatch.h>
#include <wx/dir.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include "pngfiles.h"

#include "../brushes/door_normal.xpm"
#include "../brushes/door_normal_small.xpm"
#include "../brushes/door_locked.xpm"
#include "../brushes/door_locked_small.xpm"
#include "../brushes/door_magic.xpm"
#include "../brushes/door_magic_small.xpm"
#include "../brushes/door_quest.xpm"
#include "../brushes/door_quest_small.xpm"
#include "../brushes/door_normal_alt.xpm"
#include "../brushes/door_normal_alt_small.xpm"
#include "../brushes/door_archway.xpm"
#include "../brushes/door_archway_small.xpm"

GraphicManager::GraphicManager() :
	client_version(nullptr),
	unloaded(true) {
	// Create texture subsystems
	textureManager_ = std::make_unique<rme::render::TextureManager>();
	textureCache_ = std::make_unique<rme::render::TextureCache>(*textureManager_);
	spriteLoader_ = std::make_unique<rme::render::SpriteLoader>(*this);

	animation_timer = newd wxStopWatch();
	animation_timer->Start();
}

GraphicManager::~GraphicManager() {
	delete animation_timer;
}

void GraphicManager::clear() {
	textureManager_->clear();
	textureCache_->clear();
}

void GraphicManager::cleanSoftwareSprites() {
	textureManager_->cleanSoftwareSprites();
}

Sprite* GraphicManager::getSprite(int id) {
	return textureManager_->getSprite(id);
}

GameSprite* GraphicManager::getCreatureSprite(int id) {
	return textureManager_->getCreatureSprite(id);
}

uint16_t GraphicManager::getItemSpriteMaxID() const {
	return spriteLoader_->getMetadataInfo().itemCount;
}

uint16_t GraphicManager::getCreatureSpriteMaxID() const {
	return spriteLoader_->getMetadataInfo().creatureCount;
}

GLuint GraphicManager::getFreeTextureID() {
	return textureManager_->getFreeTextureID();
}

#define loadPNGFile(name) _wxGetBitmapFromMemory(name, sizeof(name))
inline wxBitmap* _wxGetBitmapFromMemory(const unsigned char* data, int length) {
	wxMemoryInputStream is(data, length);
	wxImage img(is, "image/png");
	if (!img.IsOk()) {
		return nullptr;
	}
	return newd wxBitmap(img, -1);
}

bool GraphicManager::loadEditorSprites() {
	auto& sSpace = textureManager_->getSpriteSpace();
	// Unused graphics MIGHT be loaded here, but it's a neglectable loss
	sSpace[EDITOR_SPRITE_SELECTION_MARKER] = newd EditorSprite(
		newd wxBitmap(selection_marker_xpm16x16),
		newd wxBitmap(selection_marker_xpm32x32)
	);
	sSpace[EDITOR_SPRITE_BRUSH_CD_1x1] = newd EditorSprite(
		loadPNGFile(circular_1_small_png),
		loadPNGFile(circular_1_png)
	);
	sSpace[EDITOR_SPRITE_BRUSH_CD_3x3] = newd EditorSprite(
		loadPNGFile(circular_2_small_png),
		loadPNGFile(circular_2_png)
	);
	sSpace[EDITOR_SPRITE_BRUSH_CD_5x5] = newd EditorSprite(
		loadPNGFile(circular_3_small_png),
		loadPNGFile(circular_3_png)
	);
	sSpace[EDITOR_SPRITE_BRUSH_CD_7x7] = newd EditorSprite(
		loadPNGFile(circular_4_small_png),
		loadPNGFile(circular_4_png)
	);
	sSpace[EDITOR_SPRITE_BRUSH_CD_9x9] = newd EditorSprite(
		loadPNGFile(circular_5_small_png),
		loadPNGFile(circular_5_png)
	);
	sSpace[EDITOR_SPRITE_BRUSH_CD_15x15] = newd EditorSprite(
		loadPNGFile(circular_6_small_png),
		loadPNGFile(circular_6_png)
	);
	sSpace[EDITOR_SPRITE_BRUSH_CD_19x19] = newd EditorSprite(
		loadPNGFile(circular_7_small_png),
		loadPNGFile(circular_7_png)
	);
	sSpace[EDITOR_SPRITE_BRUSH_SD_1x1] = newd EditorSprite(
		loadPNGFile(rectangular_1_small_png),
		loadPNGFile(rectangular_1_png)
	);
	sSpace[EDITOR_SPRITE_BRUSH_SD_3x3] = newd EditorSprite(
		loadPNGFile(rectangular_2_small_png),
		loadPNGFile(rectangular_2_png)
	);
	sSpace[EDITOR_SPRITE_BRUSH_SD_5x5] = newd EditorSprite(
		loadPNGFile(rectangular_3_small_png),
		loadPNGFile(rectangular_3_png)
	);
	sSpace[EDITOR_SPRITE_BRUSH_SD_7x7] = newd EditorSprite(
		loadPNGFile(rectangular_4_small_png),
		loadPNGFile(rectangular_4_png)
	);
	sSpace[EDITOR_SPRITE_BRUSH_SD_9x9] = newd EditorSprite(
		loadPNGFile(rectangular_5_small_png),
		loadPNGFile(rectangular_5_png)
	);
	sSpace[EDITOR_SPRITE_BRUSH_SD_15x15] = newd EditorSprite(
		loadPNGFile(rectangular_6_small_png),
		loadPNGFile(rectangular_6_png)
	);
	sSpace[EDITOR_SPRITE_BRUSH_SD_19x19] = newd EditorSprite(
		loadPNGFile(rectangular_7_small_png),
		loadPNGFile(rectangular_7_png)
	);

	sSpace[EDITOR_SPRITE_OPTIONAL_BORDER_TOOL] = newd EditorSprite(
		loadPNGFile(optional_border_small_png),
		loadPNGFile(optional_border_png)
	);
	sSpace[EDITOR_SPRITE_ERASER] = newd EditorSprite(
		loadPNGFile(eraser_small_png),
		loadPNGFile(eraser_png)
	);
	sSpace[EDITOR_SPRITE_PZ_TOOL] = newd EditorSprite(
		loadPNGFile(protection_zone_small_png),
		loadPNGFile(protection_zone_png)
	);
	sSpace[EDITOR_SPRITE_PVPZ_TOOL] = newd EditorSprite(
		loadPNGFile(pvp_zone_small_png),
		loadPNGFile(pvp_zone_png)
	);
	sSpace[EDITOR_SPRITE_NOLOG_TOOL] = newd EditorSprite(
		loadPNGFile(no_logout_small_png),
		loadPNGFile(no_logout_png)
	);
	sSpace[EDITOR_SPRITE_NOPVP_TOOL] = newd EditorSprite(
		loadPNGFile(no_pvp_small_png),
		loadPNGFile(no_pvp_png)
	);

	sSpace[EDITOR_SPRITE_DOOR_NORMAL] = newd EditorSprite(
		newd wxBitmap(door_normal_small_xpm),
		newd wxBitmap(door_normal_xpm)
	);
	sSpace[EDITOR_SPRITE_DOOR_LOCKED] = newd EditorSprite(
		newd wxBitmap(door_locked_small_xpm),
		newd wxBitmap(door_locked_xpm)
	);
	sSpace[EDITOR_SPRITE_DOOR_MAGIC] = newd EditorSprite(
		newd wxBitmap(door_magic_small_xpm),
		newd wxBitmap(door_magic_xpm)
	);
	sSpace[EDITOR_SPRITE_DOOR_QUEST] = newd EditorSprite(
		newd wxBitmap(door_quest_small_xpm),
		newd wxBitmap(door_quest_xpm)
	);
	sSpace[EDITOR_SPRITE_DOOR_NORMAL_ALT] = newd EditorSprite(
		newd wxBitmap(door_normal_alt_small_xpm),
		newd wxBitmap(door_normal_alt_xpm)
	);
	sSpace[EDITOR_SPRITE_DOOR_ARCHWAY] = newd EditorSprite(
		newd wxBitmap(door_archway_small_xpm),
		newd wxBitmap(door_archway_xpm)
	);
	sSpace[EDITOR_SPRITE_WINDOW_NORMAL] = newd EditorSprite(
		loadPNGFile(window_normal_small_png),
		loadPNGFile(window_normal_png)
	);
	sSpace[EDITOR_SPRITE_WINDOW_HATCH] = newd EditorSprite(
		loadPNGFile(window_hatch_small_png),
		loadPNGFile(window_hatch_png)
	);

	sSpace[EDITOR_SPRITE_SELECTION_GEM] = newd EditorSprite(
		loadPNGFile(gem_edit_png),
		nullptr
	);
	sSpace[EDITOR_SPRITE_DRAWING_GEM] = newd EditorSprite(
		loadPNGFile(gem_move_png),
		nullptr
	);

	return true;
}

// End of GraphicManager implementation
