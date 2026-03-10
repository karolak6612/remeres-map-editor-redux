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

#ifndef RME_RENDERING_CORE_SPRITE_LOADER_STATE_H_
#define RME_RENDERING_CORE_SPRITE_LOADER_STATE_H_

#include <atomic>
#include <memory>
#include <string>
#include <cstdint>
#include <wx/filename.h>
#include "app/client_version.h"

class SpriteArchive;

class SpriteLoaderState {
public:
	void clear();

	// All public — GraphicsAssembler writes directly to these.
	std::atomic<bool> unloaded { true };
	std::string spritefile;
	std::shared_ptr<SpriteArchive> sprite_archive_;
	ClientVersion* client_version = nullptr;
	DatFormat dat_format;
	uint16_t item_count = 0;
	uint16_t creature_count = 0;
	bool is_extended = false;
	bool has_transparency = false;
	bool has_frame_durations = false;
	bool has_frame_groups = false;

	wxFileName getMetadataFileName() const;
	wxFileName getSpritesFileName() const;
};

#endif
