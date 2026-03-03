//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CORE_SPRITE_DECOMPRESSION_H_
#define RME_RENDERING_CORE_SPRITE_DECOMPRESSION_H_

#include <cstdint>
#include <memory>
#include <span>

[[nodiscard]] std::unique_ptr<uint8_t[]> decompress_sprite(std::span<const uint8_t> dump, bool use_alpha, int id = 0);

#endif // RME_RENDERING_CORE_SPRITE_DECOMPRESSION_H_
