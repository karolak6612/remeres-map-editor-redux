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
#include "rendering/core/sprite_loader_state.h"

void SpriteLoaderState::clear() {
	unloaded = true;
	spritefile.clear();
	sprite_archive_.reset();
	client_version = nullptr;
	dat_format = DAT_FORMAT_UNKNOWN;
	item_count = 0;
	creature_count = 0;
	is_extended = false;
	has_transparency = false;
	has_frame_durations = false;
	has_frame_groups = false;
}

wxFileName SpriteLoaderState::getMetadataFileName() const {
	return client_version ? client_version->getMetadataPath() : wxFileName();
}

wxFileName SpriteLoaderState::getSpritesFileName() const {
	return client_version ? client_version->getSpritesPath() : wxFileName();
}
