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

#ifndef RME_RENDERING_CORE_SPRITE_METADATA_H_
#define RME_RENDERING_CORE_SPRITE_METADATA_H_

#include <cstdint>
#include "rendering/core/sprite_light.h"

// Flat data struct holding all metadata for a game sprite.
// Separated from GameSprite to clarify data vs rendering concerns.
struct SpriteMetadata {
	uint32_t id = 0;
	uint8_t height = 0;
	uint8_t width = 0;
	uint8_t layers = 0;
	uint8_t pattern_x = 0;
	uint8_t pattern_y = 0;
	uint8_t pattern_z = 0;
	uint8_t frames = 0;
	uint32_t numsprites = 0;

	uint16_t draw_height = 0;
	uint16_t drawoffset_x = 0;
	uint16_t drawoffset_y = 0;

	uint16_t minimap_color = 0;

	bool has_light = false;
	SpriteLight light;

	bool is_simple = false;
};

#endif
