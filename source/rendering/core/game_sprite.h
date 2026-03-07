//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CORE_GAME_SPRITE_H_
#define RME_RENDERING_CORE_GAME_SPRITE_H_

#include "game/outfit.h"
#include "rendering/core/animator.h"
#include "rendering/core/atlas_manager.h"
#include "rendering/core/render_timer.h"
#include "rendering/core/sprite_atlas_cache.h"
#include "rendering/core/sprite_light.h"
#include "rendering/core/sprite_metadata.h"
#include "rendering/core/texture_gc.h"
#include "rendering/ui/sprite_icon_renderer.h"
#include "rendering/ui/sprite_size.h"
#include "util/common.h"

#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/dcmemory.h>
#include <atomic>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

class SpriteDatabase;
class SpriteLoader;
class SpritePreloader;

class Sprite {
public:
    Sprite() { }
    virtual ~Sprite() = default;

    virtual void DrawTo(SpriteDatabase& sprites, TextureGC& gc, wxDC* dc, SpriteSize sz, int start_x, int start_y, int width = -1, int height = -1) = 0;
    virtual void unloadDC() = 0;
    virtual wxSize GetSize() const = 0;

    Sprite(const Sprite&) = delete;
    Sprite& operator=(const Sprite&) = delete;
};

class CreatureSprite : public Sprite {
public:
    CreatureSprite(uint32_t clientID, const Outfit& outfit);
    ~CreatureSprite() override;

    void DrawTo(SpriteDatabase& sprites, TextureGC& gc, wxDC* dc, SpriteSize sz, int start_x, int start_y, int width = -1, int height = -1) override;
    void unloadDC() override;
    wxSize GetSize() const override
    {
        return wxSize(32, 32);
    }

    uint32_t clientID;
    Outfit outfit;
};

#endif
