//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_UTILITIES_SPRITE_ICON_GENERATOR_H_
#define RME_RENDERING_UTILITIES_SPRITE_ICON_GENERATOR_H_

#include "game/creature.h"
#include "rendering/ui/sprite_size.h"
#include <wx/bitmap.h>
#include <cstdint>

class SpriteDatabase;
class SpriteLoader;

class SpriteIconGenerator {
public:
    static wxBitmap Generate(SpriteDatabase& sprites, SpriteLoader& loader, uint32_t clientID, SpriteSize size, bool rescale = true);
    static wxBitmap Generate(SpriteDatabase& sprites, SpriteLoader& loader, uint32_t clientID, SpriteSize size, const Outfit& outfit, bool rescale = true, Direction direction = SOUTH);
};

#endif
