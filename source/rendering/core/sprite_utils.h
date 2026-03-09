//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_SPRITE_UTILS_H_
#define RME_SPRITE_UTILS_H_

#include <cstdint>
#include <memory>
#include <span>

/// Static utility functions for sprite data processing.
/// Extracted from GameSprite to decouple data processing from sprite rendering.
namespace SpriteUtils {

/// Decompress raw sprite data into RGBA pixel buffer.
/// Thread-safe — can be called from preloader thread.
[[nodiscard]] std::unique_ptr<uint8_t[]> Decompress(std::span<const uint8_t> dump, bool use_alpha, int id = 0);

/// Apply outfit colors to creature template pixels.
/// Reads mask data and writes colorized pixels to dest buffer.
void ColorizeTemplatePixels(uint8_t* dest, const uint8_t* mask, size_t pixelCount,
	int lookHead, int lookBody, int lookLegs, int lookFeet, bool destHasAlpha);

} // namespace SpriteUtils

#endif
