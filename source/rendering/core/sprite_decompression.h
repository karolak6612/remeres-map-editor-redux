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

#ifndef RME_RENDERING_CORE_SPRITE_DECOMPRESSION_H_
#define RME_RENDERING_CORE_SPRITE_DECOMPRESSION_H_

#include <cstdint>
#include <memory>
#include <span>

namespace SpriteDecompression {

// Decompress a sprite from RLE-encoded dump data into RGBA pixel data.
[[nodiscard]] std::unique_ptr<uint8_t[]> Decompress(std::span<const uint8_t> dump, bool use_alpha, int id = 0);

// Apply outfit colorization to pixel data using a template mask.
void ColorizeTemplatePixels(uint8_t* dest, const uint8_t* mask, size_t pixelCount,
	int lookHead, int lookBody, int lookLegs, int lookFeet, bool destHasAlpha);

} // namespace SpriteDecompression

#endif
