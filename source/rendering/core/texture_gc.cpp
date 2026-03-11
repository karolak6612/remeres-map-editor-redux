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
#include "rendering/core/texture_gc.h"
#include "rendering/core/sprite_database.h"
#include "rendering/core/sprite_preloader.h"

TextureGC::TextureGC() {
	preloader_ = std::make_unique<SpritePreloader>();
}

TextureGC::~TextureGC() = default;

void TextureGC::updateTime() {
	cached_time_ = time(nullptr);
	preloader_->update();
}

void TextureGC::addSpriteToCleanup(SpriteDatabase& db, uint32_t sprite_id, const GraphicsRuntimeConfig& config) {
	collector_.AddSpriteToCleanup(db.residentGameSprites(), sprite_id, config);
}

void TextureGC::garbageCollect(SpriteDatabase& db, const GraphicsRuntimeConfig& config) {
	collector_.GarbageCollect(db.residentGameSprites(), db.residentImages(), cached_time_, config);
}

void TextureGC::cleanSoftwareSprites(SpriteDatabase& db) {
	collector_.CleanSoftwareSprites(db.sprites());
}

void TextureGC::clear() {
	collector_.Clear();
}
